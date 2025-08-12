// SPDX-License-Identifier: GPL-2.0-only

#include <linux/delay.h>
#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/reset-controller.h>
#include <dt-bindings/clock/en751221-clk.h>
#include <dt-bindings/reset/econet,en751221-reset.h>

#define RST_NR_PER_BANK			32


#define REG_NP_SCU_PCIC			0x88
#define REG_NP_SCU_SSR3			0x94
#define REG_NP_SCU_SSTR			0x9c


struct en_clk_desc {
	int id;
	const char *name;
	u32 base_reg;
	u8 base_bits;
	u8 base_shift;
	union {
		const unsigned int *base_values;
		unsigned int base_value;
	};
	size_t n_base_values;

	u16 div_reg;
	u8 div_bits;
	u8 div_shift;
	u16 div_val0;
	u8 div_step;
	u8 div_offset;
};

struct en_clk_gate {
	void __iomem *base;
	struct clk_hw hw;
};

struct en_rst_data {
	const u16 *bank_ofs;
	const u16 *idx_map;
	void __iomem *base;
	struct reset_controller_dev rcdev;
};

struct en_clk_soc_data {
	struct {
		const struct en_clk_desc *desc;
		u16 size;
	} fixed_rate;
	const struct clk_ops pcie_ops;
	struct {
		const u16 *bank_ofs;
		const u16 *idx_map;
		u16 idx_map_nr;
	} reset;
	int (*hw_init)(struct platform_device *pdev, void __iomem *np_base);
};

static const u32 gsw_base[] = { 500000000, 250000000, 400000000, 200000000 };

static const struct en_clk_desc en751221_base_clks[] = {
	{
		.id = EN751221_CLK_GSW,
		.name = "gsw",

		.base_reg = REG_NP_SCU_SSR3,
		.base_bits = 2,
		.base_shift = 8,
		.base_values = gsw_base,
		.n_base_values = ARRAY_SIZE(gsw_base),

		.div_bits = 0,
		.div_shift = 0,
		.div_step = 0,
		.div_offset = 0,
	},
};

static const u16 en7581_rst_ofs[] = {
	REG_RESET_CONTROL2,
	REG_RESET_CONTROL1,
};

static const u16 en75_rst_map[] = {
	/* RST_CTRL2 */
	[EN75_XPON_PHY_RST]		= 0,
	[EN75_GFAST_RST]		= 1,
	[EN75_CPU_TIMER2_RST]		= 2,
	[EN75_UART3_RST]		= 3,
	[EN75_UART4_RST]		= 4,
	[EN75_UART5_RST]		= 5,
	[EN75_I2C2_RST]			= 6,
	[EN75_XSI_MAC_RST]		= 7,
	[EN75_XSI_PHY_RST]		= 8,

	/* RST_CTRL1 */
	[EN75_PCM1_ZSI_ISI_RST]		= RST_NR_PER_BANK + 0,
	[EN75_FE_PDMA_RST]		= RST_NR_PER_BANK + 1,
	[EN75_FE_QDMA_RST]		= RST_NR_PER_BANK + 2,
	[EN75_UNZIP_RST]		= RST_NR_PER_BANK + 3,
	[EN75_PCM2_RST]			= RST_NR_PER_BANK + 4,
	[EN75_PTM_MAC_RST]		= RST_NR_PER_BANK + 5,
	[EN75_CRYPTO_RST]		= RST_NR_PER_BANK + 6,
	[EN75_SAR_RST]			= RST_NR_PER_BANK + 7,
	[EN75_TIMER_RST]		= RST_NR_PER_BANK + 8,
	[EN75_INTC_RST]			= RST_NR_PER_BANK + 9,
	[EN75_BONDING_RST]		= RST_NR_PER_BANK + 10,
	[EN75_PCM1_RST]			= RST_NR_PER_BANK + 11,
	[EN75_UART_RST]			= RST_NR_PER_BANK + 12,
	[EN75_GPIO_RST]			= RST_NR_PER_BANK + 13,
	[EN75_GDMA_RST]			= RST_NR_PER_BANK + 14,
	[EN75_I2C_MASTER_RST]		= RST_NR_PER_BANK + 16,
	[EN75_PCM2_ZSI_ISI_RST]		= RST_NR_PER_BANK + 17,
	[EN75_SFC_RST]			= RST_NR_PER_BANK + 18,
	[EN75_UART2_RST]		= RST_NR_PER_BANK + 19,
	[EN75_GDMP_RST]			= RST_NR_PER_BANK + 20,
	[EN75_FE_RST]			= RST_NR_PER_BANK + 21,
	[EN75_USB_HOST_P0_RST]		= RST_NR_PER_BANK + 22,
	[EN75_GSW_RST]			= RST_NR_PER_BANK + 23,
	[EN75_SFC2_PCM_RST]		= RST_NR_PER_BANK + 25,
	[EN75_PCIE0_RST]		= RST_NR_PER_BANK + 26,
	[EN75_PCIE1_RST]		= RST_NR_PER_BANK + 27,
	[EN75_CPU_TIMER_RST]		= RST_NR_PER_BANK + 28,
	[EN75_PCIE_HB_RST]		= RST_NR_PER_BANK + 29,
	[EN75_SIMIF_RST]		= RST_NR_PER_BANK + 30,
	[EN75_XPON_MAC_RST]		= RST_NR_PER_BANK + 31,
};

