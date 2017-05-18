/*
 * IFQueue.h
 *
 *  Created on: May 2, 2017
 *      Author: jk
 */

#ifndef IFQUEUE_H_
#define IFQUEUE_H_

 // type conversion from sc_uint to sc_int or int... static_cast doesn't work
#pragma warning( disable : 4244)  

#include <systemc.h>
#include "hd_multiqueue.h"

SC_MODULE(IF_QUEUE) {
	sc_in<bool>							reset;

	// for DRAM Interface/Parser
	sc_in<bool> 						clock_host;
	sc_in<bool> 						cmdq_select;
	sc_in<sc_uint<HDATA_WIDTH> > 	cmd_in; // 4 Bytes
	// For xfer_buf
	sc_in<bool>							clock_fpga;
	sc_out<bool>						xfer_buf_select;
	sc_out<bool>						mwrite_enable;
	sc_out<sc_uint<32> >				tbm_address;
	sc_in<bool>							xfer_complete;
	// for Storage Queue
	sc_out<bool>						sq_select;
	sc_out<sc_biguint<MDATA_WIDTH> >	cmd_out;
	sc_in<bool>							status_update_enable;
	sc_in<sc_uint<8> >					cmdq_index;
	// for Query command 
	sc_in<bool>							queryin_select;
	sc_out<bool>						queryout_select;
	sc_inout<sc_uint<8> >				querydata_inout;

	/*
	Command Queue length is 32.. When host issueing new command, host has to confirm if the number of 
	pending commands reaches the limit. Device won't check this. 
	*/
	sc_biguint<MDATA_WIDTH>	ifcmdQ[MAX_CMDQ_DEPTH]; // 32 Byte x 32 Queue Depth
	sc_uint<8>	head, tail, current;
	sc_uint<8> in_offset, out_offset;
	sc_event ev_complete; // when handling each command, ev_complete is used to confirm if data xfer 
						  // between host and xfer (or IO) buffer is done. 
	sc_mutex xfer_buf_control; // for operation synchronization. 

	void core_process(void);
	void receive_cmd(void);
	void process_cmd(void);
	void update_status(void);
	void close_cmd(void);

	/*
	 * if needed, modify clock_fpga and clock_host...
	 */
	SC_CTOR(IF_QUEUE) {
		SC_METHOD(core_process);
		dont_initialize();
		sensitive << clock_host.pos() << clock_host.neg();
		sensitive << clock_fpga.pos() << clock_fpga.neg();
		sensitive << xfer_complete;

		SC_THREAD(receive_cmd);
		sensitive << clock_host.pos() << clock_host.neg() << cmdq_select;

		SC_THREAD(process_cmd);
		sensitive << clock_fpga.pos() << clock_fpga.neg() << xfer_complete;

		SC_THREAD(update_status);
		sensitive << status_update_enable;

		SC_THREAD(close_cmd);
		sensitive << xfer_complete << clock_host.neg() << clock_host.pos();
	}
};

void IF_QUEUE::core_process(void)
{
	if(reset.read() == 1)
	{
		head = tail = current = 0;
		in_offset = out_offset = 0;

		cout << "@" << sc_time_stamp() << ", head: " << head << ", tail: " << tail << ", in_offset: "<< in_offset << ", out_offset: "<< out_offset << endl;
	}

	if (xfer_complete.read() == 1)
	{
		ev_complete.notify();
	}
}

// host have to insert commands into cmdq not for ifcmdq to overflow
void IF_QUEUE::receive_cmd(void)
{
	while (true)
	{
		wait();

		if (cmdq_select.read() == 1)
		{
			ifcmdQ[head].range((HDATA_WIDTH*(in_offset + 1) - 1), (HDATA_WIDTH*in_offset)) = cmd_in;
			cout << "receive cmd] offset:" << in_offset << ", incomming command:" << cmd_in << endl;

			in_offset++;
			if (in_offset == DATA_WIDTH_DIFF)
			{
				in_offset = 0;
				ifcmdQ[head].range(BASE_STATUS+STATUS_WIDTH-1, BASE_STATUS) = ST_QUEUED2D;

				cout << "receive cmd is done" << endl;

				head++;
				if (head == MAX_CMDQ_DEPTH)
				{
					head = 0;
				}
			}

		}

	}
}

