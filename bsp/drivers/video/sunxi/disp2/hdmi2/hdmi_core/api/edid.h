/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef EDID_H_
#define EDID_H_

#include "hdmitx_dev.h"
#include "general_ops.h"
#include "log.h"

#define EDID_LENGTH 128
#define DDC_ADDR 0x50

#define CEA_EXT	    0x02
#define VTB_EXT	    0x10
#define DI_EXT	    0x40
#define LS_EXT	    0x50
#define MI_EXT	    0x60

typedef enum {
	EDID_ERROR = 0,
	EDID_IDLE,
	EDID_READING,
	EDID_DONE
} edid_status_t;

typedef struct supported_dtd {
	u32 refresh_rate;/* 1HZ * 1000 */
	dtd_t dtd;
} supported_dtd_t;


typedef struct speaker_alloc_code {
	unsigned char byte;
	unsigned char code;
} speaker_alloc_code_t;

#define EDID_DETAIL_EST_TIMINGS 0xf7
#define EDID_DETAIL_CVT_3BYTE 0xf8
#define EDID_DETAIL_COLOR_MGMT_DATA 0xf9
#define EDID_DETAIL_STD_MODES 0xfa
#define EDID_DETAIL_MONITOR_CPDATA 0xfb
#define EDID_DETAIL_MONITOR_NAME 0xfc
#define EDID_DETAIL_MONITOR_RANGE 0xfd
#define EDID_DETAIL_MONITOR_STRING 0xfe
#define EDID_DETAIL_MONITOR_SERIAL 0xff


/* Short Audio Descriptor */
struct cea_sad {
	u8 format;
	u8 channels; /* max number of channels - 1 */
	u8 freq;
	u8 byte2; /* meaning depends on format */
};

extern u16 byte_to_word(const u8 hi, const u8 lo);
extern u8 bit_field(const u16 data, u8 shift, u8 width);
extern u16 concat_bits(u8 bHi, u8 oHi, u8 nHi, u8 bLo, u8 oLo, u8 nLo);


/**
 * Parses the Detailed Timing Descriptor.
 * Encapsulating the parsing process
 * @param dtd pointer to dtd_t strucutute for the information to be save in
 * @param data a pointer to the 18-byte structure to be parsed.
 * @return TRUE if success
 */
int dtd_parse(hdmi_tx_dev_t *dev, dtd_t *dtd, u8 data[18]);

/**
 * @param dtd pointer to dtd_t strucutute for the information to be save in
 * @param code the CEA 861-D video code.
 * @param refreshRate the specified vertical refresh rate.
 * @return TRUE if success
 */
int dtd_fill(hdmi_tx_dev_t *dev, dtd_t *dtd, u32 code, u32 refreshRate);

/**
 * @param dtd Pointer to the current DTD parameters
 * @return The refresh rate if DTD parameters are correct and 0 if not
 */
u32 dtd_get_refresh_rate(dtd_t *dtd);

void monitor_range_limits_reset(hdmi_tx_dev_t *dev, monitorRangeLimits_t *mrl);

void colorimetry_data_block_reset(hdmi_tx_dev_t *dev,
					colorimetryDataBlock_t *cdb);
void hdr_metadata_data_block_reset(hdmi_tx_dev_t *dev,
		struct hdr_static_metadata_data_block *hdr_metadata);

int colorimetry_data_block_parse(hdmi_tx_dev_t *dev,
					colorimetryDataBlock_t *cdb, u8 *data);

int hdr_static_metadata_block_parse(hdmi_tx_dev_t *dev,
		struct hdr_static_metadata_data_block *hdr_metadata, u8 *data);

void hdmiforumvsdb_reset(hdmi_tx_dev_t *dev, hdmiforumvsdb_t *forumvsdb);
int hdmiforumvsdb_parse(hdmi_tx_dev_t *dev,
					hdmiforumvsdb_t *forumvsdb, u8 *data);

void hdmivsdb_reset(hdmi_tx_dev_t *dev, hdmivsdb_t *vsdb);
int edid_parser_CeaExtReset(hdmi_tx_dev_t *dev, sink_edid_t *edidExt);


/**
 * Parse an array of data to fill the hdmivsdb_t data strucutre
 * @param *vsdb pointer to the structure to be filled
 * @param *data pointer to the 8-bit data type array to be parsed
 * @return Success, or error code:
 * @return 1 - array pointer invalid
 * @return 2 - Invalid datablock tag
 * @return 3 - Invalid minimum length
 * @return 4 - HDMI IEEE registration identifier not valid
 * @return 5 - Invalid length - latencies are not valid
 * @return 6 - Invalid length - Interlaced latencies are not valid
 */
int hdmivsdb_parse(hdmi_tx_dev_t *dev, hdmivsdb_t *vsdb, u8 *data);


void sad_reset(hdmi_tx_dev_t *dev, shortAudioDesc_t *sad);

/**
 * Parse Short Audio Descriptor
 */
int sad_parse(hdmi_tx_dev_t *dev, shortAudioDesc_t *sad, u8 *data);

int svd_parse(hdmi_tx_dev_t *dev, shortVideoDesc_t *svd, u8 data);


/* speaker_alloc_data_block_reset */
void speaker_alloc_data_block_reset(hdmi_tx_dev_t *dev,
					speakerAllocationDataBlock_t *sadb);
int speaker_alloc_data_block_parse(hdmi_tx_dev_t *dev,
				speakerAllocationDataBlock_t *sadb, u8 *data);

void video_cap_data_block_reset(hdmi_tx_dev_t *dev,
				videoCapabilityDataBlock_t *vcdb);
int video_cap_data_block_parse(hdmi_tx_dev_t *dev,
			videoCapabilityDataBlock_t *vcdb, u8 *data);

int edid_extension_read(hdmi_tx_dev_t *dev, int block, u8 *edid_ext);
int edid_read(hdmi_tx_dev_t *dev, struct edid *edid);
int edid_parser(hdmi_tx_dev_t *dev, u8 *buffer, sink_edid_t *edidExt,
							u16 edid_size);

#endif	/* EDID_H_ */
