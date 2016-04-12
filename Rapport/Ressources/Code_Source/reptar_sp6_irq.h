/*
 * reptar_sp6_irq.h
 *
 *  Created on: Apr 5, 2016
 *      Author: redsuser
 */

#ifndef REPTAR_SP6_IRQ_H_
#define REPTAR_SP6_IRQ_H_

uint8_t reptar_sp6_irq_read(void);

void reptar_sp6_irq_write(uint16_t value);

int reptar_sp6_irq_init(void *opaque);

void reptar_sp6_irq_button(uint8_t button);

#endif /* REPTAR_SP6_IRQ_H_ */
