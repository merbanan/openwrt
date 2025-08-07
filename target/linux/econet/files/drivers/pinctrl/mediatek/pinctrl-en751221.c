// SPDX-License-Identifier: GPL-2.0-only
/*
 * Author: Lorenzo Bianconi <lorenzo@kernel.org>
 * Author: Benjamin Larsson <benjamin.larsson@genexis.eu>
 * Author: Markus Gothe <markus.gothe@genexis.eu>
 */

#include <dt-bindings/pinctrl/mt65xx.h>
#include <linux/bits.h>
#include <linux/cleanup.h>
#include <linux/gpio/driver.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/irqdomain.h>
#include <linux/mfd/syscon.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/pinctrl/pinconf-generic.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>

#include "../core.h"
#include "../pinconf.h"
#include "../pinmux.h"

#define PINCTRL_PIN_GROUP(id)						\
	PINCTRL_PINGROUP(#id, id##_pins, ARRAY_SIZE(id##_pins))

#define PINCTRL_FUNC_DESC(id)						\
	{								\
		.desc = {						\
			.func = {					\
				.name = #id,				\
				.groups = id##_groups,			\
				.ngroups = ARRAY_SIZE(id##_groups),	\
			}						\
		},							\
		.groups = id##_func_group,				\
		.group_size = ARRAY_SIZE(id##_func_group),		\
	}

#define PINCTRL_CONF_DESC(p, offset, mask)				\
	{								\
		.pin = p,						\
		.reg = { offset, mask },				\
	}

/* MUX */
#define REG_I2C_MODE				0x0104
#define GPIO_DSL_I2C_MODE_MASK			BIT(24)
#define SIPO_RCLK_MODE_MASK			BIT(23)
#define DMT_TOD_1PPS_MODE_MASK			BIT(22)
#define PCIE_RESET1_GPIO_MODE_MASK		BIT(21)
#define PCIE_RESET0_GPIO_MODE_MASK		BIT(20)
#define GPIO_SPI_QUAD_MODE_MASK			BIT(19)
#define GPIO_UART2_MODE_MASK			BIT(18)
#define SIPO_MODE_MASK				BIT(17)
#define DGASP_OUT_MODE_MASK			BIT(16)
#define GPIO_PON_MODE_MASK			BIT(15)
#define GPIO_PCM2_MODE_MASK			BIT(14)
#define GPIO_PCM1_MODE_MASK			BIT(13)
#define GPIO_SPI2_MODE_MASK			BIT(12)
#define GPIO_PCM_INT_MODE_MASK			BIT(11)
#define GPIO_PCM_RESET_MODE_MASK		BIT(10)
#define GPIO_SPI_CS4_MODE_MASK			BIT(9)
#define GPIO_SPI_CS3_MODE_MASK			BIT(8)
#define GPIO_GE_LED_MASK_MASK			BIT(7)
#define GPIO_LAN3_LED_MODE_MASK			BIT(6)
#define GPIO_LAN2_LED_MODE_MASK			BIT(5)
#define GPIO_LAN1_LED_MODE_MASK			BIT(4)
#define GPIO_LAN0_LED_MODE_MASK			BIT(3)
#define PON_TOD_1PPS_MODE_MASK			BIT(2)
#define GSW_TOD_1PPS_MODE_MASK			BIT(1)
#define PON_I2C_MODE_MASK			BIT(0)

#define REG_CPU_EJTAG_EN			0x150
#define CPU_EJTAG_EN_MASK			BIT(1)

/* LED MAP */
#define REG_LAN_LED0_MAPPING			0x0154

#define LAN4_LED_MAPPING_MASK			GENMASK(18, 16)
#define LAN4_PHY4_LED_MAP			BIT(18)
#define LAN4_PHY2_LED_MAP			BIT(17)
#define LAN4_PHY1_LED_MAP			BIT(16)
#define LAN4_PHY0_LED_MAP			0
#define LAN4_PHY3_LED_MAP			GENMASK(17, 16)

#define LAN3_LED_MAPPING_MASK			GENMASK(14, 12)
#define LAN3_PHY4_LED_MAP			BIT(14)
#define LAN3_PHY2_LED_MAP			BIT(13)
#define LAN3_PHY1_LED_MAP			BIT(12)
#define LAN3_PHY0_LED_MAP			0
#define LAN3_PHY3_LED_MAP			GENMASK(13, 12)

#define LAN2_LED_MAPPING_MASK			GENMASK(10, 8)
#define LAN2_PHY4_LED_MAP			BIT(12)
#define LAN2_PHY2_LED_MAP			BIT(11)
#define LAN2_PHY1_LED_MAP			BIT(10)
#define LAN2_PHY0_LED_MAP			0
#define LAN2_PHY3_LED_MAP			GENMASK(11, 10)

#define LAN1_LED_MAPPING_MASK			GENMASK(6, 4)
#define LAN1_PHY4_LED_MAP			BIT(6)
#define LAN1_PHY2_LED_MAP			BIT(5)
#define LAN1_PHY1_LED_MAP			BIT(4)
#define LAN1_PHY0_LED_MAP			0
#define LAN1_PHY3_LED_MAP			GENMASK(5, 4)

#define LAN0_LED_MAPPING_MASK			GENMASK(2, 0)
#define LAN0_PHY4_LED_MAP			BIT(3)
#define LAN0_PHY2_LED_MAP			BIT(2)
#define LAN0_PHY1_LED_MAP			BIT(1)
#define LAN0_PHY0_LED_MAP			0
#define LAN0_PHY3_LED_MAP			GENMASK(2, 1)

/* CONF */
#define REG_I2C_SDA_E4				0x0010
#define SPI_MISO_E4_MASK			BIT(11)
#define SPI_MOSI_E4_MASK			BIT(10)
#define SPI_CLK_E4_MASK				BIT(9)
#define SPI_CS_E4_MASK				BIT(8)
#define PCIE1_RESET_E4_MASK			BIT(7)
#define PCIE0_RESET_E4_MASK			BIT(6)
#define MDIO_E4_MASK				BIT(5)
#define MDC_E4_MASK				BIT(4)
#define UART1_RXD_E4_MASK			BIT(3)
#define UART1_TXD_E4_MASK			BIT(2)
#define I2C_SCL_E4_MASK				BIT(1)
#define I2C_SDA_E4_MASK				BIT(0)

#define REG_I2C_SDA_E8				0x0014
#define SPI_MISO_E8_MASK			BIT(11)
#define SPI_MOSI_E8_MASK			BIT(10)
#define SPI_CLK_E8_MASK				BIT(9)
#define SPI_CS_E8_MASK				BIT(8)
#define PCIE1_RESET_E8_MASK			BIT(7)
#define PCIE0_RESET_E8_MASK			BIT(6)
#define MDIO_E8_MASK				BIT(5)
#define MDC_E8_MASK				BIT(4)
#define UART1_RXD_E8_MASK			BIT(3)
#define UART1_TXD_E8_MASK			BIT(2)
#define I2C_SCL_E8_MASK				BIT(1)
#define I2C_SDA_E8_MASK				BIT(0)

#define REG_GPIO_E4				0x0018
#define REG_GPIO_E8				0x001c

#define REG_SIMCLK_E2				0x0020
#define SIMDATA_E4_MASK				BIT(10)
#define SIMRST_E4_MASK				BIT(9)
#define SIMCLK_E4_MASK				BIT(8)
#define SIMDATA_E2_MASK				BIT(2)
#define SIMRST_E2_MASK				BIT(1)
#define SIMCLK_E2_MASK				BIT(0)

#define REG_RGMII_TXCTL_E4			0x0024
#define TMII_TXC_E4_MASK			BIT(12)
#define RGMII_RXD3_E4_MASK			BIT(11)
#define RGMII_RXD2_E4_MASK			BIT(10)
#define RGMII_RXD1_E4_MASK			BIT(9)
#define RGMII_RXD0_E4_MASK			BIT(8)
#define RGMII_RXC_E4_MASK			BIT(7)
#define RGMII_RXCTL_E4_MASK			BIT(6)
#define RGMII_TXD3_E4_MASK			BIT(5)
#define RGMII_TXD2_E4_MASK			BIT(4)
#define RGMII_TXD1_E4_MASK			BIT(3)
#define RGMII_TXD0_E4_MASK			BIT(2)
#define RGMII_TXC_E4_MASK			BIT(1)
#define RGMII_TXCTL_E4_MASK			BIT(0)

#define REG_RGMII_TXCTL_E8			0x0028
#define TMII_TXC_E8_MASK			BIT(12)
#define RGMII_RXD3_E8_MASK			BIT(11)
#define RGMII_RXD2_E8_MASK			BIT(10)
#define RGMII_RXD1_E8_MASK			BIT(9)
#define RGMII_RXD0_E8_MASK			BIT(8)
#define RGMII_RXC_E8_MASK			BIT(7)
#define RGMII_RXCTL_E8_MASK			BIT(6)
#define RGMII_TXD3_E8_MASK			BIT(5)
#define RGMII_TXD2_E8_MASK			BIT(4)
#define RGMII_TXD1_E8_MASK			BIT(3)
#define RGMII_TXD0_E8_MASK			BIT(2)
#define RGMII_TXC_E8_MASK			BIT(1)
#define RGMII_TXCTL_E8_MASK			BIT(0)

#define REG_I2C_SDA_PU				0x002c
#define SPI_MISO_PU_MASK			BIT(11)
#define SPI_MOSI_PU_MASK			BIT(10)
#define SPI_CLK_PU_MASK				BIT(9)
#define SPI_CS_PU_MASK				BIT(8)
#define PCIE1_RESET_PU_MASK			BIT(7)
#define PCIE0_RESET_PU_MASK			BIT(6)
#define MDIO_PU_MASK				BIT(5)
#define MDC_PU_MASK				BIT(4)
#define UART1_RXD_PU_MASK			BIT(3)
#define UART1_TXD_PU_MASK			BIT(2)
#define I2C_SCL_PU_MASK				BIT(1)
#define I2C_SDA_PU_MASK				BIT(0)

#define REG_I2C_SDA_PD				0x0030
#define SPI_MISO_PD_MASK			BIT(11)
#define SPI_MOSI_PD_MASK			BIT(10)
#define SPI_CLK_PD_MASK				BIT(9)
#define SPI_CS_PD_MASK				BIT(8)
#define PCIE1_RESET_PD_MASK			BIT(7)
#define PCIE0_RESET_PD_MASK			BIT(6)
#define MDIO_PD_MASK				BIT(5)
#define MDC_PD_MASK				BIT(4)
#define UART1_RXD_PD_MASK			BIT(3)
#define UART1_TXD_PD_MASK			BIT(2)
#define I2C_SCL_PD_MASK				BIT(1)
#define I2C_SDA_PD_MASK				BIT(0)

#define REG_GPIO_PU				0x0034
#define REG_GPIO_PD				0x0038

#define REG_RGMII_TXCTL_PU			0x0040
#define TMII_TXC_PU_MASK			BIT(12)
#define RGMII_RXD3_PU_MASK			BIT(11)
#define RGMII_RXD2_PU_MASK			BIT(10)
#define RGMII_RXD1_PU_MASK			BIT(9)
#define RGMII_RXD0_PU_MASK			BIT(8)
#define RGMII_RXC_PU_MASK			BIT(7)
#define RGMII_RXCTL_PU_MASK			BIT(6)
#define RGMII_TXD3_PU_MASK			BIT(5)
#define RGMII_TXD2_PU_MASK			BIT(4)
#define RGMII_TXD1_PU_MASK			BIT(3)
#define RGMII_TXD0_PU_MASK			BIT(2)
#define RGMII_TXC_PU_MASK			BIT(1)
#define RGMII_TXCTL_PU_MASK			BIT(0)

#define REG_RGMII_TXCTL_PD			0x0044
#define TMII_TXC_PD_MASK			BIT(12)
#define RGMII_RXD3_PD_MASK			BIT(11)
#define RGMII_RXD2_PD_MASK			BIT(10)
#define RGMII_RXD1_PD_MASK			BIT(9)
#define RGMII_RXD0_PD_MASK			BIT(8)
#define RGMII_RXC_PD_MASK			BIT(7)
#define RGMII_RXCTL_PU_MASK			BIT(6)
#define RGMII_TXD3_PD_MASK			BIT(5)
#define RGMII_TXD2_PD_MASK			BIT(4)
#define RGMII_TXD1_PD_MASK			BIT(3)
#define RGMII_TXD0_PD_MASK			BIT(2)
#define RGMII_TXC_PD_MASK			BIT(1)
#define RGMII_TXCTL_PD_MASK			BIT(0)

/* GPIOs */
#define REG_GPIO_CTRL				0x0000
#define REG_GPIO_DATA				0x0004
#define REG_GPIO_INT				0x0008
#define REG_GPIO_INT_EDGE			0x000c
#define REG_GPIO_INT_LEVEL			0x0010
#define REG_GPIO_OE				0x0014
#define REG_GPIO_CTRL1				0x0020

/* PWM MODE CONF */
#define REG_GPIO_FLASH_MODE_CFG			0x0034
#define GPIO15_FLASH_MODE_CFG			BIT(15)
#define GPIO14_FLASH_MODE_CFG			BIT(14)
#define GPIO13_FLASH_MODE_CFG			BIT(13)
#define GPIO12_FLASH_MODE_CFG			BIT(12)
#define GPIO11_FLASH_MODE_CFG			BIT(11)
#define GPIO10_FLASH_MODE_CFG			BIT(10)
#define GPIO9_FLASH_MODE_CFG			BIT(9)
#define GPIO8_FLASH_MODE_CFG			BIT(8)
#define GPIO7_FLASH_MODE_CFG			BIT(7)
#define GPIO6_FLASH_MODE_CFG			BIT(6)
#define GPIO5_FLASH_MODE_CFG			BIT(5)
#define GPIO4_FLASH_MODE_CFG			BIT(4)
#define GPIO3_FLASH_MODE_CFG			BIT(3)
#define GPIO2_FLASH_MODE_CFG			BIT(2)
#define GPIO1_FLASH_MODE_CFG			BIT(1)
#define GPIO0_FLASH_MODE_CFG			BIT(0)

/* GPIOs Cont */
#define REG_GPIO_CTRL2				0x0060
#define REG_GPIO_CTRL3				0x0064

/* PWM MODE CONF EXT */
#define REG_GPIO_FLASH_MODE_CFG_EXT		0x0068
#define GPIO51_FLASH_MODE_CFG			BIT(31)
#define GPIO50_FLASH_MODE_CFG			BIT(30)
#define GPIO49_FLASH_MODE_CFG			BIT(29)
#define GPIO48_FLASH_MODE_CFG			BIT(28)
#define GPIO47_FLASH_MODE_CFG			BIT(27)
#define GPIO46_FLASH_MODE_CFG			BIT(26)
#define GPIO45_FLASH_MODE_CFG			BIT(25)
#define GPIO44_FLASH_MODE_CFG			BIT(24)
#define GPIO43_FLASH_MODE_CFG			BIT(23)
#define GPIO42_FLASH_MODE_CFG			BIT(22)
#define GPIO41_FLASH_MODE_CFG			BIT(21)
#define GPIO40_FLASH_MODE_CFG			BIT(20)
#define GPIO39_FLASH_MODE_CFG			BIT(19)
#define GPIO38_FLASH_MODE_CFG			BIT(18)
#define GPIO37_FLASH_MODE_CFG			BIT(17)
#define GPIO32_FLASH_MODE_CFG			BIT(16)
#define GPIO31_FLASH_MODE_CFG			BIT(15)
#define GPIO30_FLASH_MODE_CFG			BIT(14)
#define GPIO29_FLASH_MODE_CFG			BIT(13)
#define GPIO28_FLASH_MODE_CFG			BIT(12)
#define GPIO27_FLASH_MODE_CFG			BIT(11)
#define GPIO26_FLASH_MODE_CFG			BIT(10)
#define GPIO25_FLASH_MODE_CFG			BIT(9)
#define GPIO24_FLASH_MODE_CFG			BIT(8)
#define GPIO23_FLASH_MODE_CFG			BIT(7)
#define GPIO22_FLASH_MODE_CFG			BIT(6)
#define GPIO21_FLASH_MODE_CFG			BIT(5)
#define GPIO20_FLASH_MODE_CFG			BIT(4)
#define GPIO19_FLASH_MODE_CFG			BIT(3)
#define GPIO18_FLASH_MODE_CFG			BIT(2)
#define GPIO17_FLASH_MODE_CFG			BIT(1)
#define GPIO16_FLASH_MODE_CFG			BIT(0)

/* GPIOs Cont */
#define REG_GPIO_DATA1				0x0070
#define REG_GPIO_OE1				0x0078

#define ECONET_NUM_PINS				64
#define ECONET_NUM_INT_PINS			16
#define ECONET_PIN_BANK_SIZE			(ECONET_NUM_PINS / 2)
#define ECONET_REG_GPIOCTRL_NUM_PIN		(ECONET_NUM_PINS / 4)

static const u32 gpio_data_regs[] = {
	REG_GPIO_DATA,
	REG_GPIO_DATA1
};

static const u32 gpio_out_regs[] = {
	REG_GPIO_OE,
	REG_GPIO_OE1
};

static const u32 gpio_dir_regs[] = {
	REG_GPIO_CTRL,
	REG_GPIO_CTRL1,
	REG_GPIO_CTRL2,
	REG_GPIO_CTRL3
};

static const u32 irq_status_regs[] = {
	REG_GPIO_INT,
};

static const u32 irq_level_regs[] = {
	REG_GPIO_INT_LEVEL,
};

static const u32 irq_edge_regs[] = {
	REG_GPIO_INT_EDGE,
};

struct econet_pinctrl_reg {
	u32 offset;
	u32 mask;
};

enum econet_pinctrl_mux_func {
	ECONET_FUNC_MUX,
	ECONET_FUNC_PWM_MUX,
	ECONET_FUNC_PWM_EXT_MUX,
};

struct econet_pinctrl_func_group {
	const char *name;
	struct {
		enum econet_pinctrl_mux_func mux;
		u32 offset;
		u32 mask;
		u32 val;
	} regmap[2];
	int regmap_size;
};

struct econet_pinctrl_func {
	const struct function_desc desc;
	const struct econet_pinctrl_func_group *groups;
	u8 group_size;
};

struct econet_pinctrl_conf {
	u32 pin;
	struct econet_pinctrl_reg reg;
};

struct econet_pinctrl_gpiochip {
	struct gpio_chip chip;

	/* gpio */
	const u32 *data;
	const u32 *dir;
	const u32 *out;
	/* irq */
	const u32 *status;
	const u32 *level;
	const u32 *edge;

	u32 irq_type[ECONET_NUM_INT_PINS];
};

struct econet_pinctrl {
	struct pinctrl_dev *ctrl;

	struct regmap *chip_scu;
	struct regmap *regmap;

	struct econet_pinctrl_gpiochip gpiochip;
};

static struct pinctrl_pin_desc econet_pinctrl_pins[] = {
	PINCTRL_PIN(0, "uart1_txd"),
	PINCTRL_PIN(1, "uart1_rxd"),
	PINCTRL_PIN(2, "i2c_scl"),
	PINCTRL_PIN(3, "i2c_sda"),
	PINCTRL_PIN(4, "spi_cs"),
	PINCTRL_PIN(5, "spi_clk"),
	PINCTRL_PIN(6, "spi_mosi"),
	PINCTRL_PIN(7, "spi_miso"),
	PINCTRL_PIN(8, "mdio"),
	PINCTRL_PIN(9, "mdc"),
	PINCTRL_PIN(13, "gpio0"),
	PINCTRL_PIN(14, "gpio1"),
	PINCTRL_PIN(15, "gpio2"),
	PINCTRL_PIN(16, "gpio3"),
	PINCTRL_PIN(17, "gpio4"),
	PINCTRL_PIN(18, "gpio5"),
	PINCTRL_PIN(19, "gpio6"),
	PINCTRL_PIN(20, "gpio7"),
	PINCTRL_PIN(21, "gpio8"),
	PINCTRL_PIN(22, "gpio9"),
	PINCTRL_PIN(23, "gpio10"),
	PINCTRL_PIN(24, "gpio11"),
	PINCTRL_PIN(25, "gpio12"),
	PINCTRL_PIN(26, "gpio13"),
	PINCTRL_PIN(27, "gpio14"),
	PINCTRL_PIN(28, "gpio15"),
	PINCTRL_PIN(29, "gpio16"),
	PINCTRL_PIN(30, "gpio17"),
	PINCTRL_PIN(31, "gpio18"),
	PINCTRL_PIN(32, "gpio19"),
	PINCTRL_PIN(33, "gpio20"),
	PINCTRL_PIN(34, "gpio21"),
	PINCTRL_PIN(35, "gpio22"),
	PINCTRL_PIN(36, "gpio23"),
	PINCTRL_PIN(37, "gpio24"),
	PINCTRL_PIN(38, "gpio25"),
	PINCTRL_PIN(39, "gpio26"),
	PINCTRL_PIN(40, "gpio27"),
	PINCTRL_PIN(41, "gpio28"),
	PINCTRL_PIN(42, "gpio29"),
	PINCTRL_PIN(43, "pcie_reset0"),
	PINCTRL_PIN(44, "pcie_reset1"),
};

static const int pon_pins[] = {29, 30, 31, 32, 33 };
static const int pon_i2c_pins[] = { 2, 3 };
static const int dmt_i2c_pins[] = { 27, 28 };
static const int pon_tod_1pps_pins[] = { 35 };
static const int gsw_tod_1pps_pins[] = { 35 };
static const int dmt_tod_1pps_pins[] = { 35 };
static const int sipo_pins[] = { 21, 24 };
static const int sipo_rclk_pins[] = { 21, 24, 22 };
static const int uart2_pins[] = { 16, 23 };
static const int pcm1_pins[] = { 25, 26, 27, 28 };
static const int pcm2_pins[] = { 17, 18, 19, 20 };
static const int spi_quad_pins[] = { 16, 23 };
static const int spi2_pins[] = { 17, 18, 19, 20 };
static const int pcm_spi_cs3_pins[] = { 16 };
static const int pcm_spi_cs4_pins[] = { 22 };
static const int pcm_int_pins[] = { 16 };
static const int pcm_rst_pins[] = { 15 };
static const int ejtag_pins[] = { 16, 21, 23, 24, 34 };
static const int gpio0_pins[] = { 13 };
static const int gpio1_pins[] = { 14 };
static const int gpio2_pins[] = { 15 };
static const int gpio3_pins[] = { 16 };
static const int gpio4_pins[] = { 17 };
static const int gpio5_pins[] = { 18 };
static const int gpio6_pins[] = { 19 };
static const int gpio7_pins[] = { 20 };
static const int gpio8_pins[] = { 21 };
static const int gpio9_pins[] = { 22 };
static const int gpio10_pins[] = { 23 };
static const int gpio11_pins[] = { 24 };
static const int gpio12_pins[] = { 25 };
static const int gpio13_pins[] = { 26 };
static const int gpio14_pins[] = { 27 };
static const int gpio15_pins[] = { 28 };
static const int gpio16_pins[] = { 29 };
static const int gpio17_pins[] = { 30 };
static const int gpio18_pins[] = { 31 };
static const int gpio19_pins[] = { 32 };
static const int gpio20_pins[] = { 33 };
static const int gpio21_pins[] = { 34 };
static const int gpio22_pins[] = { 35 };
static const int gpio23_pins[] = { 36 };
static const int gpio24_pins[] = { 37 };
static const int gpio25_pins[] = { 38 };
static const int gpio26_pins[] = { 39 };
static const int gpio27_pins[] = { 40 };
static const int gpio28_pins[] = { 41 };
static const int gpio29_pins[] = { 42 };
static const int gpio30_pins[] = { 43 };
static const int gpio31_pins[] = { 44 };
static const int pcie_reset0_pins[] = { 43 };
static const int pcie_reset1_pins[] = { 44 };

static const struct pingroup econet_pinctrl_groups[] = {
	PINCTRL_PIN_GROUP(pon),
	PINCTRL_PIN_GROUP(pon_i2c),
	PINCTRL_PIN_GROUP(dmt_i2c),
	PINCTRL_PIN_GROUP(pon_tod_1pps),
	PINCTRL_PIN_GROUP(gsw_tod_1pps),
	PINCTRL_PIN_GROUP(dmt_tod_1pps),
	PINCTRL_PIN_GROUP(sipo),
	PINCTRL_PIN_GROUP(sipo_rclk),
	PINCTRL_PIN_GROUP(uart2),
	PINCTRL_PIN_GROUP(ejtag),
	PINCTRL_PIN_GROUP(pcm1),
	PINCTRL_PIN_GROUP(pcm2),
	PINCTRL_PIN_GROUP(spi2),
	PINCTRL_PIN_GROUP(spi_quad),
//	PINCTRL_PIN_GROUP(spi_cs1),
	PINCTRL_PIN_GROUP(pcm_int),
	PINCTRL_PIN_GROUP(pcm_rst),
	PINCTRL_PIN_GROUP(pcm_spi_cs3),
	PINCTRL_PIN_GROUP(pcm_spi_cs4),
	PINCTRL_PIN_GROUP(gpio0),
	PINCTRL_PIN_GROUP(gpio1),
	PINCTRL_PIN_GROUP(gpio2),
	PINCTRL_PIN_GROUP(gpio3),
	PINCTRL_PIN_GROUP(gpio4),
	PINCTRL_PIN_GROUP(gpio5),
	PINCTRL_PIN_GROUP(gpio6),
	PINCTRL_PIN_GROUP(gpio7),
	PINCTRL_PIN_GROUP(gpio8),
	PINCTRL_PIN_GROUP(gpio9),
	PINCTRL_PIN_GROUP(gpio10),
	PINCTRL_PIN_GROUP(gpio11),
	PINCTRL_PIN_GROUP(gpio12),
	PINCTRL_PIN_GROUP(gpio13),
	PINCTRL_PIN_GROUP(gpio14),
	PINCTRL_PIN_GROUP(gpio15),
	PINCTRL_PIN_GROUP(gpio16),
	PINCTRL_PIN_GROUP(gpio17),
	PINCTRL_PIN_GROUP(gpio18),
	PINCTRL_PIN_GROUP(gpio19),
	PINCTRL_PIN_GROUP(gpio20),
	PINCTRL_PIN_GROUP(gpio21),
	PINCTRL_PIN_GROUP(gpio22),
	PINCTRL_PIN_GROUP(gpio23),
	PINCTRL_PIN_GROUP(gpio24),
	PINCTRL_PIN_GROUP(gpio25),
	PINCTRL_PIN_GROUP(gpio26),
	PINCTRL_PIN_GROUP(gpio27),
	PINCTRL_PIN_GROUP(gpio28),
	PINCTRL_PIN_GROUP(gpio29),
	PINCTRL_PIN_GROUP(gpio30),
	PINCTRL_PIN_GROUP(gpio31),
	PINCTRL_PIN_GROUP(pcie_reset0),
	PINCTRL_PIN_GROUP(pcie_reset1),
};

static const char *const pon_groups[] = { "pon" };
static const char *const tod_1pps_groups[] = { "pon_tod_1pps", "gsw_tod_1pps", "dmt_tod_1pps" };
static const char *const sipo_groups[] = { "sipo", "sipo_rclk" };
static const char *const uart_groups[] = { "uart2" };
static const char *const pon_i2c_groups[] = { "pon_i2c" };
static const char *const dmt_i2c_groups[] = { "dmt_i2c" };
static const char *const ejtag_groups[] = { "ejtag" };
static const char *const pcm_groups[] = { "pcm1", "pcm2" };
static const char *const spi_groups[] = { "spi_quad", "spi_cs1" };
static const char *const pcm_spi_groups[] = { "pcm_spi", "pcm_spi_int",
					      "pcm_spi_rst", "pcm_spi_cs3", "pcm_spi_cs4" };
static const char *const pcie_reset_groups[] = { "pcie_reset0", "pcie_reset1" };
static const char *const pwm_groups[] = { "gpio0", "gpio1",
					  "gpio2", "gpio3",
					  "gpio4", "gpio5",
					  "gpio6", "gpio7",
					  "gpio8", "gpio9",
					  "gpio10", "gpio11",
					  "gpio12", "gpio13",
					  "gpio14", "gpio15",
					  "gpio16", "gpio17",
					  "gpio18", "gpio19",
					  "gpio20", "gpio21",
					  "gpio22", "gpio23",
					  "gpio24", "gpio25",
					  "gpio26", "gpio27",
					  "gpio28", "gpio29",
					  "gpio30", "gpio31" };
static const char *const phy0_led_groups[] = { "gpio3", "gpio7",
						"gpio8", "gpio9", "gpio10"};
static const char *const phy1_led_groups[] = { "gpio3", "gpio7",
						"gpio8", "gpio9", "gpio10"};
static const char *const phy2_led_groups[] = { "gpio3", "gpio7",
						"gpio8", "gpio9", "gpio10"};
static const char *const phy3_led_groups[] = { "gpio3", "gpio7",
						"gpio8", "gpio9", "gpio10"};
static const char *const phy4_led_groups[] = { "gpio3", "gpio7",
						"gpio8", "gpio9", "gpio10"};

static const struct econet_pinctrl_func_group pon_func_group[] = {
	{
		.name = "pon",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_I2C_MODE,
			GPIO_PON_MODE_MASK,
			GPIO_PON_MODE_MASK
		},
		.regmap_size = 1,
	},
};

static const struct econet_pinctrl_func_group tod_1pps_func_group[] = {
	{
		.name = "pon_tod_1pps",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_I2C_MODE,
			PON_TOD_1PPS_MODE_MASK,
			PON_TOD_1PPS_MODE_MASK
		},
		.regmap_size = 1,
	}, {
		.name = "gsw_tod_1pps",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_I2C_MODE,
			GSW_TOD_1PPS_MODE_MASK,
			GSW_TOD_1PPS_MODE_MASK
		},
		.regmap_size = 1,
	}, {
		.name = "dmt_tod_1pps",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_I2C_MODE,
			DMT_TOD_1PPS_MODE_MASK,
			DMT_TOD_1PPS_MODE_MASK
		},
		.regmap_size = 1,
	},
};

