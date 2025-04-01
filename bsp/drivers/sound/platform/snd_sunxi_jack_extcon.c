/*
 * sound\soc\sunxi\snd_sunxi_jack_extcon.c
 * (C) Copyright 2022-2027
 * AllWinner Technology Co., Ltd. <www.allwinnertech.com>
 * Dby <dby@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/input.h>
#include <linux/extcon.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <sound/soc.h>
#include <sound/jack.h>

#include "snd_sunxi_log.h"
#include "snd_sunxi_jack.h"

#define HLOG	"JACK"

#define DETWORK_DTIME	10

static int jack_state;
static struct sunxi_jack sunxi_jack;

static void sunxi_jack_typec_mode_set(struct sunxi_jack_typec_cfg *jack_typec_cfg,
				      unsigned int mode);

/* for mic det */
static irqreturn_t jack_interrupt(int irq, void *dev_id)
{
	struct sunxi_jack_extcon *jack_extcon = sunxi_jack.jack_extcon;

	SND_LOG_DEBUG(HLOG, "\n");

	jack_extcon->jack_irq_clean(jack_extcon->data);

	schedule_work(&sunxi_jack.det_irq_work);

	return IRQ_HANDLED;
}

/* for hp det */
static int sunxi_jack_plugin_notifier(struct notifier_block *nb, unsigned long event, void *ptr)
{
	struct sunxi_jack_extcon *jack_extcon = container_of(nb, struct sunxi_jack_extcon, hp_nb);

	SND_LOG_DEBUG(HLOG, "event -> %lu\n", event);

	if (event)
		jack_extcon->jack_plug_sta = JACK_PLUG_STA_IN;
	else
		jack_extcon->jack_plug_sta = JACK_PLUG_STA_OUT;

	if (jack_extcon->jack_irq_clean) {
		jack_extcon->jack_irq_clean(jack_extcon->data);
	} else {
		SND_LOG_DEBUG(HLOG, "jack_irq_clean func is unused\n");
	}

	schedule_delayed_work(&sunxi_jack.det_sacn_work, msecs_to_jiffies(DETWORK_DTIME));

	return NOTIFY_DONE;
}

static void sunxi_jack_det_irq_work(struct work_struct *work)
{
	struct sunxi_jack_extcon *jack_extcon = sunxi_jack.jack_extcon;
	struct sunxi_jack_typec_cfg *jack_typec_cfg = &jack_extcon->jack_typec_cfg;

	SND_LOG_DEBUG(HLOG, "\n");

	if (jack_extcon->jack_plug_sta == JACK_PLUG_STA_IN) {
		sunxi_jack_typec_mode_set(jack_typec_cfg, jack_typec_cfg->mode_audio);
		SND_LOG_DEBUG(HLOG, "typec mode set to audio\n");
		goto jack_plug_in;
	} else {
		sunxi_jack_typec_mode_set(jack_typec_cfg, jack_typec_cfg->mode_usb);
		SND_LOG_DEBUG(HLOG, "typec mode set to usb\n");
		goto jack_plug_out;
	}

jack_plug_in:
	mutex_lock(&sunxi_jack.det_mutex);
	if (jack_extcon->jack_det_irq_work) {
		jack_extcon->jack_det_irq_work(jack_extcon->data, &sunxi_jack.type);
	} else {
		SND_LOG_ERR(HLOG, "jack_det_irq_work func is invaild\n");
	}

	mutex_unlock(&sunxi_jack.det_mutex);
	goto jack_report;

jack_plug_out:
	sunxi_jack.type = 0;

jack_report:
	if (sunxi_jack.system_up) {
		sunxi_jack.system_up = false;
	} else {
		if (sunxi_jack.type == sunxi_jack.type_old) {
			SND_LOG_DEBUG(HLOG, "jack report -> unchange\n");
			return;
		}
	}
	jack_state = sunxi_jack.type;

	snd_jack_report(sunxi_jack.jack.jack, sunxi_jack.type);
	if (sunxi_jack.type == 0) {
		printk("[sound] jack report -> OUT\n");
	} else if (sunxi_jack.type == SND_JACK_HEADSET) {
		printk("[sound] jack report -> HEADSET\n");
	} else if (sunxi_jack.type == SND_JACK_HEADPHONE) {
		printk("[sound] jack report -> HEADPHONE\n");
	} else if (sunxi_jack.type == (SND_JACK_HEADSET | SND_JACK_BTN_0)) {
		sunxi_jack.type &= ~SND_JACK_BTN_0;
		snd_jack_report(sunxi_jack.jack.jack, sunxi_jack.type);
		printk("[sound] jack report -> Hook\n");
	} else if (sunxi_jack.type == (SND_JACK_HEADSET | SND_JACK_BTN_1)) {
		sunxi_jack.type &= ~SND_JACK_BTN_1;
		snd_jack_report(sunxi_jack.jack.jack, sunxi_jack.type);
		printk("[sound] jack report -> Volume ++\n");
	} else if (sunxi_jack.type == (SND_JACK_HEADSET | SND_JACK_BTN_2)) {
		sunxi_jack.type &= ~SND_JACK_BTN_2;
		snd_jack_report(sunxi_jack.jack.jack, sunxi_jack.type);
		printk("[sound] jack report -> Volume --\n");
	} else if (sunxi_jack.type == (SND_JACK_HEADSET | SND_JACK_BTN_3)) {
		sunxi_jack.type &= ~SND_JACK_BTN_3;
		snd_jack_report(sunxi_jack.jack.jack, sunxi_jack.type);
		printk("[sound] jack report -> Voice Assistant\n");
	} else {
		printk("[sound] jack report -> others 0x%x\n", sunxi_jack.type);
	}

	sunxi_jack.type_old = sunxi_jack.type;
}

