// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2019 NXP
 *
 */

#include <common.h>
#include <dm.h>
#include <mipi_dsi.h>
#include <panel.h>
#include <asm/gpio.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <dm/device_compat.h>

#define CMD_TABLE_LEN 2
typedef u8 cmd_set_table[CMD_TABLE_LEN];

/* Write Manufacture Command Set Control */
#define WRMAUCCTR 0xFE

struct ST7701S_panel_priv {
       struct gpio_desc reset;
       struct gpio_desc backlight;
       unsigned int lanes;
       enum mipi_dsi_pixel_format format;
       unsigned long mode_flags;
};

u8 dcs0[] = {0xFF ,0x77 ,0x01 ,0x00 ,0x00 ,0x13};
u8 dcs1[] = {0xEF, 0x08};
u8 dcs2[] = {0xFF, 0x77, 0x01, 0x00, 0x00, 0x10};
u8 dcs3[] = {0xC0, 0xE9, 0x03};
u8 dcs4[] = {0xC1, 0x10, 0x0C};
u8 dcs5[] = {0xC2, 0x01, 0x0A};
u8 dcs6[] = {0xC3, 0x02};
u8 dcs7[] = {0xCC, 0x10};
u8 dcs8[] = {0xCD, 0x08};
u8 dcs9[] =  {0xB0, 0x0D, 0x14, 0x9C, 0x0B, 0x10, 0x06, 0x08, 0x09, 0x08, 0x22, 0x02, 0x4F, 0x0E, 0x66, 0x2D, 0x1F };
u8 dcs10[] = {0xB1, 0x00, 0x17, 0x9E, 0x0F, 0x11, 0x06, 0x0C, 0x08, 0x08, 0x26, 0x04, 0x51, 0x10, 0x6A, 0x33, 0x1B };
u8 dcs11[] = {0xFF, 0x77, 0x01, 0x00, 0x00, 0x11 };
u8 dcs12[] = {0xB0, 0x30 };
u8 dcs13[] = {0xB1, 0x67}; //vcom 47
u8 dcs14[] = {0xB2, 0x84};
u8 dcs15[] = {0xB3, 0x80 };
u8 dcs16[] = {0xB5, 0x4E };
u8 dcs17[] = {0xB7, 0x85 };
u8 dcs18[] = {0xB8, 0x20 };
u8 dcs19[] = {0xC1, 0x78 };
u8 dcs20[] = {0xC2, 0x78 };
u8 dcs21[] = {0xD0, 0x88 };
u8 dcs22[] = {0xE0, 0x00, 0x00, 0x02 };
u8 dcs23[] = {0xE1, 0x06, 0xA0, 0x08, 0xA0, 0x05, 0xA0, 0x07, 0xA0, 0x00, 0x44, 0x44};
u8 dcs24[] = {0xE2, 0x30, 0x30, 0x44, 0x44, 0x6E, 0xA0, 0x00, 0x00, 0x6E, 0xA0, 0x00, 0x00 };
u8 dcs25[] = {0xE3, 0x00, 0x00, 0x33, 0x33 };
u8 dcs26[] = {0xE4, 0x44, 0x44 };
u8 dcs27[] = {0xE5, 0x0D, 0x69, 0x0A, 0xA0, 0x0F, 0x6B, 0x0A, 0xA0, 0x09, 0x65, 0x0A, 0xA0, 0x0B, 0x67, 0x0A, 0xA0 };
u8 dcs28[] = {0xE6, 0x00, 0x00, 0x33, 0x33 };
u8 dcs29[] = {0xE7, 0x44, 0x44 };
u8 dcs30[] = {0xE8, 0x0C, 0x68, 0x0A, 0xA0, 0x0E, 0x6A, 0x0A, 0xA0, 0x08, 0x64, 0x0A, 0xA0, 0x0A, 0x66, 0x0A, 0xA0 };
u8 dcs31[] = {0xE9, 0x36 , 0x00 };
u8 dcs32[] = {0xEB, 0x00, 0x01, 0xE4, 0xE4, 0x44, 0x88, 0x40 };
u8 dcs33[] = {0xED, 0xFF, 0x45, 0x67, 0xFA, 0x01, 0x2B, 0xCF, 0xFF, 0xFF, 0xFC, 0xB2, 0x10, 0xAF, 0x76, 0x54, 0xFF };
u8 dcs34[] = {0xEF, 0x10, 0x0D, 0x04, 0x08, 0x3F, 0x1F };
u8 dcs35[] = {0x3A, 0x66 };
u8 dcs36[] = {0x11, 0x00};
u8 dcs37[] = {0x29, 0x00};
u8 dcs38[] = {0x35, 0x00};