static const struct econet_pinctrl_func_group sipo_func_group[] = {
	{
		.name = "sipo",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_I2C_MODE,
			SIPO_MODE_MASK | SIPO_RCLK_MODE_MASK,
			SIPO_MODE_MASK
		},
		.regmap_size = 1,
	}, {
		.name = "sipo_rclk",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_I2C_MODE,
			SIPO_MODE_MASK | SIPO_RCLK_MODE_MASK,
			SIPO_MODE_MASK | SIPO_RCLK_MODE_MASK
		},
		.regmap_size = 1,
	},
};

static const struct econet_pinctrl_func_group uart_func_group[] = {
	{
		.name = "uart2",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_I2C_MODE,
			GPIO_UART2_MODE_MASK,
			GPIO_UART2_MODE_MASK
		},
		.regmap_size = 1,
	},
};

static const struct econet_pinctrl_func_group pon_i2c_func_group[] = {
	{
		.name = "pon_i2c",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_I2C_MODE,
			PON_I2C_MODE_MASK,
			PON_I2C_MODE_MASK
		},
		.regmap_size = 1,
	},
};

static const struct econet_pinctrl_func_group dmt_i2c_func_group[] = {
	{
		.name = "dmt_i2c",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_I2C_MODE,
			GPIO_DSL_I2C_MODE_MASK,
			GPIO_DSL_I2C_MODE_MASK
		},
		.regmap_size = 1,
	},
};

