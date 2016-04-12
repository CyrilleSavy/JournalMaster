/*
 * reptar_sp6_irq.c
 *
 *  Created on: Apr 5, 2016
 *      Author: redsuser
 */

#include "reptar_sp6.h"
#include <stdlib.h>

#define IRQ_SOURCE_BUTTON 0
#define IRQ_SOURCE_LBA 1
#define IRQ_SOURCE_LBS 2

sp6_state_t *sp6_irq_state;

typedef union irq_ctl_reg_t
    {

	uint16_t irq_ctl_value;
	struct
	    {
		uint16_t IRQ_CLEAR :1;
		uint16_t IRQ_BUTTON :3;
		uint16_t IRQ_STATUS :1;
		uint16_t IRQ_SOURCE :2;
		uint16_t IRQ_ENABLE :1;
		uint16_t reserved :8;

	    } irq_ctl_bits;
    } irq_ctl_reg_t;

static irq_ctl_reg_t *irq_ctl_reg;

uint8_t reptar_sp6_irq_read(void)
    {
    return (uint8_t) sp6_irq_state->regs[SP6_IRQ_CTL >> 1];
    }

void reptar_sp6_irq_write(uint16_t value)
    {
    irq_ctl_reg_t *aVal = &value;

    irq_ctl_reg->irq_ctl_bits.IRQ_ENABLE = aVal->irq_ctl_bits.IRQ_ENABLE;

    if (aVal->irq_ctl_bits.IRQ_CLEAR)
	{
	irq_ctl_reg->irq_ctl_bits.IRQ_STATUS = 0;
	DBG("reptar_sp6_irq_button : lowering an irq\n");
	qemu_irq_lower(sp6_irq_state->irq);
	}
    }

int reptar_sp6_irq_init(void *opaque)
    {
    sp6_irq_state = (sp6_state_t*) opaque;

    // Init de la valeur des leds
    sp6_irq_state->regs[SP6_IRQ_CTL >> 1] = 0x0000;

    irq_ctl_reg = (irq_ctl_reg_t*) &(sp6_irq_state->regs[SP6_IRQ_CTL >> 1]);

    DBG("IRQ initialized!\n");
    return 0;
    }

void reptar_sp6_irq_button(uint8_t button)
    {
    DBG("Button : %d\n",button);
    DBG("Irq enabled : %d\n",irq_ctl_reg->irq_ctl_bits.IRQ_ENABLE);
    DBG("Irq status : %d\n",irq_ctl_reg->irq_ctl_bits.IRQ_STATUS);

    if (irq_ctl_reg->irq_ctl_bits.IRQ_ENABLE
	    && !irq_ctl_reg->irq_ctl_bits.IRQ_STATUS)
	{
	irq_ctl_reg->irq_ctl_bits.IRQ_STATUS = 1;
	irq_ctl_reg->irq_ctl_bits.IRQ_SOURCE = IRQ_SOURCE_BUTTON;
	irq_ctl_reg->irq_ctl_bits.IRQ_BUTTON = button;

	DBG("reptar_sp6_irq_button : raising an irq\n");
	qemu_irq_raise(sp6_irq_state->irq);
	}
    }
