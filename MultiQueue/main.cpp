#include "tbm.h"
#include "xfer_buffer.h"
#include "IFQueue.h"

/* tbm + xfer_buf integration test*/
void test_case_1(void)
{
	// to/from interface
	sc_clock 					clock_host("clock_host", 1, SC_NS, 0.5);
	sc_clock 					clock_fpga("clock_fpga", 1, SC_NS, 0.5);
	sc_signal<bool>				host_select;
	sc_signal<bool>				hwrite_enable;
	sc_signal<sc_uint<32> > 	buf_address;
	sc_signal<sc_uint<32>, SC_MANY_WRITERS>		hostdata_inout;
	sc_signal<bool>				reset;

	// to/from interface
	sc_signal<bool>				gs_select;
	sc_signal<bool>				gs_write_enable;
	sc_signal<bool>				gs_out_enable;
	sc_signal<sc_uint<8> >		gs_out;

	// to/from IFQ
	sc_signal<bool>				xfer_buf_select;
	sc_signal<bool>				mwrite_enable;
	sc_signal<sc_uint<32> >		tbm_address;
	sc_signal<bool>				xfer_complete; // XXX

	// to/from tbm
	sc_signal<bool>				cs_con;
	sc_signal<bool>				we_con;
	sc_signal<sc_uint<MADDR_WIDTH> >		maddress_con;
	sc_signal<sc_biguint<MDATA_WIDTH>, SC_MANY_WRITERS>		mdata_io_con;

	sc_signal<bool>				cs_con1;
	sc_signal<bool>				we_con1;
	sc_signal<sc_uint<MADDR_WIDTH> >		maddress_con1;
	sc_signal<sc_biguint<MDATA_WIDTH> >		mdata_io_con1;

	xfer_buffer 				iobuf("xfer_buffer");
	ram_dp_ar_aw				tbm("tbm");
	int i;

	tbm.cs_0(cs_con);
	tbm.we_0(we_con);
	tbm.data_0(mdata_io_con);
	tbm.address_0(maddress_con);
	tbm.cs_1(cs_con1);
	tbm.we_1(we_con1);
	tbm.data_1(mdata_io_con1);
	tbm.address_1(maddress_con1);

	iobuf.reset(reset);
	iobuf.clock_host(clock_host);
	iobuf.clock_fpga(clock_fpga);

	iobuf.host_select(host_select);
	iobuf.hwrite_enable(hwrite_enable);
	iobuf.buf_address(buf_address);
	iobuf.hostdata_inout(hostdata_inout);

	iobuf.gs_select(gs_select);
	iobuf.gs_write_enable(gs_write_enable);
	iobuf.gs_out_enable(gs_out_enable);
	iobuf.gs_out(gs_out);

	iobuf.xfer_buf_select(xfer_buf_select);
	iobuf.mwrite_enable(mwrite_enable);
	iobuf.tbm_address(tbm_address);
	iobuf.xfer_complete(xfer_complete);

	iobuf.chip_select(cs_con);
	iobuf.write_enable(we_con);
	iobuf.maddress(maddress_con);
	iobuf.mdata_inout(mdata_io_con);

	sc_start(0, SC_NS);
	/*
	 * BSM WRITE
	 * 1. enqueue new command
	 * 2. get general write status
	 * 3. <FAKE READ>
	 * .....
	 * 4. enqueue query command for issued command
	 * 5. read query status...
	 */
	reset = 1;

	sc_start(1, SC_NS);

	reset = 0; 

	gs_select = 1;
	gs_write_enable = 1;

	for (i = 0; i < 10; i++)
	{
		sc_start(0.25, SC_NS);
		if (gs_out_enable == 1)
		{
			cout << "@" << sc_time_stamp() <<", general status:" << gs_out << endl;
		}
		sc_start(0.25, SC_NS);
	
		gs_select = 0;
		gs_write_enable = 1;
		cout << "-->@" << sc_time_stamp() << ", general status:" << gs_out << endl;
	}

	host_select = 1;
	hwrite_enable = 1;
	buf_address = 0;
	for (i = 0; i < 1024; i++)
	{
		hostdata_inout = i;
		sc_start(0.5, SC_NS);
	}

	host_select = 0;

	xfer_buf_select = 1;
	mwrite_enable = 1;
	tbm_address = 0;
	
	for (i = 0; i < 128; i++)
	{
		sc_start(0.5, SC_NS);
	}
	
	xfer_buf_select = 1;
	mwrite_enable = 0;
	tbm_address = 0;
	
	for (i = 0; i < 128; i++)
	{
		sc_start(0.5, SC_NS);
	}
	
}