static void sunxi_jack_det_scan_work(struct work_struct *work)
{
	int ret;
	struct sunxi_jack_extcon *jack_extcon = sunxi_jack.jack_extcon;
	struct sunxi_jack_typec_cfg *jack_typec_cfg = &jack_extcon->jack_typec_cfg;

	SND_LOG_DEBUG(HLOG, "\n");

	ret = extcon_get_state(jack_extcon->extdev, EXTCON_JACK_HEADPHONE);
	SND_LOG_DEBUG(HLOG, "jack extcon state %d\n", ret);
	if (ret) {
		jack_extcon->jack_plug_sta = JACK_PLUG_STA_IN;
		sunxi_jack_typec_mode_set(jack_typec_cfg, jack_typec_cfg->mode_audio);
		SND_LOG_DEBUG(HLOG, "typec mode set to audio\n");
		goto jack_plug_in;
	} else {
		jack_extcon->jack_plug_sta = JACK_PLUG_STA_OUT;
		sunxi_jack_typec_mode_set(jack_typec_cfg, jack_typec_cfg->mode_usb);
		SND_LOG_DEBUG(HLOG, "typec mode set to usb\n");
		goto jack_plug_out;
	}

jack_plug_in:
	mutex_lock(&sunxi_jack.det_mutex);
	if (jack_extcon->jack_det_scan_work) {
		jack_extcon->jack_det_scan_work(jack_extcon->data, &sunxi_jack.type);
	} else {
		SND_LOG_ERR(HLOG, "jack_det_scan_work func is invaild\n");
	}

	mutex_unlock(&sunxi_jack.det_mutex);
	goto jack_report;

jack_plug_out:
	sunxi_jack.type = 0;

jack_report:
	if (sunxi_jack.system_up) {
		sunxi_jack.system_up = false;
	} else {
		if (sunxi_jack.type == sunxi_jack.type_old) {
			SND_LOG_DEBUG(HLOG, "jack report -> unchange\n");
			return;
		}
	}
	jack_state = sunxi_jack.type;

	snd_jack_report(sunxi_jack.jack.jack, sunxi_jack.type);
	if (sunxi_jack.type == 0) {
		printk("[sound] jack report -> OUT\n");
	} else if (sunxi_jack.type == SND_JACK_HEADSET) {
		printk("[sound] jack report -> HEADSET\n");
	} else if (sunxi_jack.type == SND_JACK_HEADPHONE) {
		printk("[sound] jack report -> HEADPHONE\n");
	} else {
		printk("[sound] jack report -> others 0x%x\n", sunxi_jack.type);
	}

	sunxi_jack.type_old = sunxi_jack.type;
}

static int sunxi_jack_suspend(struct snd_soc_card *card)
{
	struct sunxi_jack_extcon *jack_extcon = sunxi_jack.jack_extcon;

	SND_LOG_DEBUG(HLOG, "\n");

	disable_irq(sunxi_jack.jack_irq);

	if (jack_extcon->jack_suspend) {
		jack_extcon->jack_suspend(jack_extcon->data);
	} else {
		SND_LOG_ERR(HLOG, "jack_suspend func is invaild\n");
	}

	return 0;
}

