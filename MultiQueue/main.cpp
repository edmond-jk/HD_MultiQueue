#include "tbm.h"
#include "xfer_buffer.h"

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

	gs_select = 1;
	gs_write_enable = 1;

	for (i = 0; i < 10; i++)
	{
		sc_start(0.25, SC_NS);
		if (gs_out_enable == 1)
		{
			cout << "@" << sc_time_stamp() <<", general status:" << gs_out << endl;
		}
	}
}

int sc_main(int argc, char**argv)
{
	// tbm test
//	test_case_0();

	// xfer_buffer test
	test_case_1();

	return 0;

}