static const struct econet_pinctrl_func_group ejtag_func_group[] = {
	{
		.name = "ejtag",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_CPU_EJTAG_EN,
			CPU_EJTAG_EN_MASK,
			CPU_EJTAG_EN_MASK
		},
		.regmap_size = 1,
	},
};

static const struct econet_pinctrl_func_group pcm_func_group[] = {
	{
		.name = "pcm1",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_I2C_MODE,
			GPIO_PCM1_MODE_MASK,
			GPIO_PCM1_MODE_MASK
		},
		.regmap_size = 1,
	}, {
		.name = "pcm2",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_I2C_MODE,
			GPIO_PCM2_MODE_MASK,
			GPIO_PCM2_MODE_MASK
		},
		.regmap_size = 1,
	},
};

static const struct econet_pinctrl_func_group spi_func_group[] = {
	{
		.name = "spi_quad",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_I2C_MODE,
			GPIO_SPI_QUAD_MODE_MASK,
			GPIO_SPI_QUAD_MODE_MASK
		},
		.regmap_size = 1,
	}, {
		.name = "spi2",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_I2C_MODE,
			GPIO_SPI2_MODE_MASK,
			GPIO_SPI2_MODE_MASK
		},
		.regmap_size = 1,
	}, {
		.name = "spi_cs3",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_I2C_MODE,
			GPIO_SPI_CS3_MODE_MASK,
			GPIO_SPI_CS3_MODE_MASK
		},
		.regmap_size = 1,
	}, {
		.name = "spi_cs4",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_I2C_MODE,
			GPIO_SPI_CS4_MODE_MASK,
			GPIO_SPI_CS4_MODE_MASK
		},
		.regmap_size = 1,
	},
};

static const struct econet_pinctrl_func_group pcm_spi_func_group[] = {
	{
		.name = "pcm_spi_int",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_I2C_MODE,
			GPIO_PCM_INT_MODE_MASK,
			GPIO_PCM_INT_MODE_MASK
		},
		.regmap_size = 1,
	}, {
		.name = "pcm_spi_rst",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_I2C_MODE,
			GPIO_PCM_RESET_MODE_MASK,
			GPIO_PCM_RESET_MODE_MASK
		},
		.regmap_size = 1,
	},
};

static const struct econet_pinctrl_func_group pcie_reset_func_group[] = {
	{
		.name = "pcie_reset0",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_I2C_MODE,
			PCIE_RESET0_GPIO_MODE_MASK,
			PCIE_RESET0_GPIO_MODE_MASK
		},
		.regmap_size = 1,
	}, {
		.name = "pcie_reset1",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_I2C_MODE,
			PCIE_RESET1_GPIO_MODE_MASK,
			PCIE_RESET1_GPIO_MODE_MASK
		},
		.regmap_size = 1,
	},
};