static unsigned int en75_get_base_rate(const struct en_clk_desc *desc,
					 void __iomem *base)
{
	u32 val;

	if (!desc->base_bits)
		return desc->base_value;

	val = readl(base + desc->base_reg);
	val >>= desc->base_shift;
	val &= (1 << desc->base_bits) - 1;

	if (val >= desc->n_base_values)
		return 0;

	return desc->base_values[val];
}

static u32 en75_get_div(const struct en_clk_desc *desc, void __iomem *base)
{
	u32 reg, val;

	if (!desc->div_bits)
		return 1;

	reg = desc->div_reg ? desc->div_reg : desc->base_reg;
	val = readl(base + reg);
	val >>= desc->div_shift;
	val &= (1 << desc->div_bits) - 1;

	if (!val && desc->div_val0)
		return desc->div_val0;

	return (val + desc->div_offset) * desc->div_step;
}




static void en7523_register_clocks(struct device *dev, struct clk_hw_onecell_data *clk_data,
				   void __iomem *base, void __iomem *np_base)
{
	const struct en_clk_soc_data *soc_data = device_get_match_data(dev);
	struct clk_hw *hw;
	u32 rate;
	int i;

	for (i = 0; i < soc_data->fixed_rate.size; i++) {
		const struct en_clk_desc *desc = &soc_data->fixed_rate.desc[i];

		rate = en75_get_base_rate(desc, base);
		rate /= en75_get_div(desc, base);

		hw = clk_hw_register_fixed_rate(dev, desc->name, NULL, 0, rate);
		if (IS_ERR(hw)) {
			pr_err("Failed to register clk %s: %ld\n",
			       desc->name, PTR_ERR(hw));
			continue;
		}

		clk_data->hws[desc->id] = hw;
	}

	hw = en7523_register_pcie_clk(dev, np_base);
	clk_data->hws[EN7523_CLK_PCIE] = hw;

	clk_data->num = EN7523_NUM_CLOCKS;
}

static int en7523_reset_update(struct reset_controller_dev *rcdev,
			       unsigned long id, bool assert)
{
	struct en_rst_data *rst_data;
	void __iomem *addr;
	u32 val;

	rst_data = container_of(rcdev, struct en_rst_data, rcdev);
	addr = rst_data->base + rst_data->bank_ofs[id / RST_NR_PER_BANK];

	val = readl(addr);
	if (assert)
		val |= BIT(id % RST_NR_PER_BANK);
	else
		val &= ~BIT(id % RST_NR_PER_BANK);
	writel(val, addr);

	return 0;
}

static int en75_reset_assert(struct reset_controller_dev *rcdev,
			       unsigned long id)
{
	return en75_reset_update(rcdev, id, true);
}

static int en75_reset_deassert(struct reset_controller_dev *rcdev,
				 unsigned long id)
{
	return en7523_reset_update(rcdev, id, false);
}

static int en75_reset_status(struct reset_controller_dev *rcdev,
			       unsigned long id)
{
	struct en_rst_data *rst_data;
	void __iomem *addr;

	rst_data = container_of(rcdev, struct en_rst_data, rcdev);
	addr = rst_data->base + rst_data->bank_ofs[id / RST_NR_PER_BANK];

	return !!(readl(addr) & BIT(id % RST_NR_PER_BANK));
}

