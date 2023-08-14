/* sound\soc\sunxi\snd_sunxi_jack.h
 * (C) Copyright 2022-2027
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Dby <dby@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __SND_SUNXI_JACK_H
#define __SND_SUNXI_JACK_H

struct sunxi_jack_codec {
	struct platform_device *pdev;

	void *data;
	int (*jack_init)(void *);
	void (*jack_exit)(void *);
	int (*jack_suspend)(void *);
	int (*jack_resume)(void *);

	void (*jack_irq_clean)(void *);
	void (*jack_det_irq_work)(void *, enum snd_jack_types *);
	void (*jack_det_scan_work)(void *, enum snd_jack_types *);
};

enum JACK_PLUG_STA {
	JACK_PLUG_STA_OUT = 0x0,
	JACK_PLUG_STA_IN,
};

struct sunxi_jack_typec_cfg {
	/* for hp conversion typec chip */
	unsigned int hp_pin_en;
	unsigned int hp_pin_sel;

	unsigned int mode_usb;
	unsigned int mode_audio;
	unsigned int mode_off;

	/* for mic conversion typec chip */
	unsigned int mic_pin_sel;
};

struct sunxi_jack_extcon {
	struct platform_device *pdev;

	enum JACK_PLUG_STA jack_plug_sta;
	struct extcon_dev *extdev;
	struct notifier_block hp_nb;
	struct sunxi_jack_typec_cfg jack_typec_cfg;

	void *data;
	int (*jack_init)(void *);
	void (*jack_exit)(void *);
	int (*jack_suspend)(void *);
	int (*jack_resume)(void *);

	void (*jack_irq_clean)(void *);
	void (*jack_det_irq_work)(void *, enum snd_jack_types *);
	void (*jack_det_scan_work)(void *, enum snd_jack_types *);
};

/* TODO:
 * struct sunxi_jack_gpio {
 * 	struct platform_device *pdev;
 * }
 */

struct sunxi_jack {
	struct snd_soc_jack jack;
	/* struct snd_soc_jack_pin pin; */
	/* struct snd_soc_jack_gpio gpio; */

	unsigned int jack_irq;
	struct mutex det_mutex;
	struct work_struct det_irq_work;
	struct delayed_work det_sacn_work;

	/* snd_jack_report value */
	bool system_up;
	enum snd_jack_types type;
	enum snd_jack_types type_old;

	/* get from machine */
	struct snd_soc_card *card;

	/* init by codec or platform */
	struct sunxi_jack_codec *jack_codec;	/* mode -> codec */
	struct sunxi_jack_extcon *jack_extcon;	/* mode -> extcon */
};

#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_JACK_CODEC)
extern int snd_sunxi_jack_codec_init(void *jack_data);
extern void snd_sunxi_jack_codec_exit(void *jack_data);

extern int snd_sunxi_jack_register(struct snd_soc_card *card);
extern void snd_sunxi_jack_unregister(struct snd_soc_card *card);
#elif IS_ENABLED(CONFIG_SND_SOC_SUNXI_JACK_EXTCON)
extern void sunxi_jack_typec_mic_gnd_set(struct sunxi_jack_typec_cfg *jack_typec_cfg, bool sel);

extern int snd_sunxi_jack_extcon_init(void *jack_data);
extern void snd_sunxi_jack_extcon_exit(void *jack_data);

extern int snd_sunxi_jack_register(struct snd_soc_card *card);
extern void snd_sunxi_jack_unregister(struct snd_soc_card *card);
#else
static inline int snd_sunxi_jack_codec_init(void *jack_data)
{
	(void)jack_data;
	pr_err("[sound %4d][PCM %s] JACK API is disabled\n", __LINE__, __func__);
	return 0;
}

static inline void snd_sunxi_jack_codec_exit(void *jack_data)
{
	(void)jack_data;
	pr_err("[sound %4d][PCM %s] JACK API is disabled\n", __LINE__, __func__);
	return;
}

static inline int snd_sunxi_jack_extcon_init(void *jack_data)
{
	(void)jack_data;
	pr_err("[sound %4d][PCM %s] JACK API is disabled\n", __LINE__, __func__);
	return 0;
}

static inline void snd_sunxi_jack_extcon_exit(void *jack_data)
{
	(void)jack_data;
	pr_err("[sound %4d][PCM %s] JACK API is disabled\n", __LINE__, __func__);
	return;
}

static inline int snd_sunxi_jack_register(struct snd_soc_card *card)
{
	(void)card;
	pr_err("[sound %4d][PCM %s] JACK API is disabled\n", __LINE__, __func__);
	return 0;
}

static inline void snd_sunxi_jack_unregister(struct snd_soc_card *card)
{
	(void)card;
	pr_err("[sound %4d][PCM %s] JACK API is disabled\n", __LINE__, __func__);
	return;
}
#endif

#endif /* __SND_SUNXI_JACK_H */