/* PWM */
static const struct econet_pinctrl_func_group pwm_func_group[] = {
	{
		.name = "gpio0",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_GPIO_FLASH_MODE_CFG,
			GPIO0_FLASH_MODE_CFG,
			GPIO0_FLASH_MODE_CFG
		},
		.regmap_size = 1,
	}, {
		.name = "gpio1",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_GPIO_FLASH_MODE_CFG,
			GPIO1_FLASH_MODE_CFG,
			GPIO1_FLASH_MODE_CFG
		},
		.regmap_size = 1,
	}, {
		.name = "gpio2",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_GPIO_FLASH_MODE_CFG,
			GPIO2_FLASH_MODE_CFG,
			GPIO2_FLASH_MODE_CFG
		},
		.regmap_size = 1,
	}, {
		.name = "gpio3",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_GPIO_FLASH_MODE_CFG,
			GPIO3_FLASH_MODE_CFG,
			GPIO3_FLASH_MODE_CFG
		},
		.regmap_size = 1,
	}, {
		.name = "gpio4",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_GPIO_FLASH_MODE_CFG,
			GPIO4_FLASH_MODE_CFG,
			GPIO4_FLASH_MODE_CFG
		},
		.regmap_size = 1,
	}, {
		.name = "gpio5",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_GPIO_FLASH_MODE_CFG,
			GPIO5_FLASH_MODE_CFG,
			GPIO5_FLASH_MODE_CFG
		},
		.regmap_size = 1,
	}, {
		.name = "gpio6",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_GPIO_FLASH_MODE_CFG,
			GPIO6_FLASH_MODE_CFG,
			GPIO6_FLASH_MODE_CFG
		},
		.regmap_size = 1,
	}, {
		.name = "gpio7",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_GPIO_FLASH_MODE_CFG,
			GPIO7_FLASH_MODE_CFG,
			GPIO7_FLASH_MODE_CFG
		},
		.regmap_size = 1,
	}, {
		.name = "gpio8",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_GPIO_FLASH_MODE_CFG,
			GPIO8_FLASH_MODE_CFG,
			GPIO8_FLASH_MODE_CFG
		},
		.regmap_size = 1,
	}, {
		.name = "gpio9",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_GPIO_FLASH_MODE_CFG,
			GPIO9_FLASH_MODE_CFG,
			GPIO9_FLASH_MODE_CFG
		},
		.regmap_size = 1,
	}, {
		.name = "gpio10",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_GPIO_FLASH_MODE_CFG,
			GPIO10_FLASH_MODE_CFG,
			GPIO10_FLASH_MODE_CFG
		},
		.regmap_size = 1,
	}, {
		.name = "gpio11",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_GPIO_FLASH_MODE_CFG,
			GPIO11_FLASH_MODE_CFG,
			GPIO11_FLASH_MODE_CFG
		},
		.regmap_size = 1,
	}, {
		.name = "gpio12",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_GPIO_FLASH_MODE_CFG,
			GPIO12_FLASH_MODE_CFG,
			GPIO12_FLASH_MODE_CFG
		},
		.regmap_size = 1,
	}, {
		.name = "gpio13",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_GPIO_FLASH_MODE_CFG,
			GPIO13_FLASH_MODE_CFG,
			GPIO13_FLASH_MODE_CFG
		},
		.regmap_size = 1,
	}, {
		.name = "gpio14",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_GPIO_FLASH_MODE_CFG,
			GPIO14_FLASH_MODE_CFG,
			GPIO14_FLASH_MODE_CFG
		},
		.regmap_size = 1,
	}, {
		.name = "gpio15",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_GPIO_FLASH_MODE_CFG,
			GPIO15_FLASH_MODE_CFG,
			GPIO15_FLASH_MODE_CFG
		},
		.regmap_size = 1,
	}, {
		.name = "gpio16",
		.regmap[0] = {
			ECONET_FUNC_PWM_EXT_MUX,
			REG_GPIO_FLASH_MODE_CFG_EXT,
			GPIO16_FLASH_MODE_CFG,
			GPIO16_FLASH_MODE_CFG
		},
		.regmap_size = 1,
	}, {
		.name = "gpio17",
		.regmap[0] = {
			ECONET_FUNC_PWM_EXT_MUX,
			REG_GPIO_FLASH_MODE_CFG_EXT,
			GPIO17_FLASH_MODE_CFG,
			GPIO17_FLASH_MODE_CFG
		},
		.regmap_size = 1,
	}, {
		.name = "gpio18",
		.regmap[0] = {
			ECONET_FUNC_PWM_EXT_MUX,
			REG_GPIO_FLASH_MODE_CFG_EXT,
			GPIO18_FLASH_MODE_CFG,
			GPIO18_FLASH_MODE_CFG
		},
		.regmap_size = 1,
	}, {
		.name = "gpio19",
		.regmap[0] = {
			ECONET_FUNC_PWM_EXT_MUX,
			REG_GPIO_FLASH_MODE_CFG_EXT,
			GPIO19_FLASH_MODE_CFG,
			GPIO19_FLASH_MODE_CFG
		},
		.regmap_size = 1,
	}, {
		.name = "gpio20",
		.regmap[0] = {
			ECONET_FUNC_PWM_EXT_MUX,
			REG_GPIO_FLASH_MODE_CFG_EXT,
			GPIO20_FLASH_MODE_CFG,
			GPIO20_FLASH_MODE_CFG
		},
		.regmap_size = 1,
	}, {
		.name = "gpio21",
		.regmap[0] = {
			ECONET_FUNC_PWM_EXT_MUX,
			REG_GPIO_FLASH_MODE_CFG_EXT,
			GPIO21_FLASH_MODE_CFG,
			GPIO21_FLASH_MODE_CFG
		},
		.regmap_size = 1,
	}, {
		.name = "gpio22",
		.regmap[0] = {
			ECONET_FUNC_PWM_EXT_MUX,
			REG_GPIO_FLASH_MODE_CFG_EXT,
			GPIO22_FLASH_MODE_CFG,
			GPIO22_FLASH_MODE_CFG
		},
		.regmap_size = 1,
	}, {
		.name = "gpio23",
		.regmap[0] = {
			ECONET_FUNC_PWM_EXT_MUX,
			REG_GPIO_FLASH_MODE_CFG_EXT,
			GPIO23_FLASH_MODE_CFG,
			GPIO23_FLASH_MODE_CFG
		},
		.regmap_size = 1,
	}, {
		.name = "gpio24",
		.regmap[0] = {
			ECONET_FUNC_PWM_EXT_MUX,
			REG_GPIO_FLASH_MODE_CFG_EXT,
			GPIO24_FLASH_MODE_CFG,
			GPIO24_FLASH_MODE_CFG
		},
		.regmap_size = 1,
	}, {
		.name = "gpio25",
		.regmap[0] = {
			ECONET_FUNC_PWM_EXT_MUX,
			REG_GPIO_FLASH_MODE_CFG_EXT,
			GPIO25_FLASH_MODE_CFG,
			GPIO25_FLASH_MODE_CFG
		},
		.regmap_size = 1,
	}, {
		.name = "gpio26",
		.regmap[0] = {
			ECONET_FUNC_PWM_EXT_MUX,
			REG_GPIO_FLASH_MODE_CFG_EXT,
			GPIO26_FLASH_MODE_CFG,
			GPIO26_FLASH_MODE_CFG
		},
		.regmap_size = 1,
	}, {
		.name = "gpio27",
		.regmap[0] = {
			ECONET_FUNC_PWM_EXT_MUX,
			REG_GPIO_FLASH_MODE_CFG_EXT,
			GPIO27_FLASH_MODE_CFG,
			GPIO27_FLASH_MODE_CFG
		},
		.regmap_size = 1,
	}, {
		.name = "gpio28",
		.regmap[0] = {
			ECONET_FUNC_PWM_EXT_MUX,
			REG_GPIO_FLASH_MODE_CFG_EXT,
			GPIO28_FLASH_MODE_CFG,
			GPIO28_FLASH_MODE_CFG
		},
		.regmap_size = 1,
	}, {
		.name = "gpio29",
		.regmap[0] = {
			ECONET_FUNC_PWM_EXT_MUX,
			REG_GPIO_FLASH_MODE_CFG_EXT,
			GPIO29_FLASH_MODE_CFG,
			GPIO29_FLASH_MODE_CFG
		},
		.regmap_size = 1,
	}, {
		.name = "gpio30",
		.regmap[0] = {
			ECONET_FUNC_PWM_EXT_MUX,
			REG_GPIO_FLASH_MODE_CFG_EXT,
			GPIO30_FLASH_MODE_CFG,
			GPIO30_FLASH_MODE_CFG
		},
		.regmap_size = 1,
	}, {
		.name = "gpio31",
		.regmap[0] = {
			ECONET_FUNC_PWM_EXT_MUX,
			REG_GPIO_FLASH_MODE_CFG_EXT,
			GPIO31_FLASH_MODE_CFG,
			GPIO31_FLASH_MODE_CFG
		},
		.regmap_size = 1,
	},
};

static const struct econet_pinctrl_func_group phy0_led_func_group[] = {
	{
		.name = "gpio3",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_I2C_MODE,
			GPIO_LAN0_LED_MODE_MASK,
			GPIO_LAN0_LED_MODE_MASK
		},
		.regmap[1] = {
			ECONET_FUNC_MUX,
			REG_LAN_LED0_MAPPING,
			LAN1_LED_MAPPING_MASK,
			LAN1_PHY1_LED_MAP
		},
		.regmap_size = 2,
	}, {
		.name = "gpio7",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_I2C_MODE,
			GPIO_LAN1_LED_MODE_MASK,
			GPIO_LAN1_LED_MODE_MASK
		},
		.regmap[1] = {
			ECONET_FUNC_MUX,
			REG_LAN_LED0_MAPPING,
			LAN2_LED_MAPPING_MASK,
			LAN2_PHY1_LED_MAP
		},
		.regmap_size = 2,
	}, {
		.name = "gpio8",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_I2C_MODE,
			GPIO_LAN2_LED_MODE_MASK,
			GPIO_LAN2_LED_MODE_MASK
		},
		.regmap[1] = {
			ECONET_FUNC_MUX,
			REG_LAN_LED0_MAPPING,
			LAN3_LED_MAPPING_MASK,
			LAN3_PHY1_LED_MAP
		},
		.regmap_size = 2,
	}, {
		.name = "gpio9",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_I2C_MODE,
			GPIO_LAN3_LED_MODE_MASK,
			GPIO_LAN3_LED_MODE_MASK
		},
		.regmap[1] = {
			ECONET_FUNC_MUX,
			REG_LAN_LED0_MAPPING,
			LAN4_LED_MAPPING_MASK,
			LAN4_PHY1_LED_MAP
		},
		.regmap_size = 2,
	},  {
		.name = "gpio10",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_I2C_MODE,
			GPIO_GE_LED_MASK_MASK,
			GPIO_GE_LED_MASK_MASK
		},
		.regmap[1] = {
			ECONET_FUNC_MUX,
			REG_LAN_LED0_MAPPING,
			LAN0_LED_MAPPING_MASK,
			LAN0_PHY1_LED_MAP
		},
		.regmap_size = 2,
	},
};


static const struct econet_pinctrl_func_group phy1_led_func_group[] = {
	{
		.name = "gpio3",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_I2C_MODE,
			GPIO_LAN0_LED_MODE_MASK,
			GPIO_LAN0_LED_MODE_MASK
		},
		.regmap[1] = {
			ECONET_FUNC_MUX,
			REG_LAN_LED0_MAPPING,
			LAN1_LED_MAPPING_MASK,
			LAN1_PHY1_LED_MAP
		},
		.regmap_size = 2,
	}, {
		.name = "gpio7",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_I2C_MODE,
			GPIO_LAN1_LED_MODE_MASK,
			GPIO_LAN1_LED_MODE_MASK
		},
		.regmap[1] = {
			ECONET_FUNC_MUX,
			REG_LAN_LED0_MAPPING,
			LAN2_LED_MAPPING_MASK,
			LAN2_PHY1_LED_MAP
		},
		.regmap_size = 2,
	}, {
		.name = "gpio8",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_I2C_MODE,
			GPIO_LAN2_LED_MODE_MASK,
			GPIO_LAN2_LED_MODE_MASK
		},
		.regmap[1] = {
			ECONET_FUNC_MUX,
			REG_LAN_LED0_MAPPING,
			LAN3_LED_MAPPING_MASK,
			LAN3_PHY1_LED_MAP
		},
		.regmap_size = 2,
	}, {
		.name = "gpio9",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_I2C_MODE,
			GPIO_LAN3_LED_MODE_MASK,
			GPIO_LAN3_LED_MODE_MASK
		},
		.regmap[1] = {
			ECONET_FUNC_MUX,
			REG_LAN_LED0_MAPPING,
			LAN4_LED_MAPPING_MASK,
			LAN4_PHY1_LED_MAP
		},
		.regmap_size = 2,
	},  {
		.name = "gpio10",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_I2C_MODE,
			GPIO_GE_LED_MASK_MASK,
			GPIO_GE_LED_MASK_MASK
		},
		.regmap[1] = {
			ECONET_FUNC_MUX,
			REG_LAN_LED0_MAPPING,
			LAN0_LED_MAPPING_MASK,
			LAN0_PHY1_LED_MAP
		},
		.regmap_size = 2,
	},
};

static const struct econet_pinctrl_func_group phy2_led_func_group[] = {
	{
		.name = "gpio3",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_I2C_MODE,
			GPIO_LAN0_LED_MODE_MASK,
			GPIO_LAN0_LED_MODE_MASK
		},
		.regmap[1] = {
			ECONET_FUNC_MUX,
			REG_LAN_LED0_MAPPING,
			LAN1_LED_MAPPING_MASK,
			LAN1_PHY1_LED_MAP
		},
		.regmap_size = 2,
	}, {
		.name = "gpio7",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_I2C_MODE,
			GPIO_LAN1_LED_MODE_MASK,
			GPIO_LAN1_LED_MODE_MASK
		},
		.regmap[1] = {
			ECONET_FUNC_MUX,
			REG_LAN_LED0_MAPPING,
			LAN2_LED_MAPPING_MASK,
			LAN2_PHY1_LED_MAP
		},
		.regmap_size = 2,
	}, {
		.name = "gpio8",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_I2C_MODE,
			GPIO_LAN2_LED_MODE_MASK,
			GPIO_LAN2_LED_MODE_MASK
		},
		.regmap[1] = {
			ECONET_FUNC_MUX,
			REG_LAN_LED0_MAPPING,
			LAN3_LED_MAPPING_MASK,
			LAN3_PHY1_LED_MAP
		},
		.regmap_size = 2,
	}, {
		.name = "gpio9",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_I2C_MODE,
			GPIO_LAN3_LED_MODE_MASK,
			GPIO_LAN3_LED_MODE_MASK
		},
		.regmap[1] = {
			ECONET_FUNC_MUX,
			REG_LAN_LED0_MAPPING,
			LAN4_LED_MAPPING_MASK,
			LAN4_PHY1_LED_MAP
		},
		.regmap_size = 2,
	},  {
		.name = "gpio10",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_I2C_MODE,
			GPIO_GE_LED_MASK_MASK,
			GPIO_GE_LED_MASK_MASK
		},
		.regmap[1] = {
			ECONET_FUNC_MUX,
			REG_LAN_LED0_MAPPING,
			LAN0_LED_MAPPING_MASK,
			LAN0_PHY1_LED_MAP
		},
		.regmap_size = 2,
	},
};