unsigned char *ST7701S_CMD_Group[]={
        dcs0,
        dcs1,
        dcs2,
        dcs3,
        dcs4,
        dcs5,
        dcs6,
        dcs7,
        dcs8,
        dcs9,
        dcs10,
        dcs11,
        dcs12,
        dcs13,
        dcs14,
        dcs15,
        dcs16,
        dcs17,
        dcs18,
        dcs19,
        dcs20,
        dcs21,
        dcs22,
        dcs23,
        dcs24,
        dcs25,
        dcs26,
        dcs27,
        dcs28,
        dcs29,
        dcs30,
        dcs31,
        dcs32,
        dcs33,
        dcs34,
        dcs35,
        dcs36,
        dcs37,
        dcs38,
};


unsigned long int ST7701S_CMD_Group_size[]={
        sizeof(dcs0),
        sizeof(dcs1),
        sizeof(dcs2),
        sizeof(dcs3),
        sizeof(dcs4),
        sizeof(dcs5),
        sizeof(dcs6),
        sizeof(dcs7),
        sizeof(dcs8),
        sizeof(dcs9),
        sizeof(dcs10),
        sizeof(dcs11),
        sizeof(dcs12),
        sizeof(dcs13),
        sizeof(dcs14),
        sizeof(dcs15),
        sizeof(dcs16),
        sizeof(dcs17),
        sizeof(dcs18),
        sizeof(dcs19),
        sizeof(dcs20),
        sizeof(dcs21),
        sizeof(dcs22),
        sizeof(dcs23),
        sizeof(dcs24),
        sizeof(dcs25),
        sizeof(dcs26),
        sizeof(dcs27),
        sizeof(dcs28),
        sizeof(dcs29),
        sizeof(dcs30),
        sizeof(dcs31),
        sizeof(dcs32),
        sizeof(dcs33),
        sizeof(dcs34),
        sizeof(dcs35),
        sizeof(dcs36),
        sizeof(dcs37),
        sizeof(dcs38),
};

static const struct display_timing default_timing = {
        .pixelclock = {8294400, 16588800, 16588800}, //Hz
        .hactive = { 480, 480, 480 },
        .hfront_porch = { 1, 1, 50 },
        .hsync_len = { 1, 1, 10 },
        .hback_porch = { 50, 50, 46 },
        .vactive = { 480, 480, 480 },
        .vfront_porch = { 10, 15, 15 },
        .vsync_len = { 1, 1, 10 },
        .vback_porch = { 5, 15, 15 },
        .flags = DISPLAY_FLAGS_HSYNC_LOW |
                 DISPLAY_FLAGS_VSYNC_LOW |
                 DISPLAY_FLAGS_DE_LOW|
                 DISPLAY_FLAGS_PIXDATA_NEGEDGE,
};


static u8 color_format_from_dsi_format(enum mipi_dsi_pixel_format format)
{
       switch (format) {
       case MIPI_DSI_FMT_RGB565:
               return 0x55;
       case MIPI_DSI_FMT_RGB666:
       case MIPI_DSI_FMT_RGB666_PACKED:
               return 0x66;
       case MIPI_DSI_FMT_RGB888:
               return 0x77;
       default:
               return 0x77; /* for backward compatibility */
       }
}

static int rad_panel_push_cmd_list(struct mipi_dsi_device *dsi)
{
        size_t i;
        int ret = 0;
        struct device *dev = &dsi->dev;

        dsi->mode_flags |= MIPI_DSI_MODE_LPM;

//      #if 0
        //send init cmds
        for(i=0; i<39; i++)
        {
                ret = mipi_dsi_generic_write(dsi, ST7701S_CMD_Group[i],
                                                ST7701S_CMD_Group_size[i]);
                if (ret < 0) {
                        return ret;
                }
               mdelay(1);
        }

        return ret;
}

static int ST7701S_enable(struct udevice *dev)
{
       struct ST7701S_panel_priv *priv = dev_get_priv(dev);
       struct mipi_dsi_panel_plat *plat = dev_get_plat(dev);
       struct mipi_dsi_device *dsi = plat->device;
       u8 color_format = color_format_from_dsi_format(priv->format);
       u16 brightness;
       int ret;

       dsi->mode_flags |= MIPI_DSI_MODE_LPM;

       ret = mipi_dsi_dcs_exit_sleep_mode(dsi);
       if (ret < 0) {
               printf("%s, %s : Failed to exit sleep mode (%d)\n", __FILE__, __func__, ret);
               return ret;
       }

       mdelay(120);
       ret = rad_panel_push_cmd_list(dsi);
       if (ret < 0) {
               printf("%s, %s : Failed to send MCS (%d)\n", __FILE__, __func__, ret);
               return -EIO;
       }
       mdelay(5);
       /* Set pixel format */
       ret = mipi_dsi_dcs_set_pixel_format(dsi, color_format);
       if (ret < 0) {
               printf("%s, %s : Failed to set pixel format (%d)\n", __FILE__, __func__, ret);
               return -EIO;
       }

       ret = mipi_dsi_dcs_set_display_on(dsi);
       if (ret < 0) {
               printf("%s, %s : Failed to set display ON (%d)\n", __FILE__, __func__, ret);
               return -EIO;
       }
       dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

       mdelay(5);
       return 0;
}

