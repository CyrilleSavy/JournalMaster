/*
 * REPTAR Spartan6 FPGA emulation
 *
 * Copyright (c) 2013-2014 HEIG-VD / REDS
 * Written by Romain Bornet
 *
 * This code is licensed under the GPL.
 */

#include "qemu-common.h"
#include "reptar_sp6.h"
#include "hw/sysbus.h"

#include "cJSON.h"

static uint64_t sp6_read(void *opaque, hwaddr addr, unsigned size) {
	uint64_t ret_value = 0;

	// Récupération de la structure d'état
	sp6_state_t *sp6_dev_struct_ptr = (sp6_state_t*) opaque;

	switch ((uint8_t) addr) {
	case SP6_IRQ_STATUS:
		break;
	case SP6_PUSH_BUT:
		ret_value = reptar_sp6_btns_read();
		break;
	case SP6_IRQ_CTL:
		ret_value = reptar_sp6_irq_read();
		break;
	case SP6_7SEG1:
		reptar_sp6_7segs_read(SP6_7SEG1);
		break;
	case SP6_7SEG2:
		ret_value = reptar_sp6_7segs_read(SP6_7SEG2);
		break;
	case SP6_7SEG3:
		ret_value = reptar_sp6_7segs_read(SP6_7SEG3);
		break;
	case SP6_LCD_CONTROL:
	case SP6_LCD_STATUS:
		DBG("sp6_read %08x bytes @%08x\n", (uint32_t )size, (uint32_t )addr);
		break;
	case SP6_LED:
		// TODO, call leds implementation
		ret_value = reptar_sp6_leds_read();
		break;
	default:
		DBG("Error, no valid register address!\n");
		break;
	}

	return (uint64_t) ret_value;
}

static void sp6_write(void *opaque, hwaddr addr, uint64_t value, unsigned size) {
	// Récupération de la structure d'état
	sp6_state_t *sp6_dev_struct_ptr = (sp6_state_t*) opaque;

	switch ((uint8_t) addr) {
	case SP6_IRQ_STATUS:
	case SP6_PUSH_BUT:
		break;
	case SP6_IRQ_CTL:
		reptar_sp6_irq_write((uint8_t) value);
		break;
	case SP6_7SEG1:
		reptar_sp6_7segs_write((uint8_t) value, SP6_7SEG1);
		break;
	case SP6_7SEG2:
		reptar_sp6_7segs_write((uint8_t) value, SP6_7SEG2);
		break;
	case SP6_7SEG3:
		reptar_sp6_7segs_write((uint8_t) value, SP6_7SEG3);
		break;
	case SP6_LCD_CONTROL:
	case SP6_LCD_STATUS:
		DBG("sp6_write %08x bytes, value = %08x, @%08x\n", (uint32_t )size,
				(uint32_t )value, (uint32_t )addr);
		break;
	case SP6_LED:
		// TODO, call leds implementation
		reptar_sp6_leds_write((uint8_t) value);
		break;
	default:
		DBG("Error, no valid register address!\n");
		break;
	}

}

static const MemoryRegionOps sp6_ops = { .read = sp6_read, .write = sp6_write,
		.endianness = DEVICE_NATIVE_ENDIAN, };

static int sp6_initfn(SysBusDevice *sbd) {
	DeviceState *dev = DEVICE(sbd);
	sp6_state_t *s = OBJECT_CHECK(sp6_state_t, dev, "reptar_sp6");

	memory_region_init_io(&s->iomem, OBJECT(s), &sp6_ops, s, "reptar_sp6",
			0x1000);
	sysbus_init_mmio(sbd, &s->iomem);

	memset(&s->regs, 0, sizeof(s->regs));

	sysbus_init_irq(sbd, &s->irq);

//    s->irq_pending = 0;
//    s->irq_enabled = 0;

	sp6_emul_init();
	reptar_sp6_leds_init(s);
	reptar_sp6_btns_init(s);
	reptar_sp6_irq_init(s);
	reptar_sp6_7segs_init(s);
	//reptar_clcd_init();

	DBG("sp6_initfn : initialization de la FPGA...\n");

	return 0;
}

static void sp6_class_init(ObjectClass *klass, void *data) {
	SysBusDeviceClass *k = SYS_BUS_DEVICE_CLASS(klass);

	k->init = sp6_initfn;
}

static void sp6_init(Object *obj) {
	// do nothing...
}

static const TypeInfo reptar_sp6_info = { .name = "reptar_sp6", .parent =
		TYPE_SYS_BUS_DEVICE, .instance_size = sizeof(sp6_state_t),
		.instance_init = sp6_init, .class_init = sp6_class_init, };

static void sp6_register_types(void) {
	type_register_static(&reptar_sp6_info);
}

type_init(sp6_register_types)
