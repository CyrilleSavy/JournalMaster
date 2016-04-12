/*
 * REPTAR FPGA buttons emulation
 *
 * Copyright (c) 2013 Reconfigurable Embedded Digital Systems (REDS) Institute at HEIG-VD, Switzerland
 * Written by Romain Bornet <romain.bornet@heig-vd.ch>
 *
 * This module provides a basic emulation for the 8 buttons of REPTAR's FPGA board.
 *
 */

#include "reptar_sp6.h"

sp6_state_t *sp6_state;

static uint8_t OldBtnValues = 0;

int reptar_sp6_btns_event_process(cJSON * object)
    {
    //read JSON and write buttons value in regs
    uint16_t btnValues = cJSON_GetObjectItem(object, "status")->valueint;
    cJSON_Delete(object);
    DBG("0x%x\n", btnValues);
    sp6_state->regs[SP6_PUSH_BUT >> 1] = btnValues;

    uint8_t buttonChanged = (uint8_t) btnValues ^ OldBtnValues;

    buttonChanged = (buttonChanged & 0x01) ? 0 : (buttonChanged & 0x02) ? 1 :
		    (buttonChanged & 0x04) ? 2 : (buttonChanged & 0x08) ? 3 :
		    (buttonChanged & 0x10) ? 4 : (buttonChanged & 0x20) ? 5 :
		    (buttonChanged & 0x40) ? 6 : (buttonChanged & 0x80) ? 7 : 0;

    OldBtnValues = btnValues;

    reptar_sp6_irq_button(buttonChanged);
    return 0;
    }

uint16_t reptar_sp6_btns_read(void)
    {
    //Read regs
    return sp6_state->regs[SP6_PUSH_BUT >> 1];
    }

int reptar_sp6_btns_init(void *opaque)
    {
    sp6_state = (sp6_state_t *) opaque;

    return 0;
    }