static int ST7701S_panel_enable_backlight(struct udevice *dev)
{
       struct mipi_dsi_panel_plat *plat = dev_get_plat(dev);
       struct mipi_dsi_device *device = plat->device;
       int ret;

       ret = mipi_dsi_attach(device);
       if (ret < 0)
               return ret;

       return ST7701S_enable(dev);
}

static int ST7701S_panel_get_display_timing(struct udevice *dev,
                                           struct display_timing *timings)
{
       struct mipi_dsi_panel_plat *plat = dev_get_plat(dev);
       struct mipi_dsi_device *device = plat->device;
       struct ST7701S_panel_priv *priv = dev_get_priv(dev);

       memcpy(timings, &default_timing, sizeof(*timings));

       /* fill characteristics of DSI data link */
       if (device) {
               device->lanes = priv->lanes;
               device->format = priv->format;
               device->mode_flags = priv->mode_flags;
       }

       return 0;
}

static int ST7701S_panel_probe(struct udevice *dev)
{
       struct ST7701S_panel_priv *priv = dev_get_priv(dev);
       int ret;
       u32 video_mode;
       priv->format = MIPI_DSI_FMT_RGB888;
       priv->mode_flags = MIPI_DSI_MODE_VIDEO_HSE | MIPI_DSI_MODE_VIDEO |
                               MIPI_DSI_CLOCK_NON_CONTINUOUS;

       printf("%s, %s : probe start \n", __FILE__, __func__);
       ret = dev_read_u32(dev, "video-mode", &video_mode);
       if (!ret) {
               switch (video_mode) {
               case 0:
                       /* burst mode */
                       priv->mode_flags |= MIPI_DSI_MODE_VIDEO_BURST;
                       break;
               case 1:
                       /* non-burst mode with sync event */
                       break;
               case 2:
                       /* non-burst mode with sync pulse */
                       priv->mode_flags |= MIPI_DSI_MODE_VIDEO_SYNC_PULSE;
                       break;
               default:
                       dev_warn(dev, "invalid video mode %d\n", video_mode);
                       break;
               }
       }

       ret = dev_read_u32(dev, "dsi-lanes", &priv->lanes);
       if (ret) {
               return ret;
       }

       ret = gpio_request_by_name(dev, "reset-gpio", 0, &priv->reset,
                                  GPIOD_IS_OUT);
       if (ret) {
               printf("%s, %s : Warning: cannot get reset GPIO\n", __FILE__, __func__);
               if (ret != -ENOENT)
                       return ret;
       }

       /* reset panel */
       ret = dm_gpio_set_value(&priv->reset, true);
       if (ret)
               printf("%s, %s : reset gpio fails to set true\n", __FILE__, __func__);
       mdelay(20);
       ret = dm_gpio_set_value(&priv->reset, false);
       if (ret)
               printf("%s, %s : reset gpio fails to set false\n", __FILE__, __func__);
       mdelay(10);
       ret = dm_gpio_set_value(&priv->reset, true);
       if (ret)
               printf("%s, %s : reset gpio fails to set true\n", __FILE__, __func__);
       mdelay(120);

       ret = gpio_request_by_name(dev, "backlight-gpio", 0, &priv->backlight,
                                   GPIOD_IS_OUT);
       if (ret) {
               printf("%s, %s : Warning: cannot get reset GPIO\n", __FILE__, __func__);
               if (ret != -ENOENT)
                       return ret;
       }

       ret = dm_gpio_set_value(&priv->reset, true);
       if (ret)
               printf("%s, %s : backlight gpio fails to set true\n", __FILE__, __func__);
       printf("%s, %s : probe done \n", __FILE__, __func__);

       return 0;
}

static int ST7701S_panel_disable(struct udevice *dev)
{
       struct ST7701S_panel_priv *priv = dev_get_priv(dev);
       if (&priv->reset != NULL)
       	dm_gpio_set_value(&priv->reset, true);

       return 0;
}

static const struct panel_ops ST7701S_panel_ops = {
       .enable_backlight = ST7701S_panel_enable_backlight,
       .get_display_timing = ST7701S_panel_get_display_timing,
};

static const struct udevice_id ST7701S_panel_ids[] = {
       { .compatible = "sitronix,st7701s" },
       { }
};

U_BOOT_DRIVER(ST7701S_panel) = {
       .name                     = "ST7701S_panel",
       .id                       = UCLASS_PANEL,
       .of_match                 = ST7701S_panel_ids,
       .ops                      = &ST7701S_panel_ops,
       .probe                    = ST7701S_panel_probe,
       .remove                   = ST7701S_panel_disable,
//       .platdata_auto_alloc_size = sizeof(struct mipi_dsi_panel_plat),
 //      .priv_auto_alloc_size   = sizeof(struct ST7701S_panel_priv),
};
