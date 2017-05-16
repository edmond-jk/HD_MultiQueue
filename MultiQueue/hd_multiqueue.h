/*
 * hd_multiqueue.h
 *
 *  Created on: May 8, 2017
 *      Author: jk
 */

#ifndef HD_MULTIQUEUE_H_
#define HD_MULTIQUEUE_H_

#define HDATA_WIDTH					32  // bits
#define MDATA_WIDTH					256 // Bits
#define DATA_WIDTH_DIFF				8	// MDATA_WIDTH / HDATA_WIDTH
#define BUF_QUEUE_DEPTH 			4	// 
#define BUF_SIZE					4096 // Bytes 
#define NUM_DATA_WIDTH_FOR_XFER_BUF	512 // BUF_SIZE / MDATA_WIDTH x BUF_QUEUE_DEPTH)
#define NUM_DATA_WIDTH_PER_BUF		128 // BUF_SIZE / MDATA_WIDTH

#define MADDR_WIDTH					32 // bits TODO: should be modified

#define MAX_CMDQ_DEPTH				32
#define MAX_BUF_PER_CMD				8 	//number of 16KB buffers

// CMDQ Status Transition
#define ST_FREE						0x00
#define	ST_QUEUED2D					0x01
#define	ST_XFER2D					0x02
#define ST_DONEB					0x03
#define ST_QUERIED					0x04
#define ST_DONED					0x05
#define ST_WAITS					0x06
#define ST_XFER2H					0x07

#define ST_QUEUED2S					0x10
#define ST_ISSUED					0x20
#define	ST_DONES					0x30

// Command Set
#define BSM_READ					0x30
#define BSM_WRITE					0x40

// Bit field for a HD command descriptor.
#define BASE_ADDR					128 	//from 128 bit position
#define ADDR_WIDTH					16		//every 16 bit

#define BASE_STATUS					16
#define STATUS_WIDTH				8

#define BASE_VALID					120
#define VALID_WIDTH					8

#define BASE_CMD					0
#define CMD_WIDTH					8

#define BASE_TAG					8
#define TAG_WIDTH					8

#define BASE_RQSZ					96
#define RQSZ_WIDTH					16 		// max. request size per buffer --> 32 MB

#define BASE_INDEX					24
#define INDEX_WIDTH					8
/*
 * Query Command Status
 */
// Estimated completion time [7:4]

// Query out information [3:2]
#define	QS_NOCMD					00
#define QS_INQUEUE					01
#define QS_INPROCESS				10
#define QS_DONE						11

// Command Sync Counter [1:0]

/*
 * General Write Status -- 1 Byte
 */
// Checksum error [7:3]

// Available 4KB buffers for a write operation [2:0]
#define GWAB_0						000
#define GWAB_1						001
#define GWAB_2						010
#define GWAB_3						011
#define GWAB_4						100

/*
 * General Read Status -- 1 Byte
 */
// Reserved [7:3]
// Available 4KB buffers for a read operation [2:0]
#define GRAB_0						000
#define GRAB_1						001
#define GRAB_2						010
#define GRAB_3						011
#define GRAB_4						100


#endif /* HD_MULTIQUEUE_H_ */