static int sunxi_jack_resume(struct snd_soc_card *card)
{
	struct sunxi_jack_extcon *jack_extcon = sunxi_jack.jack_extcon;

	SND_LOG_DEBUG(HLOG, "\n");

	enable_irq(sunxi_jack.jack_irq);

	if (jack_extcon->jack_resume) {
		jack_extcon->jack_resume(jack_extcon->data);
	} else {
		SND_LOG_ERR(HLOG, "jack_resume func is invaild\n");
	}
	schedule_delayed_work(&sunxi_jack.det_sacn_work, msecs_to_jiffies(DETWORK_DTIME));

	return 0;
}

/* jack conversion typec interface probe */
static void sunxi_jack_typec_mode_set(struct sunxi_jack_typec_cfg *jack_typec_cfg,
				      unsigned int mode)
{
	int pin_en_level;
	int pin_sel_level;

	if (mode & 0xf0)
		pin_en_level = 1;
	else
		pin_en_level = 0;

	if (mode & 0x0f)
		pin_sel_level = 1;
	else
		pin_sel_level = 0;

	gpio_set_value(jack_typec_cfg->hp_pin_en, pin_en_level);
	gpio_set_value(jack_typec_cfg->hp_pin_sel, pin_sel_level);
}

void sunxi_jack_typec_mic_gnd_set(struct sunxi_jack_typec_cfg *jack_typec_cfg, bool sel)
{
	gpio_set_value(jack_typec_cfg->mic_pin_sel, sel);
}
EXPORT_SYMBOL(sunxi_jack_typec_mic_gnd_set);

static int sunxi_jack_typec_init(struct sunxi_jack_extcon *jack_extcon)
{
	int ret;
	unsigned int temp_val;
	struct platform_device *pdev = jack_extcon->pdev;
	struct device_node *np = pdev->dev.of_node;
	struct sunxi_jack_typec_cfg *jack_typec_cfg = &jack_extcon->jack_typec_cfg;

	SND_LOG_DEBUG(HLOG, "\n");

	/* get typec conversion chip mode setting */
	ret = of_property_read_u32(np, "jack-swmode-hp-off", &temp_val);
	if (ret < 0) {
		SND_LOG_WARN(HLOG, "jack-swmode-hp-off get failed, use default 0x00\n");
		jack_typec_cfg->mode_off = 0;
	} else {
		jack_typec_cfg->mode_off = temp_val;
	}
	ret = of_property_read_u32(np, "jack-swmode-hp-usb", &temp_val);
	if (ret < 0) {
		SND_LOG_WARN(HLOG, "jack-swmode-hp-usb get failed, use default 0x00\n");
		jack_typec_cfg->mode_usb = 0;
	} else {
		jack_typec_cfg->mode_usb = temp_val;
	}
	ret = of_property_read_u32(np, "jack-swmode-hp-audio", &temp_val);
	if (ret < 0) {
		SND_LOG_WARN(HLOG, "jack-swmode-hp-audio get failed, use default 0x00\n");
		jack_typec_cfg->mode_audio = 0;
	} else {
		jack_typec_cfg->mode_audio = temp_val;
	}

	/* get typec conversion chip mode setting gpio */
	ret = of_get_named_gpio(np, "jack-swpin-hp-en", 0);
	if (ret < 0) {
		SND_LOG_ERR(HLOG, "get jack-swpin-hp-en failed\n");
		return -1;
	}
	jack_typec_cfg->hp_pin_en = ret;
	if (!gpio_is_valid(jack_typec_cfg->hp_pin_en)) {
		SND_LOG_ERR(HLOG, "jack-swpin-hp-en is invalid\n");
		return -1;
	} else {
		ret = devm_gpio_request(&pdev->dev, jack_typec_cfg->hp_pin_en, "TYPEC_HP_EN");
		if (ret) {
			SND_LOG_ERR(HLOG, "failed to request TYPEC_HP_EN\n");
			return -1;
		}
	}

	ret = of_get_named_gpio(np, "jack-swpin-hp-sel", 0);
	if (ret < 0) {
		SND_LOG_ERR(HLOG, "get jack-swpin-hp-sel failed\n");
		return -1;
	}
	jack_typec_cfg->hp_pin_sel = ret;
	if (!gpio_is_valid(jack_typec_cfg->hp_pin_sel)) {
		SND_LOG_ERR(HLOG, "jack-swpin-hp-sel is invalid\n");
		return -1;
	} else {
		ret = devm_gpio_request(&pdev->dev, jack_typec_cfg->hp_pin_sel, "TYPEC_HP_SEL");
		if (ret) {
			SND_LOG_ERR(HLOG, "failed to request TYPEC_HP_SEL\n");
			return -1;
		}
	}

	ret = of_get_named_gpio(np, "jack-swpin-mic-sel", 0);
	if (ret < 0) {
		SND_LOG_ERR(HLOG, "get jack-swpin-mic-sel failed\n");
		return -1;
	}
	jack_typec_cfg->mic_pin_sel = ret;
	if (!gpio_is_valid(jack_typec_cfg->mic_pin_sel)) {
		SND_LOG_ERR(HLOG, "jack-swpin-mic-sel is invalid\n");
		return -1;
	} else {
		ret = devm_gpio_request(&pdev->dev, jack_typec_cfg->mic_pin_sel, "TYPEC_MIC_SEL");
		if (ret) {
			SND_LOG_ERR(HLOG, "failed to request TYPEC_MIC_SEL\n");
			return -1;
		}
	}

	/* default usb mode */
	gpio_direction_output(jack_typec_cfg->hp_pin_en, 1);
	gpio_direction_output(jack_typec_cfg->hp_pin_sel, 1);
	gpio_direction_output(jack_typec_cfg->mic_pin_sel, 1);
	sunxi_jack_typec_mode_set(jack_typec_cfg, jack_typec_cfg->mode_usb);
	SND_LOG_DEBUG(HLOG, "typec mode set to usb\n");

	return 0;
}

