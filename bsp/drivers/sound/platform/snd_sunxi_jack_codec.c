/*
 * sound\soc\sunxi\snd_sunxi_jack_codec.c
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
#include <sound/soc.h>
#include <sound/jack.h>

#include "snd_sunxi_log.h"
#include "snd_sunxi_jack.h"

#define HLOG		"JACK"

#define DETWORK_DTIME	10

static int jack_state;
static struct sunxi_jack sunxi_jack;

static irqreturn_t jack_interrupt(int irq, void *dev_id)
{
	struct sunxi_jack_codec *jack_codec = sunxi_jack.jack_codec;

	SND_LOG_DEBUG(HLOG, "\n");

	jack_codec->jack_irq_clean(jack_codec->data);

	schedule_work(&sunxi_jack.det_irq_work);

	return IRQ_HANDLED;
}

static void sunxi_jack_det_irq_work(struct work_struct *work)
{
	struct sunxi_jack_codec *jack_codec = sunxi_jack.jack_codec;

	SND_LOG_DEBUG(HLOG, "\n");

	mutex_lock(&sunxi_jack.det_mutex);
	jack_codec->jack_det_irq_work(jack_codec->data, &sunxi_jack.type);

	if (sunxi_jack.system_up) {
		sunxi_jack.system_up = false;
	} else {
		if (sunxi_jack.type == sunxi_jack.type_old) {
			SND_LOG_DEBUG(HLOG, "jack report -> unchange\n");
			mutex_unlock(&sunxi_jack.det_mutex);
			return;
		}
	}

	jack_state = sunxi_jack.type;
	mutex_unlock(&sunxi_jack.det_mutex);

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
	struct sunxi_jack_codec *jack_codec = sunxi_jack.jack_codec;

	SND_LOG_DEBUG(HLOG, "\n");

	mutex_lock(&sunxi_jack.det_mutex);
	jack_codec->jack_det_scan_work(jack_codec->data, &sunxi_jack.type);

	if (sunxi_jack.system_up) {
		sunxi_jack.system_up = false;
	} else {
		if (sunxi_jack.type == sunxi_jack.type_old) {
			SND_LOG_DEBUG(HLOG, "jack report -> unchange\n");
			mutex_unlock(&sunxi_jack.det_mutex);
			return;
		}
	}

	jack_state = sunxi_jack.type;
	mutex_unlock(&sunxi_jack.det_mutex);

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
	struct sunxi_jack_codec *jack_codec = sunxi_jack.jack_codec;

	SND_LOG_DEBUG(HLOG, "\n");

	disable_irq(sunxi_jack.jack_irq);

	jack_codec->jack_suspend(jack_codec->data);

	return 0;
}

static int sunxi_jack_resume(struct snd_soc_card *card)
{
	struct sunxi_jack_codec *jack_codec = sunxi_jack.jack_codec;

	SND_LOG_DEBUG(HLOG, "\n");

	enable_irq(sunxi_jack.jack_irq);

	jack_codec->jack_resume(jack_codec->data);
	schedule_delayed_work(&sunxi_jack.det_sacn_work, msecs_to_jiffies(DETWORK_DTIME));

	return 0;
}

/*******************************************************************************
 * for codec
 ******************************************************************************/
int snd_sunxi_jack_codec_init(void *jack_data)
{
	int ret;
	struct sunxi_jack_codec *jack_codec;

	SND_LOG_DEBUG(HLOG, "\n");

	if (!jack_data) {
		SND_LOG_ERR(HLOG, "jack_data is invaild\n");
		return -1;
	}
	jack_codec = jack_data;
	sunxi_jack.jack_codec = jack_codec;

	ret = jack_codec->jack_init(jack_codec->data);
	if (ret < 0) {
		SND_LOG_ERR(HLOG, "jack_init failed\n");
		return -1;
	}

	mutex_init(&sunxi_jack.det_mutex);
	INIT_WORK(&sunxi_jack.det_irq_work, sunxi_jack_det_irq_work);
	INIT_DELAYED_WORK(&sunxi_jack.det_sacn_work, sunxi_jack_det_scan_work);

	sunxi_jack.jack_irq = platform_get_irq(jack_codec->pdev, 0);
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
	/* recheck after 2s to prevent false triggering of interrupts */
	schedule_delayed_work(&sunxi_jack.det_sacn_work, msecs_to_jiffies(2000));

	return 0;
}
EXPORT_SYMBOL(snd_sunxi_jack_codec_init);

void snd_sunxi_jack_codec_exit(void *jack_data)
{
	struct sunxi_jack_codec *jack_codec;

	SND_LOG_DEBUG(HLOG, "\n");

	if (!jack_data) {
		SND_LOG_ERR(HLOG, "jack_data is invaild\n");
		return;
	}
	jack_codec = jack_data;

	free_irq(sunxi_jack.jack_irq, NULL);
	jack_codec->jack_exit(jack_codec->data);

	cancel_work_sync(&sunxi_jack.det_irq_work);
	cancel_delayed_work_sync(&sunxi_jack.det_sacn_work);
	mutex_destroy(&sunxi_jack.det_mutex);

	return;
}
EXPORT_SYMBOL(snd_sunxi_jack_codec_exit);

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
MODULE_DESCRIPTION("sunxi soundcard jack of internal-codec");