/* tbm + xfer_buf + IF queue test */
void test_case_2(void)
{
	// to/from interface
	sc_clock 					clock_host("clock_host", 1, SC_NS, 0.5);
	sc_clock 					clock_fpga("clock_fpga", 1, SC_NS, 0.5);
	sc_signal<bool>				host_select;
	sc_signal<bool>				hwrite_enable;
	sc_signal<sc_uint<32> > 	buf_address;
	sc_signal<sc_uint<32>, SC_MANY_WRITERS>		hostdata_inout;
	sc_signal<bool>				reset;
	sc_signal<bool>				cmdq_select;
	sc_signal<sc_uint<32> >		cmd_in;
	sc_signal<bool>				queryin_select;
	sc_signal<bool>				queryout_select;
	sc_signal<sc_uint<8>, SC_MANY_WRITERS>		querydata_inout; // deliver tag number to ifq.. 

	// to/from interface
	sc_signal<bool>				gs_select;
	sc_signal<bool>				gs_write_enable;
	sc_signal<bool>				gs_out_enable;
	sc_signal<sc_uint<8> >		gs_out;

	// to/from tbm
	sc_signal<bool>				cs_con;
	sc_signal<bool>				we_con;
	sc_signal<sc_uint<MADDR_WIDTH> >		maddress_con;
	sc_signal<sc_biguint<MDATA_WIDTH>, SC_MANY_WRITERS>		mdata_io_con;

	sc_signal<bool>				cs_con1;
	sc_signal<bool>				we_con1;
	sc_signal<sc_uint<MADDR_WIDTH> >		maddress_con1;
	sc_signal<sc_biguint<MDATA_WIDTH> >		mdata_io_con1;

	// between xfer_buffer and IFQ
	sc_signal<bool>				xfer_select_con;
	sc_signal<bool>				mwrite_enable_con;
	sc_signal<sc_uint<32> >		tbm_address_con;
	sc_signal<bool>				xfer_complete_con;

	// for stroageQ, afterwards, These signal inputs have to be modified to connections. 
	sc_signal<bool>				sq_select;
	sc_signal<sc_biguint<256>>	cmd_out;
	sc_signal<bool>				status_update_enable;
	sc_signal<sc_uint<8> >		cmdq_index;



	xfer_buffer 				iobuf("xfer_buffer");
	ram_dp_ar_aw				tbm("tbm");
	IF_QUEUE					ifq("IF_QUEUE");
	int i;
	sc_biguint<256>				tCmd;
	sc_uint<32>					tDword;

	tbm.cs_0(cs_con);
	tbm.we_0(we_con);
	tbm.data_0(mdata_io_con);
	tbm.address_0(maddress_con);
	tbm.cs_1(cs_con1);
	tbm.we_1(we_con1);
	tbm.data_1(mdata_io_con1);
	tbm.address_1(maddress_con1);

	iobuf.reset(reset);
	iobuf.clock_host(clock_host);
	iobuf.clock_fpga(clock_fpga);

	iobuf.host_select(host_select);
	iobuf.hwrite_enable(hwrite_enable);
	iobuf.buf_address(buf_address);
	iobuf.hostdata_inout(hostdata_inout);

	iobuf.gs_select(gs_select);
	iobuf.gs_write_enable(gs_write_enable);
	iobuf.gs_out_enable(gs_out_enable);
	iobuf.gs_out(gs_out);

	iobuf.xfer_buf_select(xfer_select_con);
	iobuf.mwrite_enable(mwrite_enable_con);
	iobuf.tbm_address(tbm_address_con);
	iobuf.xfer_complete(xfer_complete_con);

	iobuf.chip_select(cs_con);
	iobuf.write_enable(we_con);
	iobuf.maddress(maddress_con);
	iobuf.mdata_inout(mdata_io_con);

	ifq.reset(reset);
	ifq.clock_fpga(clock_fpga);
	ifq.clock_host(clock_host);
	ifq.xfer_buf_select(xfer_select_con);
	ifq.mwrite_enable(mwrite_enable_con);
	ifq.tbm_address(tbm_address_con);
	ifq.xfer_complete(xfer_complete_con);
	ifq.cmdq_select(cmdq_select);
	ifq.cmd_in(cmd_in);
	ifq.queryin_select(queryin_select);
	ifq.queryout_select(queryout_select);
	ifq.querydata_inout(querydata_inout);

	// TOBE changed. 
	ifq.sq_select(sq_select);
	ifq.cmd_out(cmd_out);
	ifq.status_update_enable(status_update_enable);
	ifq.cmdq_index(cmdq_index);

	sc_start(0, SC_NS);
	/*
	 * BSM WRITE
	 * 1. enqueue new command
	 * 2. get general write status
	 * 3. <FAKE READ>
	 * .....
	 * 4. enqueue query command for issued command
	 * 5. read query status...
	 */

	reset = 1;
	sc_start(1, SC_NS);
	reset = 0; 

	// 1. enqueue new command "BSM write operation
	tCmd.range((BASE_CMD + CMD_WIDTH - 1), BASE_CMD) = BSM_WRITE; // command: read or write
	tCmd.range((BASE_TAG + TAG_WIDTH - 1), BASE_TAG) = 0; // sequence number
	tCmd.range((BASE_VALID + VALID_WIDTH - 1), BASE_VALID) = 0x01; // valid bit 
	tCmd.range((BASE_RQSZ + RQSZ_WIDTH - 1), BASE_RQSZ) = 0x08; // # of sectors, 8

	cmdq_select = 1;
	cout << "tCmd:" << hex << tCmd << endl;

	for (i = 0; i < 8; i++)
	{
		tDword = tCmd.range(((i + 1) * 32 - 1), (i * 32));
		cmd_in = tDword;
		
		cout << "@" << sc_time_stamp() << ":tDword (4B) ? " << hex << tDword << endl;

		sc_start(0.5, SC_NS);
	}
	cmdq_select = 0;

	// 2. query general write status
	gs_select = 1;
	gs_write_enable = 1;

	sc_start(0.25, SC_NS);
	if (gs_out_enable == 1)
	{
		cout << "@" << sc_time_stamp() <<", general status:" << gs_out.read() << endl;
		if (gs_out.read() != 0)
		{
			host_select = 1;
			hwrite_enable = 1;
			buf_address = 0;
		
			for (i = 0; i < 1024; i++)
			{
				hostdata_inout = i;
				sc_start(0.5, SC_NS);
			}
		}
	}

	sc_start(100, SC_NS);

}

int sc_main(int argc, char**argv)
{
	// (tbm + xfer_buffer) test
//	test_case_1();

	test_case_2();

	return 0;

}
