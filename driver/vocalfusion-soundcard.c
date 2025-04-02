/*
 * =====================================================================================
 *
 * Description: XMOS VocalFusion soundcard driver - based on simple loader
 *
 * Conversion: Peter Steenbergen (p.steenbergen@j1nx.nl)
 * Orig Author: Huan Truong (htruong@tnhh.net), originally written by Paul Creaser
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

// Forward declarations for the driver's probe and remove functions
static int vocalfusion_soundcard_probe(struct platform_device *pdev);
static void vocalfusion_soundcard_remove(struct platform_device *pdev);

/*
 * Probe function - Initializes the sound card device
 * Configures clock and GPIOs as specified in the Device Tree
 */
static int vocalfusion_soundcard_probe(struct platform_device *pdev) {
    struct device *dev = &pdev->dev;
    struct clk *mclk;
    struct gpio_desc *pwr_gpio, *rst_gpio;
    int rate, ret;

    // Obtain and configure the master clock
    mclk = devm_clk_get(dev, NULL);
    if (IS_ERR(mclk)) {
        dev_err(dev, "Failed to get clock: %ld\n", PTR_ERR(mclk));
        return PTR_ERR(mclk);
    }

    // Read clock frequency from the DT
    ret = of_property_read_u32(dev->of_node, "clock-frequency", &rate);
    if (ret) {
        dev_err(dev, "Failed to read 'clock-frequency' from device tree: %d\n", ret);
        return ret;
    }

    dev_info(dev, "rate set to: %u Hz\n", rate);

    // Set the clock to the desired frequency
    ret = clk_set_rate(mclk, rate);
    if (ret) {
        dev_err(dev, "Failed to set clock rate: %d\n", ret);
        return ret;
    }

    clk_prepare_enable(mclk);
    dev_info(dev, "mclk set to: %lu Hz\n", clk_get_rate(mclk));

    // Configure power and reset GPIOs
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

    // Release the GPIOs so they can be controlled from userspace
    devm_gpiod_put(dev, pwr_gpio);
    devm_gpiod_put(dev, rst_gpio);

    pr_info("VocalFusion soundcard module loaded\n");
    return 0;
}

/*
 * Remove function - Cleans up the device on removal
 * Disables and unprepares the clock
 */
static void vocalfusion_soundcard_remove(struct platform_device *pdev) {
    struct clk *mclk = devm_clk_get(&pdev->dev, NULL); // Re-obtain the clock
    if (!IS_ERR(mclk)) {
        clk_disable_unprepare(mclk); // Disable the clock
    }

    pr_info("VocalFusion soundcard module unloaded\n");
}

// Device compatibility table
static const struct of_device_id vocalfusion_soundcard_of_match[] = {
    { .compatible = "vocalfusion-soundcard", },
    {},
};

MODULE_DEVICE_TABLE(of, vocalfusion_soundcard_of_match);

// Platform driver definition
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
