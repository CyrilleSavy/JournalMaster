/*
 * REPTAR FPGA Leds emulation
 *
 * Copyright (c) 2013 Reconfigurable Embedded Digital Systems (REDS) Institute at HEIG-VD, Switzerland
 * Written by Romain Bornet <romain.bornet@heig-vd.ch>
 *
 * This module provides a basic emulation for the 8 buttons of REPTAR's FPGA board.
 *
 */

#include "reptar_sp6.h"
#include <stdlib.h>

sp6_state_t *sp6_leds_state;

uint8_t reptar_sp6_leds_read(void)
{
    return (uint8_t) sp6_leds_state->regs[SP6_LED>>1] ;
}

void reptar_sp6_leds_write(uint8_t value)
{
    cJSON *root;
    DBG("Led new value = %d\n",value);

    sp6_leds_state->regs[SP6_LED>>1] = (uint16_t) value;

    root = cJSON_CreateObject();
    cJSON_AddStringToObject(root,"perif", PERID_LED);
    cJSON_AddNumberToObject(root,"value", sp6_leds_state->regs[SP6_LED>>1]);

    sp6_emul_cmd_post(root);
}

int reptar_sp6_leds_init(void *opaque)
{
    sp6_leds_state = (sp6_state_t*) opaque;

    // Init de la valeur des leds
    sp6_leds_state->regs[SP6_LED>>1] = 0x0000;

    
    DBG("Led initialized!\n");
    return 0;
}