void IF_QUEUE::process_cmd(void)
{
	sc_uint<8> 	i;
	sc_uint<8>  num_rqsz;// number of requested 16KB buffers
	sc_uint<8>	tmp_byte;
	sc_uint<32>	tmp_dword;


	while (true)
	{
		wait();

		if (current != head)
		{
			/*
			 * Supposed that all requests should be 16KB boundary...
			 * If needed, this part has to be modified afterwards..
			 */
	//		num_rqsz = ifcmdQ[current].range(BASE_RQSZ + RQSZ_WIDTH -1, BASE_RQSZ) >> 5;
			num_rqsz = ifcmdQ[current].range(BASE_RQSZ + RQSZ_WIDTH -1, BASE_RQSZ) >> 3; // 4KB.. 8 sectors

			cout << "@" << sc_time_stamp() << ", request size:" << num_rqsz << endl;
			
			i = 0;

			// xfer_buf --> tbm
			if (ifcmdQ[current].range(BASE_CMD + CMD_WIDTH - 1, BASE_CMD) == BSM_WRITE)
			{
				cout << "1. ifcmdQ:" << ifcmdQ[current] << endl;

				ifcmdQ[current].range(BASE_STATUS + STATUS_WIDTH -1, BASE_STATUS) = ST_XFER2D;

				cout << "2. ifcmdQ:" << ifcmdQ[current] << endl;

				xfer_buf_control.lock();

				while (i < num_rqsz)
				{
					tmp_byte = ifcmdQ[current].range(BASE_VALID + VALID_WIDTH -1, BASE_VALID);
					if((tmp_byte >> i) & 0x01)
					{
						/*
						 * Suppose that each request size should be 16KB unit.
						 */

						xfer_buf_select.write(1);
						mwrite_enable.write(1);

						tmp_dword = ifcmdQ[current].range(BASE_ADDR + ADDR_WIDTH *(i+1) - 1, BASE_ADDR + ADDR_WIDTH*i);
						tbm_address = tmp_dword;

						wait(ev_complete);

						// XXX check...
						xfer_buf_select.write(0);
						mwrite_enable.write(0);
					}

					i++;
				} // while-loop

				xfer_buf_control.unlock();

				ifcmdQ[current].range(BASE_STATUS + STATUS_WIDTH -1, BASE_STATUS) = ST_DONEB;
				/*
				 * TODO: this command descriptor should be copied to the storage queue.
				 */

			}
			// confirm if the requested data reside on tbm, otherwise issue the request to storage
			else if (ifcmdQ[current].range(BASE_CMD + CMD_WIDTH - 1, BASE_CMD) == BSM_READ)
			{
				while (i < num_rqsz)
				{
					tmp_byte = ifcmdQ[current].range(BASE_VALID + VALID_WIDTH -1, BASE_VALID);

					if ((tmp_byte >> i) & 1)
					{
						// nothing to do..
					}
					else
					{
						/*
						 * TODO: this command descriptor should be copied to the storage queue
						 */
						ifcmdQ[current].range(BASE_STATUS + STATUS_WIDTH-1, BASE_STATUS) = ST_WAITS;
					}
				} // while-loop

				if (ifcmdQ[current].range(BASE_STATUS + STATUS_WIDTH-1, BASE_STATUS) != ST_WAITS)
				{
					ifcmdQ[current].range(BASE_STATUS + STATUS_WIDTH-1, BASE_STATUS) = ST_DONEB;
				}
			}

			current++;
			if (current == MAX_CMDQ_DEPTH)
			{
				current = 0;
			}
		}
	} // while-loop
}

