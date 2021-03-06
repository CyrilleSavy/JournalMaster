/*******************************************************************
 * reptar_sp6_buttons.c
 *
 * Author: Romain Bornet (RBO)
 * Copyright (c) 2013 HEIG-VD, REDS Institute
 *******************************************************************/

#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <linux/gpio.h>

#include "reptar_sp6.h"

#if 1
#include <xen-guest/io/kbdif.h>
#include <xen-guest/console.h>

extern int omapfb_xen_switch_domain(domid_t dom);
#endif /* 0 */

static struct platform_device *reptar_sp6_btns;

typedef union {
	struct {
		uint16_t irq_clear :1;
		uint16_t irq_buttons :3;
		uint16_t irq_status :1;
		uint16_t irq_source :2;
		uint16_t irq_enable :1;
		uint16_t irq_reserved :8;
	};
	uint16_t irq_reg_byte;
} sp6_irq_reg_t;

struct reptar_sp6_buttons {
	struct reptar_sp6_buttons_platdata *pdata;
	int irq;
	uint16_t *btns_reg;
	sp6_irq_reg_t *irq_reg;
	struct input_dev *input;
	uint16_t current_button;
};

/* Hard/immediate interrupt handler */
static irqreturn_t reptar_sp6_buttons_irq(int irq, void *dev_id) {
	struct reptar_sp6_buttons *dev = (struct reptar_sp6_buttons *) dev_id;

	/* to be completed */
	dev->current_button = (*dev->btns_reg & 0x00FF);

	printk("Button interrupt!\n");

	//Clear the interrupt in the FPGA
	dev->irq_reg->irq_clear = 1;

	return IRQ_WAKE_THREAD;
}

/* Threaded/deferred interrupt handler */
static irqreturn_t reptar_sp6_buttons_irq_thread(int irq, void *dev_id) {
	struct reptar_sp6_buttons *dev = (struct reptar_sp6_buttons *) dev_id;
	struct input_dev *input = dev->input;
	int pressed;
	unsigned int key;

	do {
		pressed = fls(dev->current_button);
		printk("Current buttons = %02x, pressed=%d\n", dev->current_button,
				pressed);

		if (!pressed)
			return IRQ_HANDLED;

		key = dev->pdata->keys[pressed - 1];

		/* Report key press and release */
		input_report_key(input, key, 1);
		input_sync(input);

		input_report_key(input, key, 0);
		input_sync(input);

		dev->current_button &= ~(1 << (pressed - 1));

	} while (dev->current_button);

	return IRQ_HANDLED;
}

static int reptar_sp6_buttons_probe(struct platform_device *pdev) {
	struct reptar_sp6_buttons_platdata *pdata = pdev->dev.platform_data;
	struct reptar_sp6_buttons *btns;
	struct input_dev *input;
	struct platform_device *fpga_pdev;
	struct resource *res;
	int ret;
	int gpio;
	int i;

	/* Low-level initialization */
	btns = kzalloc(sizeof(struct reptar_sp6_buttons), GFP_KERNEL);
	if (btns == NULL) {
		dev_err(&pdev->dev, "No memory for device\n");
		return -ENOMEM;
	}

	platform_set_drvdata(pdev, btns);

	btns->pdata = pdata;

	fpga_pdev = to_platform_device(pdev->dev.parent);

	if (!fpga_pdev) {
		printk("FAILED to get parent device\n");
		return -1;
	}

	res = platform_get_resource(fpga_pdev, IORESOURCE_MEM, 0);

	btns->btns_reg = ioremap(res->start + pdata->btns_reg_offset,
			sizeof(uint16_t));
	btns->irq_reg = ioremap(res->start + pdata->irq_reg_offset,
			sizeof(uint16_t));

	//check the ioremap
	if (NULL == btns->btns_reg || NULL == btns->irq_reg) {
		return -1;
	}

	gpio = platform_get_irq(fpga_pdev, 0);
	btns->irq = gpio_to_irq(gpio);

	if (gpio_request_one(gpio, GPIOF_IN, "sp6_irq")) {
		dev_err(&pdev->dev, "Failed to alloc SP6 IRQ GPIO\n");
		return -1;
	}

	/* Register 2 interrupt handlers (top, bottom) */
	/* to be completed */
	/* ... */
	ret = request_threaded_irq(btns->irq, reptar_sp6_buttons_irq,
			reptar_sp6_buttons_irq_thread, IRQF_TRIGGER_RISING, NULL, btns);

	if (ret < 0) {
		dev_err(&pdev->dev, "failed to allocate IRQ\n");
		return ret;
	}

	//enable the interrupt on the FPGA
	btns->irq_reg->irq_enable = 1;

	platform_set_drvdata(pdev, btns);

	/* Registration as input device */

	input = input_allocate_device();
	if (!input) {
		dev_err(&pdev->dev, "failed to allocate input device\n");
		ret = -ENOMEM;
		return ret;
	}

	btns->input = input;
	input_set_drvdata(input, btns);

	input->name = pdev->name;
	input->dev.parent = &pdev->dev;

	for (i = 0; i < 8; i++)
		input_set_capability(input, EV_KEY, pdata->keys[i]);

	ret = input_register_device(input);
	if (ret) {
		dev_err(&pdev->dev, "Unable to register input device, error: %d\n",
				ret);
		return ret;
	}

	// Last driver init
	btns->current_button = 0x00;

	return 0;
}

static int reptar_sp6_buttons_remove(struct platform_device *pdev) {

	struct reptar_sp6_buttons *btns;

	btns = platform_get_drvdata(pdev);

	disable_irq(btns->irq);

	input_unregister_device(btns->input);
	input_free_device(btns->input);

	gpio_free(irq_to_gpio(btns->irq));

	kfree(btns);

	return 0;
}

static struct platform_driver reptar_sp6_buttons_driver = { .probe =
		reptar_sp6_buttons_probe, .remove = reptar_sp6_buttons_remove, .driver =
		{ .name = "reptar_sp6_buttons", .owner = THIS_MODULE, }, };

//#define __init
//#define __exit

/* static int __init reptar_sp6_buttons_init(void) */
int __init reptar_sp6_buttons_init(struct platform_device *parent_fpga) {
	reptar_sp6_btns = platform_device_alloc("reptar_sp6_buttons", -1);
	platform_device_add_data(reptar_sp6_btns, &reptar_sp6_btns_pdata,
			sizeof(reptar_sp6_btns_pdata));

	/* Set link to parent device */
	reptar_sp6_btns->dev.parent = &(parent_fpga->dev);

	platform_device_add(reptar_sp6_btns);

	return platform_driver_register(&reptar_sp6_buttons_driver);
}

/* static void __exit reptar_sp6_buttons_exit(void) */
void __exit reptar_sp6_buttons_exit(void) {
	platform_device_unregister(reptar_sp6_btns);
	platform_driver_unregister(&reptar_sp6_buttons_driver);
}

