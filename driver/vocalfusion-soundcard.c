/*
 * =====================================================================================
 *
 *    Description:  XMOS VocalFusion soundcard driver - based on simple loader
 *
 *     Conversion:  Peter Steenbergen (p.steenbergen@j1nx.nl)
 *    Orig Author:  Huan Truong (htruong@tnhh.net), originally written by Paul Creaser
 *
 * =====================================================================================
 */
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kmod.h>
#include <linux/platform_device.h>
#include <sound/simple_card.h>
#include <linux/delay.h>
#include <linux/gpio.h>

void device_release_callback(struct device *dev) { /* do nothing */ };

#define CARD_PLATFORM_STR   "fe203000.i2s"
#define SND_SOC_DAIFMT_CBS_FLAG SND_SOC_DAIFMT_CBS_CFS
#define PWR_GPIO_PIN	16
#define RST_GPIO_PIN	27

static struct asoc_simple_card_info snd_rpi_simple_card_info = {
	.card = "snd_xmos_vocalfusion_card", // -> snd_soc_card.name
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
	struct clk *mclk;
	struct device *dev = &pdev->dev;
	int rate;
	int ret;
	
	mclk = devm_clk_get(&pdev->dev, NULL);
	
	if (of_property_read_u32(dev->of_node, "clock-frequency", &rate))
		return 0;
	clk_set_rate(mclk, rate);
	clk_prepare_enable(mclk);
	
	pr_alert("mclk at gpio4 set to: %lu Hz\n", clk_get_rate(mclk));
	
	ret = request_module(dmaengine);
	pr_alert("request module load '%s': %d\n",dmaengine, ret);
	ret = platform_device_register(&snd_rpi_simple_card_device);
	pr_alert("register platform device '%s': %d\n",snd_rpi_simple_card_device.name, ret);
	
	gpio_direction_output(PWR_GPIO_PIN, 1);
	gpio_direction_output(RST_GPIO_PIN, 1);
	mdelay(1);
	gpio_set_value(PWR_GPIO_PIN, 1);
	gpio_set_value(RST_GPIO_PIN, 1);
	mdelay(1);
	return 0;
}

static int vocalfusion_soundcard_remove(struct platform_device *pdev)
{
	struct clk *mclk;
	mclk = devm_clk_get(&pdev->dev, NULL);
	clk_disable_unprepare(mclk);
	
	platform_device_unregister(&snd_rpi_simple_card_device);
	return 0;
}

static const struct of_device_id vocalfusion_soundcard_of_match[] = {
	{ .compatible = "vocalfusion-soundcard", },
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