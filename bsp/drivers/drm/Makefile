# SPDX-License-Identifier: GPL-2.0-only
mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
mkfile_dir := $(dir $(mkfile_path))
disp_dir := $(mkfile_dir)../video/sunxi/disp2/disp

ifeq ($(CONFIG_AW_DRM_DE_V33X),y)
sub_dir  := lowlevel_v33x
else ifeq ($(CONFIG_AW_DRM_DE_V2X),y)
sub_dir  := lowlevel_v2x
else
sub_dir  :=
endif
sub_dir1 := de330
sub_dir2 := tcon

#$(info $(disp_dir))
#$(error "error: $(disp_dir)")
include $(disp_dir)/de/Makefile
include $(disp_dir)/de/$(sub_dir)/Makefile

#filter disp_al.o in de/lowlevel_xxx/
#it means that filter_obj includes all the files in de/lowlevel_xxx except disp_al.o
ifeq ($(CONFIG_AW_DRM_DE_V33X),y)
filter_obj1 = $(filter-out $(mydir)/disp_al_de.o,$(obj_low))
filter_obj = $(filter-out $(mydir)/disp_al_tcon.o,$(filter_obj1))
else
filter_obj = $(filter-out $(mydir)/disp_al.o,$(obj_low))
endif

#used in 5.10 kernel
filter_obj = $(obj_low)

#add prefix, result: ../../../drivers/video/sunxi/disp2/disp/de/lowlevel_xxx/xxx
#drm_low_obj = $(addprefix $(disp_dir)/,$(filter_obj))
drm_low_obj = $(addprefix ../video/sunxi/disp2/disp/,$(filter_obj))

ccflags-y := -I$(disp_dir)
ccflags-y += -I$(disp_dir)/de
ccflags-y += -I$(disp_dir)/de/$(sub_dir)
ccflags-y += -I$(disp_dir)/de/$(sub_dir)/$(sub_dir1)
ccflags-y += -I$(disp_dir)/de/$(sub_dir)/$(sub_dir2)
ccflags-y += -I$(srctree)/drivers/gpu/drm

ifeq ($(CONFIG_AW_DRM_DE_V33X),y)
ccflags-y += -DDE_VERSION_V33X
endif

obj-$(CONFIG_AW_DRM)        += sunxidrm.o

sunxidrm-y := sunxi_drm_drv.o
sunxidrm-y += sunxi_drm_plane.o
sunxidrm-y += sunxi_drm_crtc.o
sunxidrm-y += sunxi_drm_encoder.o
sunxidrm-y += sunxi_drm_connector.o
sunxidrm-$(CONFIG_AW_DRM_LCD) += sunxi_drm_lcd_connector.o
sunxidrm-$(CONFIG_AW_DRM_TV) += sunxi_drm_tv_connector.o
sunxidrm-$(CONFIG_AW_DRM_HDMI14) += sunxi_drm_hdmi_connector.o
sunxidrm-$(CONFIG_AW_DRM_HDMI20) += sunxi_drm_hdmi_connector.o

sunxidrm-y += sunxi_drm_fb.o
sunxidrm-y += sunxi_drm_gem.o
sunxidrm-$(CONFIG_DRM_FBDEV_EMULATION) += sunxi_drm_fbdev.o
sunxidrm-$(CONFIG_DRM_FBDEV_EMULATION) += sunxi_drm_bootlogo.o
sunxidrm-y += sunxi_drm_sysfs.o
sunxidrm-$(CONFIG_AW_DRM_IOMMU) += sunxi_drm_iommu.o

sunxidrm-y += $(drm_low_obj)
sunxidrm-y += sunxi_device/sunxi_common.o

#add DE
sunxidrm-$(CONFIG_AW_DRM_DE_SUN50IW1) += sunxi_device/sunxi_de.o
sunxidrm-$(CONFIG_AW_DRM_DE_V2X) += sunxi_device/sunxi_de.o
sunxidrm-$(CONFIG_AW_DRM_DE_V3X) += sunxi_device/sunxi_de.o
sunxidrm-$(CONFIG_AW_DRM_DE_V33X) += sunxi_device/sunxi_de_v33x.o

sunxidrm-$(CONFIG_AW_DRM_DE_V2X) += ../video/sunxi/disp2/disp/disp_sys_intf.o
sunxidrm-$(CONFIG_AW_DRM_DE_V2X) += ../video/sunxi/disp2/disp/de/disp_features.o

sunxidrm-$(CONFIG_AW_DRM_DE_SUN50IW1) += sunxi_device/sunxi_tcon.o
sunxidrm-$(CONFIG_AW_DRM_DE_V2X) += sunxi_device/sunxi_tcon.o
sunxidrm-$(CONFIG_AW_DRM_DE_V3X) += sunxi_device/sunxi_tcon.o
sunxidrm-$(CONFIG_AW_DRM_DE_V33X) += sunxi_device/sunxi_tcon_v33x.o

