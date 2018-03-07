/*************************************************************************
 *                          INTEL CONFIDENTIAL
 *                  Copyright 2018 Intel Corporation
 *
 * The source code contained or described herein and all documents related to
 * the source code ("Material") are owned by Intel Corporation or its
 * suppliers or licensors. Title to the Material remains with Intel
 * Corporation or its suppliers and licensors. The Material contains trade
 * secrets and proprietary and confidential information of Intel or its
 * suppliers and licensors. The Material is protected by worldwide copyright
 * and trade secret laws and treaty provisions. No part of the Material may
 * be used, copied, reproduced, modified, published, uploaded, posted,
 * transmitted, distributed, or disclosed in any way without Intel's prior
 * express written permission.
 *
 * No license under any patent, copyright, trade secret or other
 * intellectual property right is granted to or conferred upon you by
 * disclosure or delivery of the Materials, either expressly, by
 * implication, inducement, estoppel or otherwise. Any license under such
 * intellectual property rights must be express and approved by Intel in
 * writing.
 *************************************************************************/

/* cmos io device is used for android device reboot to bootloader or
 * recovery or normal boot usage
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <assert.h>

#include "inout.h"

#define CMOS_ADDR		0x74
#define CMOS_DATA		0x75
#define CMOS_BUF_SIZE		256

#define CMOS_NAME		"cmos_io"

/* #define CMOS_DEBUG */
#ifdef CMOS_DEBUG
static FILE * dbg_file;
#define DPRINTF(format, args...) \
do { fprintf(dbg_file, format, args); fflush(dbg_file); } while (0)
#else
#define DPRINTF(format, arg...)
#endif

/* cmos buffer used to store write/read contents,
 * and it should not be cleared when reboot
 */
static uint8_t cmos_buffer[CMOS_BUF_SIZE];

static int
cmos_io_handler(struct vmctx *ctx, int vcpu, int in, int port, int bytes,
				uint32_t *eax, void *arg)
{
	static int buf_offset;
	static int next_ops;  /* 0 for addr, 1 for data, in pair (addr,data)*/

	assert(port == CMOS_ADDR || port == CMOS_DATA);
	assert(bytes == 1);

#ifdef CMOS_DEBUG
	if (!dbg_file)
		dbg_file = fopen("/tmp/cmos_log", "a+");
#endif

	DPRINTF("%s port =0x%x, in=%d, size=%d, val=0x%x, ops=%d\n",
		__func__, port, in, bytes, (uint8_t)*eax, next_ops);

	if (port == CMOS_ADDR) {

		/* if port is addr, ops should be 0 */
		assert(next_ops == 0 && !in);
		if (next_ops != 0) {
			next_ops = 0;
			return -1;
		}

		buf_offset = (uint8_t)(*eax);
		next_ops = 1;

	} else if (port == CMOS_DATA) {

		assert(next_ops == 1);
		if (next_ops != 1) {
			next_ops = 0;
			return -1;
		}

		if (in)
			*eax = cmos_buffer[buf_offset];
		else
			cmos_buffer[buf_offset] = (uint8_t)*eax;

		next_ops = 0;
	}

	return 0;
}

INOUT_PORT(cmos_io, CMOS_ADDR, IOPORT_F_INOUT, cmos_io_handler);
INOUT_PORT(cmos_io, CMOS_DATA, IOPORT_F_INOUT, cmos_io_handler);
