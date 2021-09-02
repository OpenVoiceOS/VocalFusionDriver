/*
 * =====================================================================================
 *
 *    Description:  XMOS VocalFusion 3510 soundcard driver - based on simple loader
 *
 *        Version:  0.1.0
 *        Created:  2021-09-01
 *       Revision:  none
 *       Compiler:  gcc
 *
 *           Mods:  Peter Steenbergen (p.steenbergen@j1nx.nl)
 *    Orig Author:  Huan Truong (htruong@tnhh.net), originally written by Paul Creaser
 *
 * =====================================================================================
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kmod.h>
#include <linux/platform_device.h>
#include <sound/simple_card.h>
#include <linux/delay.h>

void device_release_callback(struct device *dev) { /* do nothing */ };

#define CARD_PLATFORM_STR   "fe203000.i2s"
#define SND_SOC_DAIFMT_CBS_FLAG SND_SOC_DAIFMT_CBS_CFS

static struct asoc_simple_card_info snd_rpi_simple_card_info = {
	.card = "snd_rpi_simple_card", // -> snd_soc_card.name
	.name = "simple-card_codec_link", // -> snd_soc_dai_link.name
	.codec = "snd-soc-dummy", // -> snd_soc_dai_link.codec_name
	.platform = CARD_PLATFORM_STR,
	.daifmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_FLAG,
	.cpu_dai = {
		.name = CARD_PLATFORM_STR, // -> snd_soc_dai_link.cpu_dai_name
		.sysclk = 0
	},
	.codec_dai = {
		.name = "snd-soc-dummy-dai", // -> snd_soc_dai_link.codec_dai_name
		.sysclk = 0
	}
};

static struct platform_device snd_rpi_simple_card_device = {
	.name = "asoc-simple-card", //module alias
	.id = 0,
	.num_resources = 0,
	.dev = {
		.release = &device_release_callback,
		.platform_data = &snd_rpi_simple_card_info, // *HACK ALERT*
	}
};

static int vocalfusion_soundcard_probe(struct platform_device *pdev)
{
	const char *dmaengine = "bcm2708-dmaengine"; //module name
	int ret;

	mclk = devm_clk_get(&pdev->dev, NULL);
	
	if (of_property_read_u32(dev->of_node, "clock-frequency", &rate))
		return NULL;
	clk_set_rate(mclk, rate);
	clk_prepare_enable(mclk);
	
	printk(KERN_ALERT "GPIO4_CLK: %lu Hz\n", clk_get_rate(mclk));
	parent_clk = clk_get_parent(mclk);
	if (!(IS_ERR(parent_clk))) {
		printk(KERN_ALERT "Parent clock name: %s\n", __clk_get_name(parent_clk));
		printk(KERN_ALERT "Parent clock rate: %lu Hz\n", clk_get_rate(parent_clk));
	}

	ret = request_module(dmaengine);
	pr_alert("request module load '%s': %d\n",dmaengine, ret);
	ret = platform_device_register(&snd_rpi_simple_card_device);
	pr_alert("register platform device '%s': %d\n",snd_rpi_simple_card_device.name, ret);
	return 0;
}

static int vocalfusion_soundcard_remove(struct platform_device *pdev)
{
	platform_device_unregister(&snd_rpi_simple_card_device);
	clk_disable_unprepare(pp->clk);
	return 0;
}

static const struct of_device_id vocalfusion_soundcard_of_match[] = {
	{ .compatible = "xmos,vocalfusion-soundcard", },
	{},
};

MODULE_DEVICE_TABLE(of, vocalfusion_soundcard_of_match);

static struct platform_driver vocalfusion_soundcard_driver = {
	.driver = {
		.name   = "vocalfusion-driver",
		.owner  = THIS_MODULE,
		.of_match_table = vocalfusion_soundcard_of_match,
	},
	.probe = vocalfusion_soundcard_probe,
	.remove = vocalfusion_soundcard_remove,
};

module_platform_driver(vocalfusion_soundcard_driver);
MODULE_DESCRIPTION("XMOS VocalFusion I2S Driver");
MODULE_AUTHOR("OpenVoiceOS");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:vocalfusion-soundcard");