static int en75_reset_xlate(struct reset_controller_dev *rcdev,
			      const struct of_phandle_args *reset_spec)
{
	struct en_rst_data *rst_data;

	rst_data = container_of(rcdev, struct en_rst_data, rcdev);
	if (reset_spec->args[0] >= rcdev->nr_resets)
		return -EINVAL;

	return rst_data->idx_map[reset_spec->args[0]];
}

static const struct reset_control_ops en7523_reset_ops = {
	.assert = en7523_reset_assert,
	.deassert = en7523_reset_deassert,
	.status = en7523_reset_status,
};

static int en7523_reset_register(struct device *dev, void __iomem *base,
				 const struct en_clk_soc_data *soc_data)
{
	struct en_rst_data *rst_data;

	/* no reset lines available */
	if (!soc_data->reset.idx_map_nr)
		return 0;

	rst_data = devm_kzalloc(dev, sizeof(*rst_data), GFP_KERNEL);
	if (!rst_data)
		return -ENOMEM;

	rst_data->bank_ofs = soc_data->reset.bank_ofs;
	rst_data->idx_map = soc_data->reset.idx_map;
	rst_data->base = base;

	rst_data->rcdev.nr_resets = soc_data->reset.idx_map_nr;
	rst_data->rcdev.of_xlate = en7523_reset_xlate;
	rst_data->rcdev.ops = &en7523_reset_ops;
	rst_data->rcdev.of_node = dev->of_node;
	rst_data->rcdev.of_reset_n_cells = 1;
	rst_data->rcdev.owner = THIS_MODULE;
	rst_data->rcdev.dev = dev;

	return devm_reset_controller_register(dev, &rst_data->rcdev);
}

static int en75_clk_probe(struct platform_device *pdev)
{
	struct device_node *node = pdev->dev.of_node;
	const struct en_clk_soc_data *soc_data;
	struct clk_hw_onecell_data *clk_data;
	void __iomem *base, *np_base;
	int r;

	base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(base))
		return PTR_ERR(base);

	np_base = devm_platform_ioremap_resource(pdev, 1);
	if (IS_ERR(np_base))
		return PTR_ERR(np_base);

	soc_data = device_get_match_data(&pdev->dev);
	if (soc_data->hw_init) {
		r = soc_data->hw_init(pdev, np_base);
		if (r)
			return r;
	}

	clk_data = devm_kzalloc(&pdev->dev,
				struct_size(clk_data, hws, EN7523_NUM_CLOCKS),
				GFP_KERNEL);
	if (!clk_data)
		return -ENOMEM;

	en7523_register_clocks(&pdev->dev, clk_data, base, np_base);

	r = of_clk_add_hw_provider(node, of_clk_hw_onecell_get, clk_data);
	if (r) {
		dev_err(&pdev->dev,
			"could not register clock provider: %s: %d\n",
			pdev->name, r);
		return r;
	}

	r = en7523_reset_register(&pdev->dev, np_base, soc_data);
	if (r) {
		dev_err(&pdev->dev,
			"could not register reset controller: %s: %d\n",
			pdev->name, r);
		of_clk_del_provider(node);
		return r;
	}

	return 0;
}


static const struct en_clk_soc_data en7581_data = {
	.fixed_rate = {
		.desc = en7581_base_clks,
		.size = ARRAY_SIZE(en7581_base_clks),
	},
	.reset = {
		.bank_ofs = en7581_rst_ofs,
		.idx_map = en7581_rst_map,
		.idx_map_nr = ARRAY_SIZE(en7581_rst_map),
	},
	.hw_init = en7581_clk_hw_init,
};

static const struct of_device_id of_match_clk_en7523[] = {
	{ .compatible = "econet,en751221-scu", .data = &en751221_data },
	{ /* sentinel */ }
};

static struct platform_driver clk_en75_drv = {
	.probe = en75_clk_probe,
	.driver = {
		.name = "clk-en75",
		.of_match_table = of_match_clk_en7523,
		.suppress_bind_attrs = true,
	},
};

static int __init clk_en7523_init(void)
{
	return platform_driver_register(&clk_en7523_drv);
}

arch_initcall(clk_en7523_init);
