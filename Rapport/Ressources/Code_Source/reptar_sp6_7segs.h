/*
 * reptar_sp6_7segs.h
 *
 *  Created on: Apr 12, 2016
 *      Author: redsuser
 */

#ifndef REPTAR_SP6_7SEGS_H_
#define REPTAR_SP6_7SEGS_H_

#include "cJSON.h"

typedef struct SEGState
    {

	uint8_t state;

    } SEGState_t;

uint8_t reptar_sp6_7segs_read(int segNb);
void reptar_sp6_7segs_write(uint8_t value, int segNb);
int reptar_sp6_7segs_init(void *opaque);

#endif /* REPTAR_SP6_7SEGS_H_ */