static const struct econet_pinctrl_func_group phy3_led_func_group[] = {
	{
		.name = "gpio3",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_I2C_MODE,
			GPIO_LAN0_LED_MODE_MASK,
			GPIO_LAN0_LED_MODE_MASK
		},
		.regmap[1] = {
			ECONET_FUNC_MUX,
			REG_LAN_LED0_MAPPING,
			LAN1_LED_MAPPING_MASK,
			LAN1_PHY1_LED_MAP
		},
		.regmap_size = 2,
	}, {
		.name = "gpio7",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_I2C_MODE,
			GPIO_LAN1_LED_MODE_MASK,
			GPIO_LAN1_LED_MODE_MASK
		},
		.regmap[1] = {
			ECONET_FUNC_MUX,
			REG_LAN_LED0_MAPPING,
			LAN2_LED_MAPPING_MASK,
			LAN2_PHY1_LED_MAP
		},
		.regmap_size = 2,
	}, {
		.name = "gpio8",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_I2C_MODE,
			GPIO_LAN2_LED_MODE_MASK,
			GPIO_LAN2_LED_MODE_MASK
		},
		.regmap[1] = {
			ECONET_FUNC_MUX,
			REG_LAN_LED0_MAPPING,
			LAN3_LED_MAPPING_MASK,
			LAN3_PHY1_LED_MAP
		},
		.regmap_size = 2,
	}, {
		.name = "gpio9",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_I2C_MODE,
			GPIO_LAN3_LED_MODE_MASK,
			GPIO_LAN3_LED_MODE_MASK
		},
		.regmap[1] = {
			ECONET_FUNC_MUX,
			REG_LAN_LED0_MAPPING,
			LAN4_LED_MAPPING_MASK,
			LAN4_PHY1_LED_MAP
		},
		.regmap_size = 2,
	},  {
		.name = "gpio10",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_I2C_MODE,
			GPIO_GE_LED_MASK_MASK,
			GPIO_GE_LED_MASK_MASK
		},
		.regmap[1] = {
			ECONET_FUNC_MUX,
			REG_LAN_LED0_MAPPING,
			LAN0_LED_MAPPING_MASK,
			LAN0_PHY1_LED_MAP
		},
		.regmap_size = 2,
	},
};

static const struct econet_pinctrl_func_group phy4_led_func_group[] = {
	{
		.name = "gpio3",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_I2C_MODE,
			GPIO_LAN0_LED_MODE_MASK,
			GPIO_LAN0_LED_MODE_MASK
		},
		.regmap[1] = {
			ECONET_FUNC_MUX,
			REG_LAN_LED0_MAPPING,
			LAN1_LED_MAPPING_MASK,
			LAN1_PHY1_LED_MAP
		},
		.regmap_size = 2,
	}, {
		.name = "gpio7",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_I2C_MODE,
			GPIO_LAN1_LED_MODE_MASK,
			GPIO_LAN1_LED_MODE_MASK
		},
		.regmap[1] = {
			ECONET_FUNC_MUX,
			REG_LAN_LED0_MAPPING,
			LAN2_LED_MAPPING_MASK,
			LAN2_PHY1_LED_MAP
		},
		.regmap_size = 2,
	}, {
		.name = "gpio8",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_I2C_MODE,
			GPIO_LAN2_LED_MODE_MASK,
			GPIO_LAN2_LED_MODE_MASK
		},
		.regmap[1] = {
			ECONET_FUNC_MUX,
			REG_LAN_LED0_MAPPING,
			LAN3_LED_MAPPING_MASK,
			LAN3_PHY1_LED_MAP
		},
		.regmap_size = 2,
	}, {
		.name = "gpio9",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_I2C_MODE,
			GPIO_LAN3_LED_MODE_MASK,
			GPIO_LAN3_LED_MODE_MASK
		},
		.regmap[1] = {
			ECONET_FUNC_MUX,
			REG_LAN_LED0_MAPPING,
			LAN4_LED_MAPPING_MASK,
			LAN4_PHY1_LED_MAP
		},
		.regmap_size = 2,
	},  {
		.name = "gpio10",
		.regmap[0] = {
			ECONET_FUNC_MUX,
			REG_I2C_MODE,
			GPIO_GE_LED_MASK_MASK,
			GPIO_GE_LED_MASK_MASK
		},
		.regmap[1] = {
			ECONET_FUNC_MUX,
			REG_LAN_LED0_MAPPING,
			LAN0_LED_MAPPING_MASK,
			LAN0_PHY1_LED_MAP
		},
		.regmap_size = 2,
	},
};


static const struct econet_pinctrl_func econet_pinctrl_funcs[] = {
	PINCTRL_FUNC_DESC(pon),
	PINCTRL_FUNC_DESC(tod_1pps),
	PINCTRL_FUNC_DESC(sipo),
	PINCTRL_FUNC_DESC(uart),
	PINCTRL_FUNC_DESC(pon_i2c),
	PINCTRL_FUNC_DESC(dmt_i2c),
	PINCTRL_FUNC_DESC(ejtag),
	PINCTRL_FUNC_DESC(pcm),
	PINCTRL_FUNC_DESC(spi),
	PINCTRL_FUNC_DESC(pcm_spi),
	PINCTRL_FUNC_DESC(pcie_reset),
	PINCTRL_FUNC_DESC(pwm),
	PINCTRL_FUNC_DESC(phy0_led),
	PINCTRL_FUNC_DESC(phy1_led),
	PINCTRL_FUNC_DESC(phy2_led),
	PINCTRL_FUNC_DESC(phy3_led),
	PINCTRL_FUNC_DESC(phy4_led),
};

static const struct econet_pinctrl_conf econet_pinctrl_pullup_conf[] = {
	PINCTRL_CONF_DESC(0, REG_I2C_SDA_PU, UART1_TXD_PU_MASK),
	PINCTRL_CONF_DESC(1, REG_I2C_SDA_PU, UART1_RXD_PU_MASK),
	PINCTRL_CONF_DESC(2, REG_I2C_SDA_PU, I2C_SDA_PU_MASK),
	PINCTRL_CONF_DESC(3, REG_I2C_SDA_PU, I2C_SCL_PU_MASK),
	PINCTRL_CONF_DESC(4, REG_I2C_SDA_PU, SPI_CS_PU_MASK),
	PINCTRL_CONF_DESC(5, REG_I2C_SDA_PU, SPI_CLK_PU_MASK),
	PINCTRL_CONF_DESC(6, REG_I2C_SDA_PU, SPI_MOSI_PU_MASK),
	PINCTRL_CONF_DESC(7, REG_I2C_SDA_PU, SPI_MISO_PU_MASK),
	PINCTRL_CONF_DESC(8, REG_I2C_SDA_PD, MDIO_PU_MASK),
	PINCTRL_CONF_DESC(9, REG_I2C_SDA_PD, MDC_PU_MASK),
	PINCTRL_CONF_DESC(13, REG_GPIO_PU, BIT(0)),
	PINCTRL_CONF_DESC(14, REG_GPIO_PU, BIT(1)),
	PINCTRL_CONF_DESC(15, REG_GPIO_PU, BIT(2)),
	PINCTRL_CONF_DESC(16, REG_GPIO_PU, BIT(3)),
	PINCTRL_CONF_DESC(17, REG_GPIO_PU, BIT(4)),
	PINCTRL_CONF_DESC(18, REG_GPIO_PU, BIT(5)),
	PINCTRL_CONF_DESC(19, REG_GPIO_PU, BIT(6)),
	PINCTRL_CONF_DESC(20, REG_GPIO_PU, BIT(7)),
	PINCTRL_CONF_DESC(21, REG_GPIO_PU, BIT(8)),
	PINCTRL_CONF_DESC(22, REG_GPIO_PU, BIT(9)),
	PINCTRL_CONF_DESC(23, REG_GPIO_PU, BIT(10)),
	PINCTRL_CONF_DESC(24, REG_GPIO_PU, BIT(11)),
	PINCTRL_CONF_DESC(25, REG_GPIO_PU, BIT(12)),
	PINCTRL_CONF_DESC(26, REG_GPIO_PU, BIT(13)),
	PINCTRL_CONF_DESC(27, REG_GPIO_PU, BIT(14)),
	PINCTRL_CONF_DESC(28, REG_GPIO_PU, BIT(15)),
	PINCTRL_CONF_DESC(29, REG_GPIO_PU, BIT(16)),
	PINCTRL_CONF_DESC(30, REG_GPIO_PU, BIT(17)),
	PINCTRL_CONF_DESC(31, REG_GPIO_PU, BIT(18)),
	PINCTRL_CONF_DESC(32, REG_GPIO_PU, BIT(18)),
	PINCTRL_CONF_DESC(33, REG_GPIO_PU, BIT(20)),
	PINCTRL_CONF_DESC(34, REG_GPIO_PU, BIT(21)),
	PINCTRL_CONF_DESC(35, REG_GPIO_PU, BIT(22)),
	PINCTRL_CONF_DESC(36, REG_GPIO_PU, BIT(23)),
	PINCTRL_CONF_DESC(37, REG_GPIO_PU, BIT(24)),
	PINCTRL_CONF_DESC(38, REG_GPIO_PU, BIT(25)),
	PINCTRL_CONF_DESC(39, REG_GPIO_PU, BIT(26)),
	PINCTRL_CONF_DESC(40, REG_GPIO_PU, BIT(27)),
	PINCTRL_CONF_DESC(41, REG_GPIO_PU, BIT(28)),
	PINCTRL_CONF_DESC(42, REG_GPIO_PU, BIT(29)),
	PINCTRL_CONF_DESC(43, REG_GPIO_PU, BIT(30)),
	PINCTRL_CONF_DESC(44, REG_GPIO_PU, BIT(31)),
	PINCTRL_CONF_DESC(61, REG_I2C_SDA_PU, PCIE0_RESET_PU_MASK),
	PINCTRL_CONF_DESC(62, REG_I2C_SDA_PU, PCIE1_RESET_PU_MASK),
};

