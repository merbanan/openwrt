// SPDX-License-Identifier: GPL-2.0+

/*
 * Airoha UART driver
 *
 * Copyright (c) 2025 Genexis Sweden AB
 * Author: Benjamin Larsson <benjamin.larsson@genexis.eu>
 */

/* The Airoha UART is 16550-compatible except for the baud rate calculation. */

#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/serial_reg.h>
#include <linux/serial_8250.h>

#include "8250.h"

struct airoha_uart {
	int line;
	int type;
};

/* Airoha UART registers */
#define UART_AIROHA_BRDL	0
#define UART_AIROHA_BRDH	1
#define UART_AIROHA_XINCLKDR	10
#define UART_AIROHA_XYD		11

#define XYD_Y 65000
#define XINDIV_CLOCK 20000000
#define UART_BRDL_20M 0x01
#define UART_BRDH_20M 0x00

static const int clock_div_tab[] = { 10, 4, 2};
static const int clock_div_reg[] = {  4, 2, 1};

/**
 * airoha8250_set_baud_rate() - baud rate calculation routine
 * @port: uart port
 * @baud: requested uart baud rate
 * @hs: uart type selector, 0 for regular uart and 1 for high-speed uart
 *
 * crystal_clock = 20 MHz (fixed frequency)
 * xindiv_clock = crystal_clock / clock_div
 * (x/y) = XYD, 32 bit register with 16 bits of x and then 16 bits of y
 * clock_div = XINCLK_DIVCNT (default set to 10 (0x4)),
 *           - 3 bit register [ 1, 2, 4, 8, 10, 12, 16, 20 ]
 *
 * baud_rate = ((xindiv_clock) * (x/y)) / ([BRDH,BRDL] * 16)
 *
 * Selecting divider needs to fulfill
 * 1.8432 MHz <= xindiv_clk <= APB clock / 2
 * The clocks are unknown but a divider of value 1 did not result in a valid
 * waveform.
 *
 * XYD_y seems to need to be larger then XYD_x for proper waveform generation.
 * Setting [BRDH,BRDL] to [0,1] and XYD_y to 65000 gives even values
 * for usual baud rates.
 */

static void airoha8250_set_baud_rate(struct uart_port *port,
			     unsigned int baud, unsigned int hs)
{
	struct uart_8250_port *up = up_to_u8250p(port);
	unsigned int xyd_x, nom, denom;
	int i;

	/* set DLAB to access the baud rate divider registers (BRDH, BRDL) */
	serial_port_out(port, UART_LCR, up->lcr | UART_LCR_DLAB);
	/* set baud rate calculation defaults */
	/* set BRDIV ([BRDH,BRDL]) to 1 */
	serial_port_out(port, UART_AIROHA_BRDL, UART_BRDL_20M);
	serial_port_out(port, UART_AIROHA_BRDH, UART_BRDH_20M);
	/* calculate XYD_x and XINCLKDR register by searching
	 * through a table of crystal_clock divisors
	 *
	 * for the HSUART xyd_x needs to be scaled by a factor of 2
	 */
	for (i = 0 ; i < ARRAY_SIZE(clock_div_tab) ; i++) {
		denom = (XINDIV_CLOCK/40) / clock_div_tab[i];
		nom = baud * (XYD_Y/40);
		xyd_x = ((nom/denom) << 4) >> hs;
		if (xyd_x < XYD_Y)
			break;
	}
	serial_port_out(port, UART_AIROHA_XINCLKDR, clock_div_reg[i]);
	serial_port_out(port, UART_AIROHA_XYD, (xyd_x<<16) | XYD_Y);
	/* unset DLAB */
	serial_port_out(port, UART_LCR, up->lcr);
}

static void airoha_set_termios(struct uart_port *port, struct ktermios *termios,
		          const struct ktermios *old)
{
	unsigned int baud;
	baud = serial8250_get_baud_rate(port, termios, old);
	serial8250_do_set_termios(port, termios, old);

	if (port->type == PORT_AIROHA)
		airoha8250_set_baud_rate(port, baud, 0);
	if (port->type == PORT_AIROHA_HS)
		airoha8250_set_baud_rate(port, baud, 1);
}

static int airoha_uart_probe(struct platform_device *ofdev)
{
	struct uart_8250_port port8250;
	struct airoha_uart *uart;
	unsigned int port_type;
	struct device *dev = &ofdev->dev;
	struct device_node *np = dev->of_node;
	struct uart_port *port;
	struct resource resource;
	int ret, irq;

	uart = devm_kzalloc(&ofdev->dev, sizeof(*uart), GFP_KERNEL);
	if (!uart)
		return -ENOMEM;

	memset(&port8250, 0, sizeof(port8250));

	port_type = (unsigned long)of_device_get_match_data(&ofdev->dev);
	if (port_type == PORT_UNKNOWN)
		return -EINVAL;
	
	port = &port8250.port;
	spin_lock_init(&port->lock);

	port->flags = UPF_BOOT_AUTOCONF | UPF_FIXED_PORT | UPF_FIXED_TYPE;
	port->dev = &ofdev->dev;
	port->set_termios = airoha_set_termios;

	ret = of_address_to_resource(np, 0, &resource);
	if (ret) {
		dev_err_probe(dev, ret, "invalid address\n");
		return ret;
	}
	irq = platform_get_irq(ofdev, 0);
	if (irq < 0)
		return irq;

	port->irq = irq;
	port->mapbase = resource.start;
	port->mapsize = resource_size(&resource);
	port->flags |= UPF_IOREMAP;

	ret = uart_read_and_validate_port_properties(port);
	if (ret)
		return ret;

	ret = serial8250_register_8250_port(&port8250);
	if (ret < 0)
		return ret;

	platform_set_drvdata(ofdev, uart);
	uart->line = ret;

	return 0;
}

static void airoha_uart_remove(struct platform_device *ofdev)
{
	struct airoha_uart *uart = platform_get_drvdata(ofdev);

	serial8250_unregister_port(uart->line);
}
		
static const struct of_device_id airoha_uart_of_match[] = {
	{ .compatible = "airoha,airoha-uart", .data = (void *)PORT_AIROHA, },
	{ .compatible = "airoha,airoha-hsuart", .data = (void *)PORT_AIROHA_HS, },	{ },
};
MODULE_DEVICE_TABLE(of, airoha_uart_of_match);


static struct platform_driver airoha_uart_driver = {
	.driver = {
		.name = "airoha-uart",
		.of_match_table = airoha_uart_of_match,
	},
	.probe = airoha_uart_probe,
	.remove = airoha_uart_remove,
};

module_platform_driver(airoha_uart_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Handling of Airoha specific 8250 variants");
MODULE_AUTHOR("Benjamin Larsson <benjamin.larsson@genexis.eu>");
