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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/clk.h>
#include <linux/gpio/consumer.h>
#include <linux/delay.h>
#include <sound/simple_card.h>

static struct asoc_simple_card_info card_info;
static struct platform_device card_device;

// Forward declarations for callback functions
static int vocalfusion_soundcard_probe(struct platform_device *pdev);
static int vocalfusion_soundcard_remove(struct platform_device *pdev);

// Device release callback - required but does nothing in this case
static void device_release_callback(struct device *dev) {}

// Helper function to determine the I2S platform name from the device tree
static int get_i2s_platform_from_device_tree(char **card_platform, char **dmaengine) {
    struct device_node *node;
    const char *compatible;
    int ret = 0;

    node = of_find_node_by_path("/");
    if (!node) {
        pr_err("Failed to find device tree root node\n");
        return -ENODEV; // No such device
    }

    compatible = of_get_property(node, "compatible", NULL);
    if (!compatible) {
        pr_err("Failed to get compatible property from device tree\n");
        ret = -ENODATA; // No data available
        goto out_put_node;
    }

    // Determine the platform and DMA engine based on the compatible property
    if (strstr(compatible, "raspberrypi,5") != NULL) {
        *card_platform = "1f000a4000.i2s"; // Pi 5
        *dmaengine = "snd_pcm_dmaengine";
    } else if (strstr(compatible, "raspberrypi,4") != NULL) {
        *card_platform = "fe203000.i2s"; // Pi 4
        *dmaengine = "bcm2708-dmaengine";
    } else if (strstr(compatible, "raspberrypi,3") != NULL) {
        *card_platform = "3f203000.i2s"; // Pi 2 and 3
        *dmaengine = "bcm2708-dmaengine";
    } else if (strstr(compatible, "raspberrypi,zero") != NULL) {
        *card_platform = "20203000.i2s"; // Pi Zero
        *dmaengine = "bcm2708-dmaengine";
    } else {
        pr_info("Unknown or unsupported Raspberry Pi model, defaulting to Pi 4 platform\n");
        *card_platform = "fe203000.i2s";
        *dmaengine = "bcm2708-dmaengine";
    }

    out_put_node:
        of_node_put(node);
    return ret;
}

// Default card information
static struct asoc_simple_card_info default_card_info = {
    .card = "snd_xmos_vocalfusion_card", // -> snd_soc_card.name
    .name = "simple-card_codec_link", // -> snd_soc_dai_link.name
    .codec = "snd-soc-dummy", // "dmic-codec", // -> snd_soc_dai_link.codec_name
    .platform = "not-set.i2s",
    .daifmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS,
    .cpu_dai = {
        .name = "not-set.i2s", // -> snd_soc_dai_link.cpu_dai_name
        .sysclk = 0
    },
    .codec_dai = {
        .name = "snd-soc-dummy-dai", //"dmic-codec", // -> snd_soc_dai_link.codec_dai_name
        .sysclk = 0
    },
};

// Setup the platform device
static struct platform_device default_card_device = {
    .name = "asoc-simple-card", //module alias
    .id = 0,
    .num_resources = 0,
    .dev = {
        .release = &device_release_callback,
        .platform_data = &default_card_info, // Hack to pass card info
    },
};

/*
 * Setup the card info
 */
static int vocalfusion_soundcard_probe(struct platform_device *pdev) {
    struct device *dev = &pdev->dev;
    struct clk *mclk;
    struct gpio_desc *pwr_gpio, *rst_gpio;
    char *dmaengine = NULL, *card_platform = NULL;
    int rate, ret;

    // Get the I2S platform and DMA engine from the device tree
    ret = get_i2s_platform_from_device_tree(&card_platform, &dmaengine);
    if (ret) {
        dev_err(dev, "Failed to get I2S platform or DMA engine from device tree: %d\n", ret);
        return ret;
    }

    dev_info(dev, "Module initialized with platform: %s, DMA engine: %s\n", card_platform, dmaengine);

    // Set up card information
    card_info = default_card_info;
    card_info.platform = card_platform;
    card_info.cpu_dai.name = card_platform;

    // Register the platform device
    card_device = default_card_device;
    card_device.dev.platform_data = &card_info;

    ret = platform_device_register(&card_device);
    if (ret) {
        dev_err(dev, "Failed to register platform device '%s': %d\n", card_device.name, ret);
        return ret;
    }

    // Clock configuration
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

    ret = clk_set_rate(mclk, rate);
    if (ret) {
        dev_err(dev, "Failed to set clock rate: %d\n", ret);
        return ret;
    }

    clk_prepare_enable(mclk);
    dev_info(dev, "mclk set to: %lu Hz\n", clk_get_rate(mclk));

    // Module load
    ret = request_module(dmaengine);
    if (ret) {
        dev_err(dev, "Failed to load module '%s': %d\n", dmaengine, ret);
        return ret;
    }

    // GPIO configuration
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

    return 0;
}

static int vocalfusion_soundcard_remove(struct platform_device *pdev) {
    struct clk *mclk = devm_clk_get(&pdev->dev, NULL);
    clk_disable_unprepare(mclk);

    platform_device_unregister(&card_device);
    pr_alert("vocalfusion soundcard module unloaded\n");
    return 0;
}

static
const struct of_device_id vocalfusion_soundcard_of_match[] = {
    { .compatible = "vocalfusion-soundcard", },
    {},
};

MODULE_DEVICE_TABLE(of, vocalfusion_soundcard_of_match);

static struct platform_driver vocalfusion_soundcard_driver = {
    .driver = {
        .name = "vocalfusion-driver",
        .owner = THIS_MODULE,
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
