/*
 * =====================================================================================
 *
 * Description: SJ201 HAT - XMOS VocalFusion 3510INT & TAS5806MD soundcard driver
 * Author: Peter Steenbergen (p.steenbergen@j1nx.nl)
 *
 * =====================================================================================
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/clk.h>
#include <linux/gpio/consumer.h>
#include <linux/delay.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>

static int snd_rpi_sj201_soundcard_init(struct snd_soc_pcm_runtime *rtd)
{
    struct snd_soc_dai *codec_dai = asoc_rtd_to_codec(rtd, 0);
    struct snd_soc_dai *cpu_dai = asoc_rtd_to_cpu(rtd, 0);

    struct device *dev = &pdev->dev;
    struct clk *mclk;
    struct gpio_desc *pwr_gpio, *rst_gpio;
    int rate, ret;

    mclk = devm_clk_get(dev, NULL);
    if (IS_ERR(mclk)) {
        dev_err(dev, "Failed to get clock: %ld\n", PTR_ERR(mclk));
        return PTR_ERR(mclk);
    }

    ret = of_property_read_u32(dev->of_node, "clock-frequency", &rate);
    if (ret) {
        dev_err(dev, "Failed to read 'clock-frequency' from device tree: %d\n", ret);
        return ret;
    }

    dev_info(dev, "rate set to: %u Hz\n", rate);

    ret = clk_set_rate(mclk, rate);
    if (ret) {
        dev_err(dev, "Failed to set clock rate: %d\n", ret);
        return ret;
    }

    clk_prepare_enable(mclk);
    dev_info(dev, "mclk set to: %lu Hz\n", clk_get_rate(mclk));

    pwr_gpio = devm_gpiod_get(dev, "pwr", GPIOD_OUT_HIGH);
    if (IS_ERR(pwr_gpio)) {
        dev_err(dev, "Failed to get PWR GPIO: %ld\n", PTR_ERR(pwr_gpio));
        return PTR_ERR(pwr_gpio);
    }

    rst_gpio = devm_gpiod_get(dev, "rst", GPIOD_OUT_HIGH);
    if (IS_ERR(rst_gpio)) {
        dev_err(dev, "Failed to get RST GPIO: %ld\n", PTR_ERR(rst_gpio));
        return PTR_ERR(rst_gpio);
    }

    gpiod_set_value(pwr_gpio, 1);
    gpiod_set_value(rst_gpio, 1);

}

SND_SOC_DAILINK_DEFS(sj201,
    DAILINK_COMP_ARRAY(COMP_CPU("bcm2708-i2s.0")),
    DAILINK_COMP_ARRAY(COMP_CODEC("tas5806md", "tas5806md-amplifier")),
    DAILINK_COMP_ARRAY(COMP_PLATFORM("bcm2835-i2s.0")));

static struct snd_soc_dai_link snd_rpi_sj201_soundcard_dai[] = {
{
    .dai_fmt 			= SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
			  	SND_SOC_DAIFMT_CBM_CFS,
    .init			= snd_rpi_sj201_soundcard_init,
    .symmetric_rate		= 1,
    .symmetric_channels		= 1,
    .symmetric_sample_bits	= 1,
    SND_SOC_DAILINK_REG(sj201),
},
};

static struct snd_soc_card snd_rpi_sj201_soundcard = {
    .owner		= THIS_MODULE,
    .dai_link		= snd_rpi_sj201_soundcard_dai,
    .num_links		= ARRAY_SIZE(snd_rpi_sj201_soundcard_dai),
};

static int snd_rpi_sj201_soundcard_probe(struct platform_device *pdev) {
    int ret = 0;

    snd_rpi_sj201_soundcard.dev = &pdev->dev;

    if (pdev->dev.of_node) {
        struct device_node *i2s_node;
        struct snd_soc_card *card = &snd_rpi_sj201_soundcard;
        struct snd_soc_dai_link *dai = &snd_rpi_sj201_soundcard_dai[0];

        i2s_node = of_parse_phandle(pdev->dev.of_node,
                                    "i2s-controller", 0);
        if (i2s_node) {
            dai->cpus->dai_name = NULL;
            dai->cpus->of_node = i2s_node;
            dai->platforms->name = NULL;
            dai->platforms->of_node = i2s_node;
        }

        if (of_property_read_string(pdev->dev.of_node, "card_name",
                                    &card->name))
            card->name = "SJ201Soundcard";

        if (of_property_read_string(pdev->dev.of_node, "dai_name",
                                    &dai->name))
            dai->name = "SJ201 Soundcard";

        if (of_property_read_string(pdev->dev.of_node,
                                    "dai_stream_name", &dai->stream_name))
            dai->stream_name = "SJ201 Soundcard HiFi";

    }

    ret = snd_soc_register_card(&snd_rpi_iqaudio_codec);
    if (ret) {
        if (ret != -EPROBE_DEFER)
            dev_err(&pdev->dev,
                    "snd_soc_register_card() failed: %d\n", ret);
        return ret;
    }

    pr_info("SJ201 soundcard module loaded\n");
    return 0;
}

static int snd_rpi_sj201_soundcard_remove(struct platform_device *pdev) {
    struct clk *mclk = devm_clk_get(&pdev->dev, NULL);
    if (!IS_ERR(mclk)) {
        clk_disable_unprepare(mclk);
    }
    snd_soc_unregister_card(&snd_rpi_sj201_soundcard);
    pr_info("SJ201 soundcard module unloaded\n");
    return 0;
}

static const struct of_device_id sj201_of_match[] = {
    { .compatible = "sj201,sj201-card", },
    {},
};

MODULE_DEVICE_TABLE(of, sj201_of_match);

static struct platform_driver snd_rpi_sj201_soundcard_driver = {
    .driver = {
        .name = "sj201-driver",
        .owner = THIS_MODULE,
        .of_match_table = sj201_of_match,
    },
    .probe = snd_rpi_sj201_soundcard_probe,
    .remove = snd_rpi_sj201_soundcard_remove,
};

module_platform_driver(snd_rpi_sj201_soundcard_driver);

MODULE_DESCRIPTION("ASoc Driver for SJ201 HAT");
MODULE_AUTHOR("OpenVoiceOS");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:vocalfusion-soundcard");
