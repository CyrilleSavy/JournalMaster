/*
 * (C) Copyright 2000
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <command.h>
#include <asm/arch/mux.h>
#include <asm/arch/gpio.h>
#include <asm/io.h>
#include <asm/errno.h>
#include "board/ti/reptar/reptar.h"
#include <asm/gpio.h>

#define NB_SWITCHS 	5
#define NB_LEDS 	4

int main(int argc, char *argv[]) {
	printf("Start of the GPIO U-boot Standalone Application\n");

	/* Donnee de l'exercice : on considere que u-boot initialise correctement les io
	 * si ce n'etait pas le cas, on devrait aussi utiliser les fonctions suivantes :

	 int gpio_request(int gpio, const char *label);
	 int gpio_direction_input(unsigned gpio);
	 int gpio_direction_output(unsigned gpio, int value);
	  void gpio_free(unsigned gpio);
	 */

	int end = 0;
	while (end == 0) {

		//switchs et leds 0..3
		int value = gpio_get_value(GPIO_SW_0);
		gpio_set_value(GPIO_LED_0, value);
		value = gpio_get_value(GPIO_SW_1);
		gpio_set_value(GPIO_LED_1, value);
		value = gpio_get_value(GPIO_SW_2);
		gpio_set_value(GPIO_LED_2, value);
		value = gpio_get_value(GPIO_SW_3);
		gpio_set_value(GPIO_LED_3, value);

		//switch 4
		end = gpio_get_value(GPIO_SW_4);
	}

	printf("Stop of the GPIO U-boot Standalone Application\n");

	return (0);
}
