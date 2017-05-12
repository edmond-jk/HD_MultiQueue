/*
 * tbm.h
 *
 *  Created on: May 5, 2017
 *      Author: jk
 */

#ifndef TBM_H_
#define TBM_H_

#include <systemc.h>
#include "hd_multiqueue.h"

#define RAM_DEPTH	1 << 10 // 32Byte x 1024 x 1024 --> 32Byte x 2^20 --> 32MB, TODO:

/*
 * cs: chip select
 * oe: output enable
 * we: write enable
 *
 * Herein, address is based on "DATA_WIDTH" unit. If using byte-addressing, modify.
 *
 */

SC_MODULE(ram_dp_ar_aw)
{
	sc_in<sc_uint<MADDR_WIDTH> >  		address_0;
	sc_in<sc_uint<MADDR_WIDTH> > 		address_1;
	sc_inout<sc_biguint<MDATA_WIDTH> > 	data_0;
	sc_inout<sc_biguint<MDATA_WIDTH> > 	data_1;
	sc_in<bool>	cs_0;
	sc_in<bool> we_0;
	sc_in<bool>	cs_1;
	sc_in<bool> we_1;

	sc_biguint<MDATA_WIDTH>	mem[RAM_DEPTH];

	void READ_0(void);
	void READ_1(void);
	void WRITE_0(void);
	void WRITE_1(void);

	SC_CTOR(ram_dp_ar_aw)
	{
		SC_METHOD (READ_0);
		sensitive << address_0 << cs_0 << we_0;
		SC_METHOD (READ_1);
		sensitive << address_1 << cs_1 << we_1;
		SC_METHOD (WRITE_0);
		sensitive << address_0 << cs_0 << we_0;
		SC_METHOD (WRITE_1);
		sensitive << address_1 << cs_1 << we_1;
	}
};

void ram_dp_ar_aw::READ_0(void)
{
	if (cs_0.read() && !we_0.read())
	{
		data_0 = mem[address_0.read()];
	}
}

void ram_dp_ar_aw::READ_1(void)
{
	if (cs_1.read() && !we_1.read())
	{
		data_1 = mem[address_1.read()];
	}
}

void ram_dp_ar_aw::WRITE_0(void)
{
	if (cs_0.read() && we_0.read())
	{
		mem[address_0.read()] = data_0.read();
	}
}

void ram_dp_ar_aw::WRITE_1(void)
{
	if (cs_1.read() && we_1.read())
	{
		mem[address_1.read()] = data_1.read();
	}
}

#endif /* TBM_H_ */