sunxidrm-$(CONFIG_AW_DRM_LCD) += sunxi_device/sunxi_lcd.o
sunxidrm-$(CONFIG_AW_DRM_BACKLIGHT) += sunxi_device/sunxi_backlight.o
sunxidrm-$(CONFIG_AW_DRM_TV) += sunxi_device/sunxi_tv.o
sunxidrm-$(CONFIG_AW_DRM_HDMI14) += sunxi_device/sunxi_hdmi14.o
sunxidrm-$(CONFIG_AW_DRM_HDMI20) += sunxi_device/sunxi_hdmi20.o

ifeq ($(CONFIG_AW_DRM_TV),y)
tv_dir := $(srctree)/../../bsp/drivers/video/sunxi/disp2/tv
ccflags-y += -I$(tv_dir)

sunxidrm-$(CONFIG_ARCH_SUN8IW11) += ../../../video/sunxi/disp2/tv/de_tve_sun8iw11.o
sunxidrm-$(CONFIG_ARCH_SUN50IW2) += ../../../video/sunxi/disp2/tv/de_tve_v1.o
sunxidrm-$(CONFIG_ARCH_SUN8IW12) += ../../../video/sunxi/disp2/tv/de_tve_sun8iw11.o
sunxidrm-$(CONFIG_ARCH_SUN8IW7) += ../../../video/sunxi/disp2/tv/de_tve_sun8iw7.o
sunxidrm-$(CONFIG_ARCH_SUN8IW17) += ../../../video/sunxi/disp2/tv/de_tve_sun8iw11.o
endif

ifeq ($(CONFIG_AW_DRM_HDMI14),y)
sunxidrm-y += sunxi_drm_hdmi_edid.o

hdmi14_dir := $(srctree)/../../bsp/drivers/video/sunxi/disp2/hdmi
ccflags-y += -I$(hdmi14_dir)

hdmi14_relative_dir := ../../../video/sunxi/disp2/hdmi

$(shell cp $(hdmi14_dir)/libhdmi_sun50iw1 $(hdmi14_dir)/libhdmi_sun50iw1.a)
$(shell cp $(hdmi14_dir)/libhdmi_sun8iw11 $( hdmi14_dir)/libhdmi_sun8iw11.a)

sunxidrm-$(CONFIG_ARCH_SUN50IW1) += $(hdmi14_relative_dir)/libhdmi_sun50iw1.a
sunxidrm-$(CONFIG_ARCH_SUN8IW11) += $(hdmi14_relative_dir)/libhdmi_sun8iw11.a
sunxidrm-$(CONFIG_ARCH_SUN8IW12) += $(hdmi14_relative_dir)/hdmi_bsp_sun8iw12.o
sunxidrm-$(CONFIG_ARCH_SUN8IW7) += $(hdmi14_relative_dir)/hdmi_bsp_sun8iw7.o
sunxidrm-$(CONFIG_ARCH_SUN8IW6) += $(hdmi14_relative_dir)/hdmi_bsp_sun8iw6.o
endif

ifeq ($(CONFIG_AW_DRM_HDMI20),y)
sunxidrm-y += sunxi_drm_hdmi_edid.o
hdmi20_dir := $(srctree)/../../bsp/drivers/video/sunxi/disp2/hdmi2
include $(hdmi20_dir)/Makefile

ccflags-y += -I$(hdmi20_dir)

hdmi20_lowlevel_obj = $(addprefix ../../../../$(hdmi20_dir)/,$(hdmi20_lowlevel))
sunxidrm-y += $(hdmi20_lowlevel_obj)
endif

ifeq ($(CONFIG_AW_DRM_LCD),y)
sunxidrm-$(CONFIG_AW_DRM_LCD_DSI) += sunxi_device/sunxi_dsi.o
sunxidrm-$(CONFIG_LCD_SUPPORT_DEFAULT) += \
		sunxi_device/lcd_panel/default_panel.o
sunxidrm-$(CONFIG_LCD_SUPPORT_BP101WX1) += \
		sunxi_device/lcd_panel/bp101wx1-206.o
ifeq ($(CONFIG_AW_DRM_LCD_DSI),y)
sunxidrm-$(CONFIG_LCD_SUPPORT_HE0801A068) += \
		sunxi_device/lcd_panel/he0801a068.o
sunxidrm-$(CONFIG_LCD_SUPPORT_KD101N51) += \
		sunxi_device/lcd_panel/kd101n51.o
sunxidrm-$(CONFIG_LCD_SUPPORT_JC0WS0081CM) += \
		sunxi_device/lcd_panel/jc0ws008lcm.o
endif
endif