static void sunxi_jack_typec_exit(struct sunxi_jack_extcon *jack_extcon)
{
	struct platform_device *pdev = jack_extcon->pdev;
	struct sunxi_jack_typec_cfg *jack_typec_cfg = &jack_extcon->jack_typec_cfg;

	SND_LOG_DEBUG(HLOG, "\n");

	sunxi_jack_typec_mode_set(jack_typec_cfg, jack_typec_cfg->mode_usb);
	SND_LOG_DEBUG(HLOG, "typec mode set to usb\n");

	if (jack_typec_cfg->hp_pin_en)
		devm_gpio_free(&pdev->dev, jack_typec_cfg->hp_pin_en);

	if (jack_typec_cfg->hp_pin_sel)
		devm_gpio_free(&pdev->dev, jack_typec_cfg->hp_pin_sel);

	if (jack_typec_cfg->mic_pin_sel)
		devm_gpio_free(&pdev->dev, jack_typec_cfg->mic_pin_sel);
}

/*******************************************************************************
 * for codec or platform
 ******************************************************************************/
int snd_sunxi_jack_extcon_init(void *jack_data)
{
	int ret;
	struct device_node *np;
	struct platform_device *pdev;
	struct sunxi_jack_extcon *jack_extcon;

	SND_LOG_DEBUG(HLOG, "\n");

	if (!jack_data) {
		SND_LOG_ERR(HLOG, "jack_data is invaild\n");
		return -1;
	}
	jack_extcon = jack_data;
	sunxi_jack.jack_extcon = jack_extcon;

	if (jack_extcon->jack_init) {
		ret = jack_extcon->jack_init(jack_extcon->data);
		if (ret < 0) {
			SND_LOG_ERR(HLOG, "jack_init failed\n");
			return -1;
		}
	} else {
		SND_LOG_ERR(HLOG, "jack_init func is invaild\n");
	}

	mutex_init(&sunxi_jack.det_mutex);
	INIT_WORK(&sunxi_jack.det_irq_work, sunxi_jack_det_irq_work);
	INIT_DELAYED_WORK(&sunxi_jack.det_sacn_work, sunxi_jack_det_scan_work);

	/* for hp det only */
	pdev = jack_extcon->pdev;
	np = pdev->dev.of_node;
	if (of_property_read_bool(np, "extcon")) {
		jack_extcon->extdev = extcon_get_edev_by_phandle(&pdev->dev, 0);
		if (IS_ERR(jack_extcon->extdev)) {
			SND_LOG_ERR(HLOG, "get extcon dev failed\n");
			return -1;
		}
	} else {
		SND_LOG_ERR(HLOG, "get extcon failed\n");
		return -1;
	}
	jack_extcon->hp_nb.notifier_call = sunxi_jack_plugin_notifier;
	ret = extcon_register_notifier(jack_extcon->extdev, EXTCON_JACK_HEADPHONE, &jack_extcon->hp_nb);
	if (ret < 0) {
		SND_LOG_ERR(HLOG, "register jack notifier failed\n");
		return -1;
	}

	ret = sunxi_jack_typec_init(jack_extcon);
	if (ret < 0) {
		SND_LOG_ERR(HLOG, "typec jack init failed\n");
		return -1;
	}

	/* for mic det only */
	sunxi_jack.jack_irq = platform_get_irq(pdev, 0);
	if (sunxi_jack.jack_irq < 0) {
		SND_LOG_ERR(HLOG, "platform_get_irq failed\n");
		return -ENODEV;
	}

	ret = request_irq(sunxi_jack.jack_irq, jack_interrupt, IRQF_TRIGGER_NONE, "jack irq", NULL);
	if (ret < 0) {
		SND_LOG_ERR(HLOG, "request_irq failed\n");
		return -1;
	}

	/* system startup detection */
	sunxi_jack.system_up = true;
	schedule_delayed_work(&sunxi_jack.det_sacn_work, msecs_to_jiffies(DETWORK_DTIME));

	return 0;
}
EXPORT_SYMBOL(snd_sunxi_jack_extcon_init);