static const struct econet_pinctrl_conf econet_pinctrl_pulldown_conf[] = {
	PINCTRL_CONF_DESC(0, REG_I2C_SDA_PD, UART1_TXD_PD_MASK),
	PINCTRL_CONF_DESC(1, REG_I2C_SDA_PD, UART1_RXD_PD_MASK),
	PINCTRL_CONF_DESC(2, REG_I2C_SDA_PD, I2C_SDA_PD_MASK),
	PINCTRL_CONF_DESC(3, REG_I2C_SDA_PD, I2C_SCL_PD_MASK),
	PINCTRL_CONF_DESC(4, REG_I2C_SDA_PD, SPI_CS_PD_MASK),
	PINCTRL_CONF_DESC(5, REG_I2C_SDA_PD, SPI_CLK_PD_MASK),
	PINCTRL_CONF_DESC(6, REG_I2C_SDA_PD, SPI_MOSI_PD_MASK),
	PINCTRL_CONF_DESC(7, REG_I2C_SDA_PD, SPI_MISO_PD_MASK),
	PINCTRL_CONF_DESC(8, REG_I2C_SDA_PD, MDIO_PD_MASK),
	PINCTRL_CONF_DESC(9, REG_I2C_SDA_PD, MDC_PD_MASK),
	PINCTRL_CONF_DESC(13, REG_GPIO_PD, BIT(0)),
	PINCTRL_CONF_DESC(14, REG_GPIO_PD, BIT(1)),
	PINCTRL_CONF_DESC(15, REG_GPIO_PD, BIT(2)),
	PINCTRL_CONF_DESC(16, REG_GPIO_PD, BIT(3)),
	PINCTRL_CONF_DESC(17, REG_GPIO_PD, BIT(4)),
	PINCTRL_CONF_DESC(18, REG_GPIO_PD, BIT(5)),
	PINCTRL_CONF_DESC(19, REG_GPIO_PD, BIT(6)),
	PINCTRL_CONF_DESC(20, REG_GPIO_PD, BIT(7)),
	PINCTRL_CONF_DESC(21, REG_GPIO_PD, BIT(8)),
	PINCTRL_CONF_DESC(22, REG_GPIO_PD, BIT(9)),
	PINCTRL_CONF_DESC(23, REG_GPIO_PD, BIT(10)),
	PINCTRL_CONF_DESC(24, REG_GPIO_PD, BIT(11)),
	PINCTRL_CONF_DESC(25, REG_GPIO_PD, BIT(12)),
	PINCTRL_CONF_DESC(26, REG_GPIO_PD, BIT(13)),
	PINCTRL_CONF_DESC(27, REG_GPIO_PD, BIT(14)),
	PINCTRL_CONF_DESC(28, REG_GPIO_PD, BIT(15)),
	PINCTRL_CONF_DESC(29, REG_GPIO_PD, BIT(16)),
	PINCTRL_CONF_DESC(30, REG_GPIO_PD, BIT(17)),
	PINCTRL_CONF_DESC(31, REG_GPIO_PD, BIT(18)),
	PINCTRL_CONF_DESC(32, REG_GPIO_PD, BIT(18)),
	PINCTRL_CONF_DESC(33, REG_GPIO_PD, BIT(20)),
	PINCTRL_CONF_DESC(34, REG_GPIO_PD, BIT(21)),
	PINCTRL_CONF_DESC(35, REG_GPIO_PD, BIT(22)),
	PINCTRL_CONF_DESC(36, REG_GPIO_PD, BIT(23)),
	PINCTRL_CONF_DESC(37, REG_GPIO_PD, BIT(24)),
	PINCTRL_CONF_DESC(38, REG_GPIO_PD, BIT(25)),
	PINCTRL_CONF_DESC(39, REG_GPIO_PD, BIT(26)),
	PINCTRL_CONF_DESC(40, REG_GPIO_PD, BIT(27)),
	PINCTRL_CONF_DESC(41, REG_GPIO_PD, BIT(28)),
	PINCTRL_CONF_DESC(42, REG_GPIO_PD, BIT(29)),
	PINCTRL_CONF_DESC(43, REG_GPIO_PD, BIT(30)),
	PINCTRL_CONF_DESC(44, REG_GPIO_PD, BIT(31)),
	PINCTRL_CONF_DESC(61, REG_I2C_SDA_PD, PCIE0_RESET_PD_MASK),
	PINCTRL_CONF_DESC(62, REG_I2C_SDA_PD, PCIE1_RESET_PD_MASK),
};

static const struct econet_pinctrl_conf econet_pinctrl_drive_e4_conf[] = {
	PINCTRL_CONF_DESC(0, REG_I2C_SDA_E4, UART1_TXD_E4_MASK),
	PINCTRL_CONF_DESC(1, REG_I2C_SDA_E4, UART1_RXD_E4_MASK),
	PINCTRL_CONF_DESC(2, REG_I2C_SDA_E4, I2C_SDA_E4_MASK),
	PINCTRL_CONF_DESC(3, REG_I2C_SDA_E4, I2C_SCL_E4_MASK),
	PINCTRL_CONF_DESC(4, REG_I2C_SDA_E4, SPI_CS_E4_MASK),
	PINCTRL_CONF_DESC(5, REG_I2C_SDA_E4, SPI_CLK_E4_MASK),
	PINCTRL_CONF_DESC(6, REG_I2C_SDA_E4, SPI_MOSI_E4_MASK),
	PINCTRL_CONF_DESC(7, REG_I2C_SDA_E4, SPI_MISO_E4_MASK),
	PINCTRL_CONF_DESC(8, REG_I2C_SDA_E4, MDIO_E4_MASK),
	PINCTRL_CONF_DESC(9, REG_I2C_SDA_E4, MDC_E4_MASK),
	PINCTRL_CONF_DESC(13, REG_GPIO_E4, BIT(0)),
	PINCTRL_CONF_DESC(14, REG_GPIO_E4, BIT(1)),
	PINCTRL_CONF_DESC(15, REG_GPIO_E4, BIT(2)),
	PINCTRL_CONF_DESC(16, REG_GPIO_E4, BIT(3)),
	PINCTRL_CONF_DESC(17, REG_GPIO_E4, BIT(4)),
	PINCTRL_CONF_DESC(18, REG_GPIO_E4, BIT(5)),
	PINCTRL_CONF_DESC(19, REG_GPIO_E4, BIT(6)),
	PINCTRL_CONF_DESC(20, REG_GPIO_E4, BIT(7)),
	PINCTRL_CONF_DESC(21, REG_GPIO_E4, BIT(8)),
	PINCTRL_CONF_DESC(22, REG_GPIO_E4, BIT(9)),
	PINCTRL_CONF_DESC(23, REG_GPIO_E4, BIT(10)),
	PINCTRL_CONF_DESC(24, REG_GPIO_E4, BIT(11)),
	PINCTRL_CONF_DESC(25, REG_GPIO_E4, BIT(12)),
	PINCTRL_CONF_DESC(26, REG_GPIO_E4, BIT(13)),
	PINCTRL_CONF_DESC(27, REG_GPIO_E4, BIT(14)),
	PINCTRL_CONF_DESC(28, REG_GPIO_E4, BIT(15)),
	PINCTRL_CONF_DESC(29, REG_GPIO_E4, BIT(16)),
	PINCTRL_CONF_DESC(30, REG_GPIO_E4, BIT(17)),
	PINCTRL_CONF_DESC(31, REG_GPIO_E4, BIT(18)),
	PINCTRL_CONF_DESC(32, REG_GPIO_E4, BIT(18)),
	PINCTRL_CONF_DESC(33, REG_GPIO_E4, BIT(20)),
	PINCTRL_CONF_DESC(34, REG_GPIO_E4, BIT(21)),
	PINCTRL_CONF_DESC(35, REG_GPIO_E4, BIT(22)),
	PINCTRL_CONF_DESC(36, REG_GPIO_E4, BIT(23)),
	PINCTRL_CONF_DESC(37, REG_GPIO_E4, BIT(24)),
	PINCTRL_CONF_DESC(38, REG_GPIO_E4, BIT(25)),
	PINCTRL_CONF_DESC(39, REG_GPIO_E4, BIT(26)),
	PINCTRL_CONF_DESC(40, REG_GPIO_E4, BIT(27)),
	PINCTRL_CONF_DESC(41, REG_GPIO_E4, BIT(28)),
	PINCTRL_CONF_DESC(42, REG_GPIO_E4, BIT(29)),
	PINCTRL_CONF_DESC(43, REG_GPIO_E4, BIT(30)),
	PINCTRL_CONF_DESC(44, REG_GPIO_E4, BIT(31)),
	PINCTRL_CONF_DESC(61, REG_I2C_SDA_E4, PCIE0_RESET_E4_MASK),
	PINCTRL_CONF_DESC(62, REG_I2C_SDA_E4, PCIE1_RESET_E4_MASK),
};

static const struct econet_pinctrl_conf econet_pinctrl_drive_e8_conf[] = {
	PINCTRL_CONF_DESC(0, REG_I2C_SDA_E8, UART1_TXD_E8_MASK),
	PINCTRL_CONF_DESC(1, REG_I2C_SDA_E8, UART1_RXD_E8_MASK),
	PINCTRL_CONF_DESC(2, REG_I2C_SDA_E8, I2C_SDA_E8_MASK),
	PINCTRL_CONF_DESC(3, REG_I2C_SDA_E8, I2C_SCL_E8_MASK),
	PINCTRL_CONF_DESC(4, REG_I2C_SDA_E8, SPI_CS_E8_MASK),
	PINCTRL_CONF_DESC(5, REG_I2C_SDA_E8, SPI_CLK_E8_MASK),
	PINCTRL_CONF_DESC(6, REG_I2C_SDA_E8, SPI_MOSI_E8_MASK),
	PINCTRL_CONF_DESC(7, REG_I2C_SDA_E8, SPI_MISO_E8_MASK),
	PINCTRL_CONF_DESC(8, REG_I2C_SDA_E8, MDIO_E8_MASK),
	PINCTRL_CONF_DESC(9, REG_I2C_SDA_E8, MDC_E8_MASK),
	PINCTRL_CONF_DESC(13, REG_GPIO_E8, BIT(0)),
	PINCTRL_CONF_DESC(14, REG_GPIO_E8, BIT(1)),
	PINCTRL_CONF_DESC(15, REG_GPIO_E8, BIT(2)),
	PINCTRL_CONF_DESC(16, REG_GPIO_E8, BIT(3)),
	PINCTRL_CONF_DESC(17, REG_GPIO_E8, BIT(4)),
	PINCTRL_CONF_DESC(18, REG_GPIO_E8, BIT(5)),
	PINCTRL_CONF_DESC(19, REG_GPIO_E8, BIT(6)),
	PINCTRL_CONF_DESC(20, REG_GPIO_E8, BIT(7)),
	PINCTRL_CONF_DESC(21, REG_GPIO_E8, BIT(8)),
	PINCTRL_CONF_DESC(22, REG_GPIO_E8, BIT(9)),
	PINCTRL_CONF_DESC(23, REG_GPIO_E8, BIT(10)),
	PINCTRL_CONF_DESC(24, REG_GPIO_E8, BIT(11)),
	PINCTRL_CONF_DESC(25, REG_GPIO_E8, BIT(12)),
	PINCTRL_CONF_DESC(26, REG_GPIO_E8, BIT(13)),
	PINCTRL_CONF_DESC(27, REG_GPIO_E8, BIT(14)),
	PINCTRL_CONF_DESC(28, REG_GPIO_E8, BIT(15)),
	PINCTRL_CONF_DESC(29, REG_GPIO_E8, BIT(16)),
	PINCTRL_CONF_DESC(30, REG_GPIO_E8, BIT(17)),
	PINCTRL_CONF_DESC(31, REG_GPIO_E8, BIT(18)),
	PINCTRL_CONF_DESC(32, REG_GPIO_E8, BIT(18)),
	PINCTRL_CONF_DESC(33, REG_GPIO_E8, BIT(20)),
	PINCTRL_CONF_DESC(34, REG_GPIO_E8, BIT(21)),
	PINCTRL_CONF_DESC(35, REG_GPIO_E8, BIT(22)),
	PINCTRL_CONF_DESC(36, REG_GPIO_E8, BIT(23)),
	PINCTRL_CONF_DESC(37, REG_GPIO_E8, BIT(24)),
	PINCTRL_CONF_DESC(38, REG_GPIO_E8, BIT(25)),
	PINCTRL_CONF_DESC(39, REG_GPIO_E8, BIT(26)),
	PINCTRL_CONF_DESC(40, REG_GPIO_E8, BIT(27)),
	PINCTRL_CONF_DESC(41, REG_GPIO_E8, BIT(28)),
	PINCTRL_CONF_DESC(42, REG_GPIO_E8, BIT(29)),
	PINCTRL_CONF_DESC(43, REG_GPIO_E8, BIT(30)),
	PINCTRL_CONF_DESC(44, REG_GPIO_E8, BIT(31)),
	PINCTRL_CONF_DESC(61, REG_I2C_SDA_E8, PCIE0_RESET_E8_MASK),
	PINCTRL_CONF_DESC(62, REG_I2C_SDA_E8, PCIE1_RESET_E8_MASK),
};

static int econet_convert_pin_to_reg_offset(struct pinctrl_dev *pctrl_dev,
					    struct pinctrl_gpio_range *range,
					    int pin)
{
	if (!range)
		range = pinctrl_find_gpio_range_from_pin_nolock(pctrl_dev,
								pin);
	if (!range)
		return -EINVAL;

	return pin - range->pin_base;
}