void IF_QUEUE::update_status(void)
{
	while (true)
	{
		wait();

		if (status_update_enable.read() == 1)
		{
			ifcmdQ[cmdq_index.read()].range(BASE_STATUS + STATUS_WIDTH - 1, BASE_STATUS) = ST_DONED;
		}
	}
}

void IF_QUEUE::close_cmd(void)
{
	sc_uint<8> t_query_out;
	sc_uint<8> k;
	sc_uint<8> tmp_byte;
	sc_uint<32>	tmp_dword;

	while (true)
	{
		wait();

		queryout_select.write(0);

		if (queryin_select.read() == 1)
		{
			// Host supposes that command will be processed in the sequence of FIFO..
			if (ifcmdQ[tail].range(BASE_TAG + TAG_WIDTH -1, BASE_TAG) == querydata_inout.read())
			{
				cout << "3. ifcmdQ:" << ifcmdQ[tail] << endl;
				
				if (ifcmdQ[tail].range(BASE_CMD + CMD_WIDTH - 1, BASE_CMD) == BSM_WRITE)
				{
					if ((ifcmdQ[tail].range(BASE_STATUS + STATUS_WIDTH -1, BASE_STATUS) == ST_DONEB) ||
							(ifcmdQ[tail].range(BASE_STATUS + STATUS_WIDTH -1, BASE_STATUS) == ST_DONED))
					{
						t_query_out.range(3,2) = QS_DONE;
						// other bits should be set..
						queryout_select.write(1);
						querydata_inout.write (t_query_out);

						ifcmdQ[tail].range(BASE_STATUS + STATUS_WIDTH -1, BASE_STATUS) = ST_FREE;
						tail++;
						if (tail == MAX_CMDQ_DEPTH)
						{
							tail = 0;
						}
					}
					else
					{
						t_query_out.range(3,2) = QS_INPROCESS;
						// other bits should be set..
						queryout_select.write(1);
						querydata_inout.write (t_query_out);
					}
				}
				else if (ifcmdQ[tail].range(BASE_CMD + CMD_WIDTH -1, BASE_CMD) == BSM_READ)
				{
					if ((ifcmdQ[tail].range(BASE_STATUS + STATUS_WIDTH -1, BASE_STATUS) == ST_DONEB) ||
							(ifcmdQ[tail].range(BASE_STATUS + STATUS_WIDTH -1, BASE_STATUS) == ST_DONED))
					{
						t_query_out.range(3,2) = QS_DONE;
						queryout_select.write(1);
						querydata_inout.write(t_query_out);

						k = 0;
						xfer_buf_control.lock();

						while (k < MAX_BUF_PER_CMD)
						{
							tmp_byte = ifcmdQ[tail].range(BASE_VALID + VALID_WIDTH -1, BASE_VALID);
							if((tmp_byte >> k) & 1)
							{
								xfer_buf_select.write(1);
								mwrite_enable.write(0);
								tmp_dword = ifcmdQ[tail].range(BASE_ADDR + ADDR_WIDTH *(k+1) - 1, BASE_ADDR + ADDR_WIDTH*k);

								tbm_address = tmp_dword;
								wait(ev_complete);
							}
							k++;
						} // while-loop

						xfer_buf_control.unlock();

						ifcmdQ[tail].range(BASE_STATUS + STATUS_WIDTH -1, BASE_STATUS) = ST_FREE;
						tail++;
						if (tail == MAX_CMDQ_DEPTH)
						{
							tail = 0;
						}
					}
					else
					{
						t_query_out.range(3,2) = QS_INPROCESS;
						queryout_select.write(1);
						querydata_inout.write (t_query_out);
					}

				}
				else
				{
					cout << "The requested query is not a BSM write and a BSM READ." << endl;
				}
			}
			else
			{
				cout << "there is no cmd with the requested tag" << endl;
			}
		}

	}

}

#endif /* IFQUEUE_H_ */
