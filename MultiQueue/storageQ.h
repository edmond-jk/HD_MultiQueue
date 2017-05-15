#ifndef STORAGE_QUEUE_H_ 
#define STORAGE_QUEUE_H_

#include <systemc.h>
#include "hd_multiqueue.h"

SC_MODULE(storageQ)
{
	sc_in<bool>						clock;
	sc_in<bool>						reset;
	sc_in<bool>						sq_select;
	sc_in <sc_biguint<CMD_WIDTH> >	cmd_in;
	sc_out<bool>					status_update_enable;
	sc_out<sc_uint<8> >				cmdq_index;
	sc_uint<8>						head, tail;
	sc_biguint<CMD_WIDTH>			SQ[MAX_CMDQ_DEPTH]; 

	void initializer(void);
	void receive_cmd(void);
	void process_cmd(void);

	SC_CTOR(storageQ)
	{
		SC_METHOD(initializer);
		dont_initialize();
		sensitive << reset;

		SC_THREAD(receive_cmd);
		sensitive << sq_select;

		SC_THREAD(process_cmd);
		sensitive << clock;
	}
};

void storageQ::initializer(void)
{
	if (reset.read() == 1)
	{
		head = tail = 0;
	}
}

void storageQ::receive_cmd(void)
{
	while (true)
	{ 
		wait();

		if (sq_select.read() == 1)
		{
			SQ[head] = cmd_in.read();
			
			head++;
			if (head == MAX_CMDQ_DEPTH)
			{
				head = 0;
			}
		}
	}
}

void storageQ::process_cmd(void)
{
	sc_uint<8> tmp_byte;

	while (true)
	{
		wait();
		status_update_enable.write(0);
		
		if (head != tail)
		{
			wait(100, SC_NS);
			status_update_enable.write(1);
			tmp_byte = SQ[tail].range((BASE_INDEX + INDEX_WIDTH - 1), BASE_INDEX);
			cmdq_index.write(tmp_byte);
			tail++;
			if (tail == MAX_CMDQ_DEPTH)
			{
				tail = 0;
			}
		}
	}
}
#endif // STORAGE_QUEUE_H_ 