/* gpio callbacks */
static void econet_gpio_set(struct gpio_chip *chip, unsigned int gpio,
			    int value)
{
	struct econet_pinctrl *pinctrl = gpiochip_get_data(chip);
	u32 offset = gpio % ECONET_PIN_BANK_SIZE;
	u8 index = gpio / ECONET_PIN_BANK_SIZE;

	regmap_update_bits(pinctrl->regmap, pinctrl->gpiochip.data[index],
			   BIT(offset), value ? BIT(offset) : 0);
}

static int econet_gpio_get(struct gpio_chip *chip, unsigned int gpio)
{
	struct econet_pinctrl *pinctrl = gpiochip_get_data(chip);
	u32 val, pin = gpio % ECONET_PIN_BANK_SIZE;
	u8 index = gpio / ECONET_PIN_BANK_SIZE;
	int err;

	err = regmap_read(pinctrl->regmap,
			  pinctrl->gpiochip.data[index], &val);

	return err ? err : !!(val & BIT(pin));
}

static int econet_gpio_direction_output(struct gpio_chip *chip,
					unsigned int gpio, int value)
{
	int err;

	err = pinctrl_gpio_direction_output(chip, gpio);
	if (err)
		return err;

	econet_gpio_set(chip, gpio, value);

	return 0;
}

/* irq callbacks */
static void econet_irq_unmask(struct irq_data *data)
{
	u8 offset = data->hwirq % ECONET_REG_GPIOCTRL_NUM_PIN;
	u8 index = data->hwirq / ECONET_REG_GPIOCTRL_NUM_PIN;
	u32 mask = GENMASK(2 * offset + 1, 2 * offset);
	struct econet_pinctrl_gpiochip *gpiochip;
	struct econet_pinctrl *pinctrl;
	u32 val = BIT(2 * offset);

	gpiochip = irq_data_get_irq_chip_data(data);
	if (WARN_ON_ONCE(data->hwirq >= ARRAY_SIZE(gpiochip->irq_type)))
		return;

	pinctrl = container_of(gpiochip, struct econet_pinctrl, gpiochip);
	switch (gpiochip->irq_type[data->hwirq]) {
	case IRQ_TYPE_LEVEL_LOW:
		val = val << 1;
		fallthrough;
	case IRQ_TYPE_LEVEL_HIGH:
		regmap_update_bits(pinctrl->regmap, gpiochip->level[index],
				   mask, val);
		break;
	case IRQ_TYPE_EDGE_FALLING:
		val = val << 1;
		fallthrough;
	case IRQ_TYPE_EDGE_RISING:
		regmap_update_bits(pinctrl->regmap, gpiochip->edge[index],
				   mask, val);
		break;
	case IRQ_TYPE_EDGE_BOTH:
		regmap_set_bits(pinctrl->regmap, gpiochip->edge[index], mask);
		break;
	default:
		break;
	}
}

static void econet_irq_mask(struct irq_data *data)
{
	u8 offset = data->hwirq % ECONET_REG_GPIOCTRL_NUM_PIN;
	u8 index = data->hwirq / ECONET_REG_GPIOCTRL_NUM_PIN;
	u32 mask = GENMASK(2 * offset + 1, 2 * offset);
	struct econet_pinctrl_gpiochip *gpiochip;
	struct econet_pinctrl *pinctrl;

	gpiochip = irq_data_get_irq_chip_data(data);
	pinctrl = container_of(gpiochip, struct econet_pinctrl, gpiochip);

	regmap_clear_bits(pinctrl->regmap, gpiochip->level[index], mask);
	regmap_clear_bits(pinctrl->regmap, gpiochip->edge[index], mask);
}

static int econet_irq_type(struct irq_data *data, unsigned int type)
{
	struct econet_pinctrl_gpiochip *gpiochip;

	gpiochip = irq_data_get_irq_chip_data(data);
	if (data->hwirq >= ARRAY_SIZE(gpiochip->irq_type))
		return -EINVAL;

	if (type == IRQ_TYPE_PROBE) {
		if (gpiochip->irq_type[data->hwirq])
			return 0;

		type = IRQ_TYPE_EDGE_RISING | IRQ_TYPE_EDGE_FALLING;
	}
	gpiochip->irq_type[data->hwirq] = type & IRQ_TYPE_SENSE_MASK;

	return 0;
}

static irqreturn_t econet_irq_handler(int irq, void *data)
{
	struct econet_pinctrl *pinctrl = data;
	bool handled = false;
	int i;

	for (i = 0; i < ARRAY_SIZE(irq_status_regs); i++) {
		struct gpio_irq_chip *girq = &pinctrl->gpiochip.chip.irq;
		u32 regmap;
		unsigned long status;
		int irq;

		if (regmap_read(pinctrl->regmap, pinctrl->gpiochip.status[i],
				&regmap))
			continue;

		status = regmap;
		for_each_set_bit(irq, &status, ECONET_PIN_BANK_SIZE) {
			u32 offset = irq + i * ECONET_PIN_BANK_SIZE;

			generic_handle_irq(irq_find_mapping(girq->domain,
							    offset));
			regmap_write(pinctrl->regmap,
				     pinctrl->gpiochip.status[i], BIT(irq));
		}
		handled |= !!status;
	}

	return handled ? IRQ_HANDLED : IRQ_NONE;
}

static const struct irq_chip econet_gpio_irq_chip = {
	.name = "econet-gpio-irq",
	.irq_unmask = econet_irq_unmask,
	.irq_mask = econet_irq_mask,
	.irq_mask_ack = econet_irq_mask,
	.irq_set_type = econet_irq_type,
	.flags = IRQCHIP_SET_TYPE_MASKED | IRQCHIP_IMMUTABLE,
};

static int econet_pinctrl_add_gpiochip(struct econet_pinctrl *pinctrl,
				       struct platform_device *pdev)
{
	struct econet_pinctrl_gpiochip *chip = &pinctrl->gpiochip;
	struct gpio_chip *gc = &chip->chip;
	struct gpio_irq_chip *girq = &gc->irq;
	struct device *dev = &pdev->dev;
	int irq, err;

	chip->data = gpio_data_regs;
	chip->dir = gpio_dir_regs;
	chip->out = gpio_out_regs;
	chip->status = irq_status_regs;
	chip->level = irq_level_regs;
	chip->edge = irq_edge_regs;

	gc->parent = dev;
	gc->label = dev_name(dev);
	gc->request = gpiochip_generic_request;
	gc->free = gpiochip_generic_free;
	gc->direction_input = pinctrl_gpio_direction_input;
	gc->direction_output = econet_gpio_direction_output;
	gc->set = econet_gpio_set;
	gc->get = econet_gpio_get;
	gc->base = -1;
	gc->ngpio = ECONET_NUM_PINS;

	girq->default_type = IRQ_TYPE_NONE;
	girq->handler = handle_simple_irq;
	gpio_irq_chip_set_chip(girq, &econet_gpio_irq_chip);

	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
		return irq;

	err = devm_request_irq(dev, irq, econet_irq_handler, IRQF_SHARED,
			       dev_name(dev), pinctrl);
	if (err) {
		dev_err(dev, "error requesting irq %d: %d\n", irq, err);
		return err;
	}

	return devm_gpiochip_add_data(dev, gc, pinctrl);
}

/* pinmux callbacks */
static int econet_pinmux_set_mux(struct pinctrl_dev *pctrl_dev,
				 unsigned int selector,
				 unsigned int group)
{
	struct econet_pinctrl *pinctrl = pinctrl_dev_get_drvdata(pctrl_dev);
	const struct econet_pinctrl_func *func;
	struct function_desc *desc;
	struct group_desc *grp;
	int i;

	desc = pinmux_generic_get_function(pctrl_dev, selector);
	if (!desc)
		return -EINVAL;

	grp = pinctrl_generic_get_group(pctrl_dev, group);
	if (!grp)
		return -EINVAL;

	dev_err(pctrl_dev->dev, "enable function %s group %s\n",
		desc->func.name, grp->grp.name);

	func = desc->data;
	for (i = 0; i < func->group_size; i++) {
		const struct econet_pinctrl_func_group *group;
		int j;

		group = &func->groups[i];
		if (strcmp(group->name, grp->grp.name))
			continue;

		for (j = 0; j < group->regmap_size; j++) {
			switch (group->regmap[j].mux) {
			case ECONET_FUNC_PWM_EXT_MUX:
			case ECONET_FUNC_MUX:
				regmap_update_bits(pinctrl->regmap,
						   group->regmap[j].offset,
						   group->regmap[j].mask,
						   group->regmap[j].val);
				break;
			default:
				regmap_update_bits(pinctrl->chip_scu,
						   group->regmap[j].offset,
						   group->regmap[j].mask,
						   group->regmap[j].val);
				break;
			}
		}
		return 0;
	}

	return -EINVAL;
}

static int econet_pinmux_set_direction(struct pinctrl_dev *pctrl_dev,
				       struct pinctrl_gpio_range *range,
				       unsigned int p, bool input)
{
	struct econet_pinctrl *pinctrl = pinctrl_dev_get_drvdata(pctrl_dev);
	u32 mask, index;
	int err, pin;

	pin = econet_convert_pin_to_reg_offset(pctrl_dev, range, p);
	if (pin < 0)
		return pin;

	/* set output enable */
	mask = BIT(pin % ECONET_PIN_BANK_SIZE);
	index = pin / ECONET_PIN_BANK_SIZE;
	err = regmap_update_bits(pinctrl->regmap, pinctrl->gpiochip.out[index],
				 mask, !input ? mask : 0);
	if (err)
		return err;

	/* set direction */
	mask = BIT(2 * (pin % ECONET_REG_GPIOCTRL_NUM_PIN));
	index = pin / ECONET_REG_GPIOCTRL_NUM_PIN;
	return regmap_update_bits(pinctrl->regmap,
				  pinctrl->gpiochip.dir[index], mask,
				  !input ? mask : 0);
}

static const struct pinmux_ops econet_pmxops = {
	.get_functions_count = pinmux_generic_get_function_count,
	.get_function_name = pinmux_generic_get_function_name,
	.get_function_groups = pinmux_generic_get_function_groups,
	.gpio_set_direction = econet_pinmux_set_direction,
	.set_mux = econet_pinmux_set_mux,
	.strict = true,
};

/* pinconf callbacks */
static const struct econet_pinctrl_reg *
econet_pinctrl_get_conf_reg(const struct econet_pinctrl_conf *conf,
			    int conf_size, int pin)
{
	int i;

	for (i = 0; i < conf_size; i++) {
		if (conf[i].pin == pin)
			return &conf[i].reg;
	}

	return NULL;
}

static int econet_pinctrl_get_conf(struct econet_pinctrl *pinctrl,
				   const struct econet_pinctrl_conf *conf,
				   int conf_size, int pin, u32 *val)
{
	const struct econet_pinctrl_reg *reg;

	reg = econet_pinctrl_get_conf_reg(conf, conf_size, pin);
	if (!reg)
		return -EINVAL;

	if (regmap_read(pinctrl->chip_scu, reg->offset, val))
		return -EINVAL;

	*val = (*val & reg->mask) >> __ffs(reg->mask);

	return 0;
}

static int econet_pinctrl_set_conf(struct econet_pinctrl *pinctrl,
				   const struct econet_pinctrl_conf *conf,
				   int conf_size, int pin, u32 val)
{
	const struct econet_pinctrl_reg *reg = NULL;

	reg = econet_pinctrl_get_conf_reg(conf, conf_size, pin);
	if (!reg)
		return -EINVAL;


	if (regmap_update_bits(pinctrl->chip_scu, reg->offset, reg->mask,
			       val << __ffs(reg->mask)))
		return -EINVAL;

	return 0;
}

#define econet_pinctrl_get_pullup_conf(pinctrl, pin, val)			\
	econet_pinctrl_get_conf((pinctrl), econet_pinctrl_pullup_conf,		\
				ARRAY_SIZE(econet_pinctrl_pullup_conf),		\
				(pin), (val))
#define econet_pinctrl_get_pulldown_conf(pinctrl, pin, val)			\
	econet_pinctrl_get_conf((pinctrl), econet_pinctrl_pulldown_conf,	\
				ARRAY_SIZE(econet_pinctrl_pulldown_conf),	\
				(pin), (val))
#define econet_pinctrl_get_drive_e4_conf(pinctrl, pin, val)			\
	econet_pinctrl_get_conf((pinctrl), econet_pinctrl_drive_e4_conf,	\
				ARRAY_SIZE(econet_pinctrl_drive_e4_conf),	\
				(pin), (val))