void snd_sunxi_jack_extcon_exit(void *jack_data)
{
	struct sunxi_jack_extcon *jack_extcon;

	SND_LOG_DEBUG(HLOG, "\n");

	if (!jack_data) {
		SND_LOG_ERR(HLOG, "jack_data is invaild\n");
		return;
	}
	jack_extcon = jack_data;

	free_irq(sunxi_jack.jack_irq, NULL);
	extcon_unregister_notifier(jack_extcon->extdev, EXTCON_JACK_HEADPHONE, &jack_extcon->hp_nb);

	if (jack_extcon->jack_exit) {
		jack_extcon->jack_exit(jack_extcon->data);
	} else {
		SND_LOG_ERR(HLOG, "jack_exit func is invaild\n");
	}

	cancel_work_sync(&sunxi_jack.det_irq_work);
	cancel_delayed_work_sync(&sunxi_jack.det_sacn_work);
	mutex_destroy(&sunxi_jack.det_mutex);

	sunxi_jack_typec_exit(jack_extcon);

	return;
}
EXPORT_SYMBOL(snd_sunxi_jack_extcon_exit);

/*******************************************************************************
 * for machcine
 ******************************************************************************/
int snd_sunxi_jack_register(struct snd_soc_card *card)
{
	int ret;

	SND_LOG_DEBUG(HLOG, "\n");

	if (!card) {
		SND_LOG_ERR(HLOG, "snd_soc_card is invaild\n");
		return -1;
	}
	sunxi_jack.card = card;

	ret = snd_soc_card_jack_new(sunxi_jack.card, "Headphones",
				    SND_JACK_HEADSET
				    | SND_JACK_HEADPHONE
				    | SND_JACK_BTN_0
				    | SND_JACK_BTN_1
				    | SND_JACK_BTN_2
				    | SND_JACK_BTN_3,
				    &sunxi_jack.jack, NULL, 0);
	if (ret) {
		SND_LOG_ERR(HLOG, "snd_soc_card_jack_new failed\n");
		return -1;
	}

	snd_jack_set_key(sunxi_jack.jack.jack, SND_JACK_BTN_0, KEY_MEDIA);
	snd_jack_set_key(sunxi_jack.jack.jack, SND_JACK_BTN_1, KEY_VOLUMEUP);
	snd_jack_set_key(sunxi_jack.jack.jack, SND_JACK_BTN_2, KEY_VOLUMEDOWN);
	snd_jack_set_key(sunxi_jack.jack.jack, SND_JACK_BTN_3, KEY_VOICECOMMAND);

	card->suspend_pre = sunxi_jack_suspend;
	card->resume_post = sunxi_jack_resume;

	return 0;
}
EXPORT_SYMBOL(snd_sunxi_jack_register);

void snd_sunxi_jack_unregister(struct snd_soc_card *card)
{
	SND_LOG_DEBUG(HLOG, "\n");

	/* sunxi_jack.card = NULL; */

	return;
}
EXPORT_SYMBOL(snd_sunxi_jack_unregister);

module_param_named(jack_state, jack_state, int, S_IRUGO | S_IWUSR);

MODULE_AUTHOR("Dby@allwinnertech.com");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("sunxi soundcard jack of extcon");