#define econet_pinctrl_get_drive_e8_conf(pinctrl, pin, val)			\
	econet_pinctrl_get_conf((pinctrl), econet_pinctrl_drive_e8_conf,	\
				ARRAY_SIZE(econet_pinctrl_drive_e8_conf),	\
				(pin), (val))
#define econet_pinctrl_get_pcie_rst_od_conf(pinctrl, pin, val)			\
	econet_pinctrl_get_conf((pinctrl), econet_pinctrl_pcie_rst_od_conf,	\
				ARRAY_SIZE(econet_pinctrl_pcie_rst_od_conf),	\
				(pin), (val))
#define econet_pinctrl_set_pullup_conf(pinctrl, pin, val)			\
	econet_pinctrl_set_conf((pinctrl), econet_pinctrl_pullup_conf,		\
				ARRAY_SIZE(econet_pinctrl_pullup_conf),		\
				(pin), (val))
#define econet_pinctrl_set_pulldown_conf(pinctrl, pin, val)			\
	econet_pinctrl_set_conf((pinctrl), econet_pinctrl_pulldown_conf,	\
				ARRAY_SIZE(econet_pinctrl_pulldown_conf),	\
				(pin), (val))
#define econet_pinctrl_set_drive_e4_conf(pinctrl, pin, val)			\
	econet_pinctrl_set_conf((pinctrl), econet_pinctrl_drive_e4_conf,	\
				ARRAY_SIZE(econet_pinctrl_drive_e4_conf),	\
				(pin), (val))
#define econet_pinctrl_set_drive_e8_conf(pinctrl, pin, val)			\
	econet_pinctrl_set_conf((pinctrl), econet_pinctrl_drive_e8_conf,	\
				ARRAY_SIZE(econet_pinctrl_drive_e8_conf),	\
				(pin), (val))

static int econet_pinconf_get_direction(struct pinctrl_dev *pctrl_dev, u32 p)
{
	struct econet_pinctrl *pinctrl = pinctrl_dev_get_drvdata(pctrl_dev);
	u32 val, mask;
	int err, pin;
	u8 index;

	pin = econet_convert_pin_to_reg_offset(pctrl_dev, NULL, p);
	if (pin < 0)
		return pin;

	index = pin / ECONET_REG_GPIOCTRL_NUM_PIN;
	err = regmap_read(pinctrl->regmap, pinctrl->gpiochip.dir[index], &val);
	if (err)
		return err;

	mask = BIT(2 * (pin % ECONET_REG_GPIOCTRL_NUM_PIN));
	return val & mask ? PIN_CONFIG_OUTPUT_ENABLE : PIN_CONFIG_INPUT_ENABLE;
}

static int econet_pinconf_get(struct pinctrl_dev *pctrl_dev,
			      unsigned int pin, unsigned long *config)
{
	struct econet_pinctrl *pinctrl = pinctrl_dev_get_drvdata(pctrl_dev);
	enum pin_config_param param = pinconf_to_config_param(*config);
	u32 arg;

	switch (param) {
	case PIN_CONFIG_BIAS_PULL_DOWN:
	case PIN_CONFIG_BIAS_DISABLE:
	case PIN_CONFIG_BIAS_PULL_UP: {
		u32 pull_up, pull_down;

		if (econet_pinctrl_get_pullup_conf(pinctrl, pin, &pull_up) ||
		    econet_pinctrl_get_pulldown_conf(pinctrl, pin, &pull_down))
			return -EINVAL;

		if (param == PIN_CONFIG_BIAS_PULL_UP &&
		    !(pull_up && !pull_down))
			return -EINVAL;
		else if (param == PIN_CONFIG_BIAS_PULL_DOWN &&
			 !(pull_down && !pull_up))
			return -EINVAL;
		else if (pull_up || pull_down)
			return -EINVAL;

		arg = 1;
		break;
	}
	case PIN_CONFIG_DRIVE_STRENGTH: {
		u32 e2, e4;

		if (econet_pinctrl_get_drive_e4_conf(pinctrl, pin, &e2) ||
		    econet_pinctrl_get_drive_e8_conf(pinctrl, pin, &e4))
			return -EINVAL;

		arg = e4 << 1 | e2;
		break;
	}
	case PIN_CONFIG_OUTPUT_ENABLE:
	case PIN_CONFIG_INPUT_ENABLE:
		arg = econet_pinconf_get_direction(pctrl_dev, pin);
		if (arg != param)
			return -EINVAL;

		arg = 1;
		break;
	default:
		return -EOPNOTSUPP;
	}

	*config = pinconf_to_config_packed(param, arg);

	return 0;
}

static int econet_pinconf_set_pin_value(struct pinctrl_dev *pctrl_dev,
					unsigned int p, bool value)
{
	struct econet_pinctrl *pinctrl = pinctrl_dev_get_drvdata(pctrl_dev);
	int pin;

	pin = econet_convert_pin_to_reg_offset(pctrl_dev, NULL, p);
	if (pin < 0)
		return pin;

	econet_gpio_set(&pinctrl->gpiochip.chip, pin, value);

	return 0;
}

static int econet_pinconf_set(struct pinctrl_dev *pctrl_dev,
			      unsigned int pin, unsigned long *configs,
			      unsigned int num_configs)
{
	struct econet_pinctrl *pinctrl = pinctrl_dev_get_drvdata(pctrl_dev);
	int i;

	for (i = 0; i < num_configs; i++) {
		u32 param = pinconf_to_config_param(configs[i]);
		u32 arg = pinconf_to_config_argument(configs[i]);

		switch (param) {
		case PIN_CONFIG_BIAS_DISABLE:
			econet_pinctrl_set_pulldown_conf(pinctrl, pin, 0);
			econet_pinctrl_set_pullup_conf(pinctrl, pin, 0);
			break;
		case PIN_CONFIG_BIAS_PULL_UP:
			econet_pinctrl_set_pulldown_conf(pinctrl, pin, 0);
			econet_pinctrl_set_pullup_conf(pinctrl, pin, 1);
			break;
		case PIN_CONFIG_BIAS_PULL_DOWN:
			econet_pinctrl_set_pulldown_conf(pinctrl, pin, 1);
			econet_pinctrl_set_pullup_conf(pinctrl, pin, 0);
			break;
		case PIN_CONFIG_DRIVE_STRENGTH: {
			u32 e4 = 0, e8 = 0;

			switch (arg) {
			case MTK_DRIVE_4mA:
				break;
			case MTK_DRIVE_8mA:
				e4 = 1;
				break;
			case MTK_DRIVE_12mA:
				e8 = 1;
				break;
			case MTK_DRIVE_16mA:
				e4 = 1;
				e8 = 1;
				break;
			default:
				return -EINVAL;
			}

			econet_pinctrl_set_drive_e4_conf(pinctrl, pin, e4);
			econet_pinctrl_set_drive_e8_conf(pinctrl, pin, e8);
			break;
		}
		case PIN_CONFIG_OUTPUT_ENABLE:
		case PIN_CONFIG_INPUT_ENABLE:
		case PIN_CONFIG_OUTPUT: {
			bool input = param == PIN_CONFIG_INPUT_ENABLE;
			int err;

			err = econet_pinmux_set_direction(pctrl_dev, NULL, pin,
							  input);
			if (err)
				return err;

			if (param == PIN_CONFIG_OUTPUT) {
				err = econet_pinconf_set_pin_value(pctrl_dev,
								   pin, !!arg);
				if (err)
					return err;
			}
			break;
		}
		default:
			return -EOPNOTSUPP;
		}
	}

	return 0;
}

static int econet_pinconf_group_get(struct pinctrl_dev *pctrl_dev,
				    unsigned int group, unsigned long *config)
{
	u32 cur_config = 0;
	int i;

	for (i = 0; i < econet_pinctrl_groups[group].npins; i++) {
		if (econet_pinconf_get(pctrl_dev,
				       econet_pinctrl_groups[group].pins[i],
				       config))
			return -EOPNOTSUPP;

		if (i && cur_config != *config)
			return -EOPNOTSUPP;

		cur_config = *config;
	}

	return 0;
}

static int econet_pinconf_group_set(struct pinctrl_dev *pctrl_dev,
				    unsigned int group, unsigned long *configs,
				    unsigned int num_configs)
{
	int i;

	for (i = 0; i < econet_pinctrl_groups[group].npins; i++) {
		int err;

		err = econet_pinconf_set(pctrl_dev,
					 econet_pinctrl_groups[group].pins[i],
					 configs, num_configs);
		if (err)
			return err;
	}

	return 0;
}

static const struct pinconf_ops econet_confops = {
	.is_generic = true,
	.pin_config_get = econet_pinconf_get,
	.pin_config_set = econet_pinconf_set,
	.pin_config_group_get = econet_pinconf_group_get,
	.pin_config_group_set = econet_pinconf_group_set,
	.pin_config_config_dbg_show = pinconf_generic_dump_config,
};

static const struct pinctrl_ops econet_pctlops = {
	.get_groups_count = pinctrl_generic_get_group_count,
	.get_group_name = pinctrl_generic_get_group_name,
	.get_group_pins = pinctrl_generic_get_group_pins,
	.dt_node_to_map = pinconf_generic_dt_node_to_map_all,
	.dt_free_map = pinconf_generic_dt_free_map,
};

static struct pinctrl_desc econet_pinctrl_desc = {
	.name = KBUILD_MODNAME,
	.owner = THIS_MODULE,
	.pctlops = &econet_pctlops,
	.pmxops = &econet_pmxops,
	.confops = &econet_confops,
	.pins = econet_pinctrl_pins,
	.npins = ARRAY_SIZE(econet_pinctrl_pins),
};

static int econet_pinctrl_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct econet_pinctrl *pinctrl;
	struct regmap *map;
	int err, i;

	pinctrl = devm_kzalloc(dev, sizeof(*pinctrl), GFP_KERNEL);
	if (!pinctrl)
		return -ENOMEM;
	
	pinctrl->regmap = device_node_to_regmap(dev->parent->of_node);
	if (IS_ERR(pinctrl->regmap))
		return PTR_ERR(pinctrl->regmap);

	map = syscon_regmap_lookup_by_compatible("econet,en751221-chip-scu");
	if (IS_ERR(map))
		return PTR_ERR(map);

	pinctrl->chip_scu = map;

	err = devm_pinctrl_register_and_init(dev, &econet_pinctrl_desc,
					     pinctrl, &pinctrl->ctrl);

	if (err)
		return err;

	/* build pin groups */
	for (i = 0; i < ARRAY_SIZE(econet_pinctrl_groups); i++) {
		const struct pingroup *grp = &econet_pinctrl_groups[i];

		err = pinctrl_generic_add_group(pinctrl->ctrl, grp->name,
						grp->pins, grp->npins,
						(void *)grp);
		if (err < 0) {
			dev_err(&pdev->dev, "Failed to register group %s\n",
				grp->name);
			return err;
		}
	}

	/* build functions */
	for (i = 0; i < ARRAY_SIZE(econet_pinctrl_funcs); i++) {
		const struct econet_pinctrl_func *func;

		func = &econet_pinctrl_funcs[i];
		err = pinmux_generic_add_function(pinctrl->ctrl,
						  func->desc.func.name,
						  func->desc.func.groups,
						  func->desc.func.ngroups,
						  (void *)func);
		if (err < 0) {
			dev_err(dev, "Failed to register function %s\n",
				func->desc.func.name);
			return err;
		}
	}

	err = pinctrl_enable(pinctrl->ctrl);
	if (err)
		return err;

	/* build gpio-chip */
	return econet_pinctrl_add_gpiochip(pinctrl, pdev);
}

static const struct of_device_id econet_pinctrl_of_match[] = {
	{ .compatible = "econet,en751221-pinctrl" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, econet_pinctrl_of_match);

static struct platform_driver econet_pinctrl_driver = {
	.probe = econet_pinctrl_probe,
	.driver = {
		.name = "pinctrl-econet",
		.of_match_table = econet_pinctrl_of_match,
	},
};
module_platform_driver(econet_pinctrl_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lorenzo Bianconi <lorenzo@kernel.org>");
MODULE_AUTHOR("Benjamin Larsson <benjamin.larsson@genexis.eu>");
MODULE_AUTHOR("Markus Gothe <markus.gothe@genexis.eu>");
MODULE_DESCRIPTION("Pinctrl driver for Econet EN7512x SoCs");
