/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file gui_draw.c
 *     GUI elements drawing functions.
 * @par Purpose:
 *     On-screen drawing of GUI elements, like buttons, menus and panels.
 * @par Comment:
 *     None.
 * @author   Tomasz Lis
 * @date     20 Jan 2009 - 30 Jan 2009
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#include "pre_inc.h"
#include "gui_draw.h"

#include "globals.h"
#include "bflib_basics.h"
#include "bflib_video.h"
#include "bflib_sprite.h"
#include "bflib_planar.h"
#include "bflib_vidraw.h"
#include "bflib_sprfnt.h"
#include "bflib_guibtns.h"
#include "config_strings.h"

#include "player_data.h"
#include "front_simple.h"
#include "frontend.h"
#include "config_spritecolors.h"
#include "custom_sprites.h"
#include "sprites.h"
#include "post_inc.h"
#include "frontmenu_ingame_tabs.h"

#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************/
char gui_textbuf[TEXT_BUFFER_LENGTH];
unsigned char *gui_slab;
unsigned char *frontend_background;
struct TbSpriteSheet * frontend_sprite = NULL;
int gui_blink_rate = 1; // Number of frames before menu/map effects flash. Default value, overwritten by cfg setting.
int neutral_flash_rate = 1; // Number of frames before neutral rooms/creatures cycle colours. Default value, overwritten by cfg setting.
/******************************************************************************/

int get_bitmap_max_scale(int img_w,int img_h,int rect_w,int rect_h)
{
    int m;
    int w = 0;
    int h = 0;
    for (m=0; m < 5; m++)
    {
        w += img_w;
        h += img_h;
        if (w > rect_w) break;
        if (h > rect_h) break;
    }
    // The image width can't be larger than video resolution
    if (m < 1)
    {
        if (w > lbDisplay.PhysicalScreenWidth)
          return 0;
        m = 1;
    }
    return m;
}

void draw_bar64k(long pos_x, long pos_y, int units_per_px, long width)
{
    if (width < 72*units_per_px/16)
    {
        ERRORLOG("Bar is too small");
        return;
    }
    // Button opening sprite
    const struct TbSprite* spr = get_button_sprite(GBS_frontend_button_std_l);
    long x = pos_x;
    LbSpriteDrawResized(x, pos_y, units_per_px, spr);
    x += (spr->SWidth * units_per_px + 8) / 16;
    // Button body
    long body_end = pos_x + width - 2 * ((32 * units_per_px + 8) / 16);
    while (x < body_end)
    {
        spr = get_button_sprite(GBS_frontend_button_std_c);
        LbSpriteDrawResized(x/pixel_size, pos_y/pixel_size, units_per_px, spr);
        x += spr->SWidth * units_per_px / 16;
    }
    x = body_end;
    spr = get_button_sprite(GBS_frontend_button_std_c);
    LbSpriteDrawResized(x/pixel_size, pos_y/pixel_size, units_per_px, spr);
    x += (spr->SWidth * units_per_px + 8) / 16;
    // Button ending sprite
    spr = get_button_sprite(GBS_frontend_button_std_r);
    LbSpriteDrawResized(x/pixel_size, pos_y/pixel_size, units_per_px, spr);
}

void draw_lit_bar64k(long pos_x, long pos_y, int units_per_px, long width)
{
    if (width < 32*units_per_px/16)
    {
        ERRORLOG("Bar is too small");
        return;
    }
    // opening sprite
    long x = pos_x;
    const struct TbSprite* spr = get_button_sprite(GBS_frontend_button_sta_l);
    LbSpriteDrawResized(x, pos_y, units_per_px, spr);
    x += (spr->SWidth * units_per_px + 8) / 16;
    // body
    long body_end = pos_x + width - 2 * ((32 * units_per_px + 8) / 16);
    while (x < body_end)
    {
        spr = get_button_sprite(GBS_frontend_button_sta_c);
        LbSpriteDrawResized(x, pos_y, units_per_px, spr);
        x += (spr->SWidth * units_per_px + 8) / 16;
    }
    x = body_end;
    spr = get_button_sprite(GBS_frontend_button_sta_c);
    LbSpriteDrawResized(x, pos_y, units_per_px, spr);
    x += (spr->SWidth * units_per_px + 8) / 16;
    // ending sprite
    spr = get_button_sprite(GBS_frontend_button_sta_r);
    LbSpriteDrawResized(x, pos_y, units_per_px, spr);
}

void draw_slab64k_background(long pos_x, long pos_y, long width, long height)
{
    long i;
    long scr_x = pos_x / pixel_size;
    long scr_y = pos_y / pixel_size;
    long scr_h = height / pixel_size;
    long scr_w = width / pixel_size;
    if (scr_x < 0)
    {
        i = scr_x + width / pixel_size;
        scr_x = 0;
        scr_w = i;
    }
    if (scr_y < 0)
    {
        i = scr_y + scr_h;
        scr_y = 0;
        scr_h = i;
    }
    i = lbDisplay.PhysicalScreenWidth * pixel_size;
    if (scr_x + scr_w > i)
        scr_w = i - scr_x;
    i = MyScreenHeight;
    if (scr_y + scr_h > i)
        scr_h = i - scr_y;
    TbPixel* out = &lbDisplay.WScreen[scr_x + lbDisplay.GraphicsScreenWidth * scr_y];
    for (i=0; scr_h > i; i++)
    {
        TbPixel* inp = &gui_slab[GUI_SLAB_DIMENSION * (i % GUI_SLAB_DIMENSION)];
        if (scr_w >= GUI_SLAB_DIMENSION)
        {
            memcpy(out, inp, GUI_SLAB_DIMENSION);
            int k;
            for (k = GUI_SLAB_DIMENSION; k < scr_w - GUI_SLAB_DIMENSION; k += GUI_SLAB_DIMENSION)
            {
                memcpy(out + k, inp, GUI_SLAB_DIMENSION);
            }
            if (width - k > 0) {
                memcpy(out + k, inp, scr_w - k);
            }
        } else
        {
            memcpy(out, inp, scr_w);
        }
        out += lbDisplay.GraphicsScreenWidth;
    }
}

void draw_slab64k(long pos_x, long pos_y, int units_per_px, long width, long height)
{
    // Draw one pixel more, to make sure we won't get empty area after scaling
    draw_slab64k_background(pos_x, pos_y, width+scale_value_for_resolution_with_upp(1,units_per_px), height+scale_value_for_resolution_with_upp(1,units_per_px));
    const struct TbSprite* spr = get_button_sprite(GBS_borders_frame_thck_tl);
    int bs_units_per_spr = calculate_relative_upp(16, units_per_px, spr->SWidth);
    int border_shift = scale_value_for_resolution_with_upp(6,units_per_px);
    int i;
    int i_increment = units_per_px;
    for (i = i_increment - border_shift; i < width-2*border_shift; i += i_increment)
    {
        spr = get_button_sprite(GBS_borders_frame_thck_tc);
        LbSpriteDrawResized(pos_x + i, pos_y - border_shift, bs_units_per_spr, spr);
        spr = get_button_sprite(GBS_borders_frame_thck_bc);
        LbSpriteDrawResized(pos_x + i, pos_y + height, bs_units_per_spr, spr);
    }
    for (i = i_increment - border_shift; i < height-2*border_shift; i += i_increment)
    {
        spr = get_button_sprite(GBS_borders_frame_thck_ml);
        LbSpriteDrawResized(pos_x - border_shift, pos_y + i, bs_units_per_spr, spr);
        spr = get_button_sprite(GBS_borders_frame_thck_mr);
        LbSpriteDrawResized(pos_x + width, pos_y + i, bs_units_per_spr, spr);
    }
    spr = get_button_sprite(GBS_borders_frame_thck_tl);
    LbSpriteDrawResized(pos_x - border_shift, pos_y - border_shift, bs_units_per_spr, spr);
    spr = get_button_sprite(GBS_borders_frame_thck_tr);
    LbSpriteDrawResized(pos_x + width - 2*border_shift, pos_y - border_shift, bs_units_per_spr, spr);
    spr = get_button_sprite(GBS_borders_frame_thck_bl);
    LbSpriteDrawResized(pos_x - border_shift, pos_y + height - 2*border_shift, bs_units_per_spr, spr);
    spr = get_button_sprite(GBS_borders_frame_thck_br);
    LbSpriteDrawResized(pos_x + width - 2*border_shift, pos_y + height - 2*border_shift, bs_units_per_spr, spr);
}

void draw_ornate_slab64k(long pos_x, long pos_y, int units_per_px, long width, long height)
{
    draw_slab64k_background(pos_x, pos_y, width, height);
    const struct TbSprite* spr = get_button_sprite(GBS_parchment_map_frame_deco_a_tl);
    int bs_units_per_spr = scale_ui_value(2048/spr->SWidth);
    int i;
    for (i= scale_ui_value(10); i < width- scale_ui_value(12); i+= scale_ui_value(32))
    {
        spr = get_button_sprite(GBS_borders_frame_thin_tc);
        LbSpriteDrawResized(pos_x + i, pos_y - scale_ui_value(4), bs_units_per_spr, spr);
        spr = get_button_sprite(GBS_borders_frame_thin_bc);
        LbSpriteDrawResized(pos_x + i, pos_y + height, bs_units_per_spr, spr);
    }
    for (i= scale_ui_value(10); i < height- scale_ui_value(16); i+= scale_ui_value(32))
    {
        spr = get_button_sprite(GBS_borders_frame_thin_ml);
        LbSpriteDrawResized(pos_x - scale_ui_value(4), pos_y + i, bs_units_per_spr, spr);
        spr = get_button_sprite(GBS_borders_frame_thin_mr);
        LbSpriteDrawResized(pos_x + width, pos_y + i, bs_units_per_spr, spr);
    }
    spr = get_button_sprite(GBS_borders_frame_thin_tl);
    LbSpriteDrawResized(pos_x - scale_ui_value(4), pos_y - scale_ui_value(4), bs_units_per_spr, spr);
    spr = get_button_sprite(GBS_borders_frame_thin_tr);
    LbSpriteDrawResized(pos_x + width - scale_ui_value(28), pos_y - scale_ui_value(4), bs_units_per_spr, spr);
    spr = get_button_sprite(GBS_borders_frame_thin_bl);
    LbSpriteDrawResized(pos_x - scale_ui_value(4), pos_y + height - scale_ui_value(28), bs_units_per_spr, spr);
    spr = get_button_sprite(GBS_borders_frame_thin_br);
    LbSpriteDrawResized(pos_x + width - scale_ui_value(28), pos_y + height - scale_ui_value(28), bs_units_per_spr, spr);
    spr = get_button_sprite(GBS_parchment_map_frame_deco_a_tl);
    LbSpriteDrawResized(pos_x - scale_ui_value(32), pos_y - scale_ui_value(14), bs_units_per_spr, spr);
    spr = get_button_sprite(GBS_parchment_map_frame_deco_a_bl);
    LbSpriteDrawResized(pos_x - scale_ui_value(34), pos_y + height - scale_ui_value(78), bs_units_per_spr, spr);
    lbDisplay.DrawFlags |= Lb_SPRITE_FLIP_HORIZ;
    spr = get_button_sprite(GBS_parchment_map_frame_deco_a_tl);
    LbSpriteDrawResized(pos_x + width - scale_ui_value(96), pos_y - scale_ui_value(14), bs_units_per_spr, spr);
    spr = get_button_sprite(GBS_parchment_map_frame_deco_a_bl);
    LbSpriteDrawResized(pos_x + width - scale_ui_value(92), pos_y + height - scale_ui_value(78), bs_units_per_spr, spr);
    lbDisplay.DrawFlags &= ~Lb_SPRITE_FLIP_HORIZ;
}

void draw_ornate_slab_outline64k(long pos_x, long pos_y, int units_per_px, long width, long height)
{
    const struct TbSprite* spr = get_button_sprite(GBS_parchment_map_frame_deco_a_tl);
    int bs_units_per_spr = scale_ui_value_lofi(2048)/spr->SWidth;
    long x = pos_x;
    long y = pos_y;
    int i;
    for (i = scale_ui_value_lofi(10); i < width - scale_ui_value_lofi(12); i += scale_ui_value_lofi(32))
    {
        spr = get_button_sprite(GBS_borders_frame_thin_tc);
        LbSpriteDrawResized(pos_x + i, pos_y - scale_ui_value_lofi(4), bs_units_per_spr, spr);
        spr = get_button_sprite(GBS_borders_frame_thin_bc);
        LbSpriteDrawResized(pos_x + i, pos_y + height, bs_units_per_spr, spr);
    }
    for (i= scale_ui_value_lofi(10); i < height - scale_ui_value_lofi(16); i+= scale_ui_value_lofi(32))
    {
        spr = get_button_sprite(GBS_borders_frame_thin_ml);
        LbSpriteDrawResized(x - scale_ui_value_lofi(4), y + i, bs_units_per_spr, spr);
        spr = get_button_sprite(GBS_borders_frame_thin_mr);
        LbSpriteDrawResized(x + width, y + i, bs_units_per_spr, spr);
    }
    spr = get_button_sprite(GBS_borders_frame_thin_tl);
    LbSpriteDrawResized(x - scale_ui_value_lofi(4),          y - scale_ui_value_lofi(4),           bs_units_per_spr, spr);
    spr = get_button_sprite(GBS_borders_frame_thin_tr);
    LbSpriteDrawResized(x + width - scale_ui_value_lofi(28), y - scale_ui_value_lofi(4),           bs_units_per_spr, spr);
    spr = get_button_sprite(GBS_borders_frame_thin_bl);
    LbSpriteDrawResized(x - scale_ui_value_lofi(4),          y + height - scale_ui_value_lofi(28), bs_units_per_spr, spr);
    spr = get_button_sprite(GBS_borders_frame_thin_br);
    LbSpriteDrawResized(x + width - scale_ui_value_lofi(28), y + height - scale_ui_value_lofi(28), bs_units_per_spr, spr);
    spr = get_button_sprite(GBS_parchment_map_frame_deco_a_tl);
    LbSpriteDrawResized(x - scale_ui_value_lofi(32), y - scale_ui_value_lofi(14),          bs_units_per_spr, spr);
    spr = get_button_sprite(GBS_parchment_map_frame_deco_a_bl);
    LbSpriteDrawResized(x - scale_ui_value_lofi(34), y + height - scale_ui_value_lofi(78), bs_units_per_spr, spr);
    lbDisplay.DrawFlags |= Lb_SPRITE_FLIP_HORIZ;
    spr = get_button_sprite(GBS_parchment_map_frame_deco_a_tl);
    LbSpriteDrawResized(x + width - scale_ui_value_lofi(96), y - scale_ui_value_lofi(14),          bs_units_per_spr, spr);
    spr = get_button_sprite(GBS_parchment_map_frame_deco_a_bl);
    LbSpriteDrawResized(x + width - scale_ui_value_lofi(92), y + height - scale_ui_value_lofi(78), bs_units_per_spr, spr);
    lbDisplay.DrawFlags &= ~Lb_SPRITE_FLIP_HORIZ;
}

void draw_round_slab64k(long pos_x, long pos_y, int units_per_px, long width, long height, long style_type)
{
    unsigned short drwflags_mem = lbDisplay.DrawFlags;
    lbDisplay.DrawFlags &= ~Lb_SPRITE_OUTLINE;
    if (style_type == ROUNDSLAB64K_LIGHT) {
        lbDisplay.DrawFlags |= Lb_SPRITE_TRANSPAR4;
        LbDrawBox(pos_x + scale_ui_value_lofi(4), pos_y + scale_ui_value_lofi(4), width - scale_ui_value_lofi(8), height - scale_ui_value_lofi(8), 1);
        lbDisplay.DrawFlags &= ~Lb_SPRITE_TRANSPAR4;
    } else {
        lbDisplay.DrawFlags |= Lb_SPRITE_TRANSPAR8;
        LbDrawBox(pos_x + scale_ui_value_lofi(4), pos_y + scale_ui_value_lofi(4), width - scale_ui_value_lofi(8), height - scale_ui_value_lofi(8), 1);
        lbDisplay.DrawFlags &= ~Lb_SPRITE_TRANSPAR8;
    }
    int x;
    int y;
    const struct TbSprite* spr = get_panel_sprite(GPS_message_frame_thin_hex_ct);
    int ps_units_per_spr = scale_ui_value_lofi(416)/spr->SWidth;
    long i;
    for (i = 0; i < width - scale_ui_value_lofi(68); i += scale_ui_value_lofi(26))
    {
        x = pos_x + i + scale_ui_value_lofi(34);
        y = pos_y;
        spr = get_panel_sprite(GPS_message_frame_thin_hex_ct);
        LbSpriteDrawResized(x, y, ps_units_per_spr, spr);
        y += height - scale_ui_value_lofi(4);
        spr = get_panel_sprite(GPS_message_frame_thin_hex_cb);
        LbSpriteDrawResized(x, y, ps_units_per_spr, spr);
    }
    for (i = 0; i < height - scale_ui_value_lofi(56); i += scale_ui_value_lofi(20))
    {
        x = pos_x;
        y = pos_y + i + scale_ui_value_lofi(28);
        spr = get_panel_sprite(GPS_message_frame_thin_hex_cr);
        LbSpriteDrawResized(x, y, ps_units_per_spr, spr);
        x += width - scale_ui_value_lofi(4);
        spr = get_panel_sprite(GPS_message_frame_thin_hex_cl);
        LbSpriteDrawResized(x, y, ps_units_per_spr, spr);
    }
    x = pos_x + width - scale_ui_value_lofi(34);
    y = pos_y + height - scale_ui_value_lofi(28);
    spr = get_panel_sprite(GPS_message_frame_thin_hex_tl);
    LbSpriteDrawResized(pos_x, pos_y, ps_units_per_spr, spr);
    spr = get_panel_sprite(GPS_message_frame_thin_hex_tr);
    LbSpriteDrawResized(x,     pos_y, ps_units_per_spr, spr);
    spr = get_panel_sprite(GPS_message_frame_thin_hex_bl);
    LbSpriteDrawResized(pos_x, y,     ps_units_per_spr, spr);
    spr = get_panel_sprite(GPS_message_frame_thin_hex_br);
    LbSpriteDrawResized(x,     y,     ps_units_per_spr, spr);
    lbDisplay.DrawFlags = drwflags_mem;
}

/**
 * Returns units-per-pixel to be used for drawing given GUI button, assuming it consists of one panel sprite.
 * Uses sprite height as constant factor.
 * @param gbtn
 * @param spridx
 * @return
 */
int simple_gui_panel_sprite_height_units_per_px(const struct GuiButton *gbtn, long spridx, int fraction)
{
    const struct TbSprite* spr = get_panel_sprite(spridx);
    if (spr->SHeight < 1)
        return 16;
    int units_per_px = ((gbtn->height * fraction / 100) * 16 + spr->SHeight / 2) / spr->SHeight;
    if (units_per_px < 1)
        units_per_px = 1;
    return units_per_px;
}

/**
 * Returns units-per-pixel to be used for drawing given GUI button, assuming it consists of one panel sprite.
 * Uses sprite width as constant factor.
 * @param gbtn
 * @param spridx
 * @return
 */
int simple_gui_panel_sprite_width_units_per_px(const struct GuiButton *gbtn, long spridx, int fraction)
{
    const struct TbSprite* spr = get_panel_sprite(spridx);
    if (spr->SWidth < 1)
        return 16;
    int units_per_px = ((gbtn->width * fraction / 100) * 16 + spr->SWidth / 2) / spr->SWidth;
    if (units_per_px < 1)
        units_per_px = 1;
    return units_per_px;
}

/**
 * Returns units-per-pixel to be used for drawing given GUI button, assuming it consists of one button sprite.
 * Uses sprite height as constant factor.
 * @param gbtn
 * @param spridx
 * @return
 */
int simple_button_sprite_height_units_per_px(const struct GuiButton *gbtn, long spridx, int fraction)
{
    const struct TbSprite* spr = get_button_sprite_for_player(spridx, my_player_number);
    if (spr->SHeight < 1)
        return 16;
    int units_per_px = ((gbtn->height * fraction / 100) * 16 + spr->SHeight / 2) / spr->SHeight;
    if (units_per_px < 1)
        units_per_px = 1;
    return units_per_px;
}

/**
 * Returns units-per-pixel to be used for drawing given GUI button, assuming it consists of one button sprite.
 * Uses sprite width as constant factor.
 * @param gbtn
 * @param spridx
 * @return
 */
int simple_button_sprite_width_units_per_px(const struct GuiButton *gbtn, long spridx, int fraction)
{
    const struct TbSprite* spr = get_button_sprite_for_player(spridx, my_player_number);
    if (spr->SWidth < 1)
        return 16;
    int units_per_px = ((gbtn->width * fraction / 100) * 16 + spr->SWidth / 2) / spr->SWidth;
    if (units_per_px < 1)
        units_per_px = 1;
    return units_per_px;
}

/**
 * Returns units-per-pixel to be used for drawing given GUI button, assuming it consists of one sprite.
 * Uses sprite height as constant factor.
 * @param gbtn
 * @param spridx
 * @return
 */
int simple_frontend_sprite_height_units_per_px(const struct GuiButton *gbtn, long spridx, int fraction)
{
    const struct TbSprite* spr = get_frontend_sprite(spridx);
    if (spr->SHeight < 1)
        return 16;
    int units_per_px = ((gbtn->height * fraction / 100) * 16 + spr->SHeight / 2) / spr->SHeight;
    if (units_per_px < 1)
        units_per_px = 1;
    return units_per_px;
}

/**
 * Returns units-per-pixel to be used for drawing given GUI button, assuming it consists of one sprite.
 * Uses sprite width as constant factor.
 * @param gbtn
 * @param spridx
 * @return
 */
int simple_frontend_sprite_width_units_per_px(const struct GuiButton *gbtn, long spridx, int fraction)
{
    const struct TbSprite* spr = get_frontend_sprite(spridx);
    if (spr->SWidth < 1)
        return 16;
    int units_per_px = ((gbtn->width * fraction / 100) * 16 + spr->SWidth / 2) / spr->SWidth;
    if (units_per_px < 1)
        units_per_px = 1;
    return units_per_px;
}

/** Draws a string on GUI button.
 *  Note that the source text buffer may be damaged by this function.
 * @param gbtn Button to draw text on.
 * @param base_width Width of the button before scaling.
 * @param text Text to be displayed.
 */
void draw_button_string(struct GuiButton *gbtn, int base_width, const char *text)
{
    static unsigned char cursor_type = 0;
    unsigned long flgmem = lbDisplay.DrawFlags;
    long cursor_pos = -1;
    static char dtext[TEXT_BUFFER_LENGTH];
    snprintf(dtext, TEXT_BUFFER_LENGTH, "%s", text);
    if ((gbtn->gbtype == LbBtnT_EditBox) && (gbtn == input_button))
    {
        cursor_type++;
        if ((cursor_type & 0x02) == 0)
          cursor_pos = input_field_pos;
        LbLocTextStringConcat(dtext, " ", TEXT_BUFFER_LENGTH);
        lbDisplay.DrawColour = LbTextGetFontFaceColor();
        lbDisplayEx.ShadowColour = LbTextGetFontBackColor();
    }
    TbBool low_res = ( (MyScreenHeight < 400) && (dbc_language > 0) );
    int width = gbtn->width;
    int x = gbtn->scr_pos_x;
    if (low_res)
    {
        // TODO: Is there a better way of adjusting for East Asian text? This is ridiculous.
        width += 32;
        switch (gbtn->tooltip_stridx)
        {
            case GUIStr_PickCreatrIdleDesc:
            case GUIStr_PickCreatrWorkingDesc:
            case GUIStr_PickCreatrFightingDesc:
            {
                x -= gbtn->width;
                break;
            }
            case GUIStr_MnuCancel:
            {
                x -= 12;
                break;
            }
            case GUIStr_ExperienceDesc:
            {
                x -= 16;
                break;
            }
            default:
            {
                x -= 8;
                break;
            }
        }
    }
    LbTextSetJustifyWindow(x, gbtn->scr_pos_y, width);
    LbTextSetClipWindow(x, gbtn->scr_pos_y, width, gbtn->height);
    lbDisplay.DrawFlags = Lb_TEXT_HALIGN_CENTER;// | Lb_TEXT_UNDERLNSHADOW;
    if (cursor_pos >= 0) {
        // Mind the order, 'cause inserting makes positions shift
        LbLocTextStringInsert(dtext, "\x0B", cursor_pos+1, TEXT_BUFFER_LENGTH);
        LbLocTextStringInsert(dtext, "\x0B", cursor_pos, TEXT_BUFFER_LENGTH);
    }
    int units_per_px = (gbtn->width * 16 + base_width / 2) / base_width;
    int tx_units_per_px = (units_per_px * 22 / LbTextLineHeight());
    unsigned long w = 4 * units_per_px / 16;
    if (low_res)
    {
        if ( (gbtn->tooltip_stridx != GUIStr_PickCreatrIdleDesc) && (gbtn->tooltip_stridx != GUIStr_PickCreatrWorkingDesc) && (gbtn->tooltip_stridx != GUIStr_PickCreatrFightingDesc) )
        {
            tx_units_per_px += (units_per_px / 2);
        }
    }
    unsigned long h = (gbtn->height - text_string_height(tx_units_per_px, dtext)) / 2 - 3 * units_per_px / 16;
    if (dbc_language > 0)
    {
        if (gbtn->id_num == BID_QUERY_INFO)
        {
            if (MyScreenWidth > 640)
            {
                h += (13 + (MyScreenWidth / 640));
                w += 8;
                tx_units_per_px = scale_value_by_horizontal_resolution(10);
            }
        }
        else if (gbtn->id_num == BID_DUNGEON_INFO)
        {
            if (MyScreenWidth > 640)
            {
                h += (12 + (MyScreenWidth / 640));
                w += 8;
                tx_units_per_px = scale_value_by_horizontal_resolution(12);
            }
        }
        else if (gbtn->tooltip_stridx == GUIStr_ExperienceDesc)
        {
            if (MyScreenWidth > 640)
            {
                h += (8 + (MyScreenWidth / 640));
            }
        }
    }
    LbTextDrawResized(w, h, tx_units_per_px, dtext);
    LbTextSetJustifyWindow(0, 0, LbGraphicsScreenWidth());
    LbTextSetClipWindow(0/pixel_size, 0/pixel_size, MyScreenWidth/pixel_size, MyScreenHeight/pixel_size);
    LbTextSetWindow(0/pixel_size, 0/pixel_size, MyScreenWidth/pixel_size, MyScreenHeight/pixel_size);
    lbDisplay.DrawFlags = flgmem;
}

void draw_message_box_at(long startx, long starty, long box_width, long box_height, long spritesx, long spritesy)
{
    const struct TbSprite *spr;
    long n;

    // Draw top line of sprites
    long x = startx;
    long y = starty;
    {
        spr = get_frontend_sprite(GFS_hugearea_thn_cor_tl);
        LbSpriteDrawResized(x, y, units_per_pixel, spr);
        x += spr->SWidth * units_per_pixel / 16;
    }
    for (n=0; n < spritesx; n++)
    {
        spr = get_frontend_sprite((n % 4) + GFS_hugearea_thn_tx1_tc);
        LbSpriteDrawResized(x, y, units_per_pixel, spr);
        x += spr->SWidth * units_per_pixel / 16;
    }
    x = startx;
    {
        spr = get_frontend_sprite(GFS_hugearea_thn_cor_tl);
        x += spr->SWidth * units_per_pixel / 16;
    }
    for (n=0; n < spritesx; n++)
    {
        spr = get_frontend_sprite((n % 4) + GFS_hugearea_thn_tx1_tc);
        LbSpriteDrawResized(x, y, units_per_pixel, spr);
        x += spr->SWidth * units_per_pixel / 16;
    }
    {
        spr = get_frontend_sprite(GFS_hugearea_thn_cor_tr);
        LbSpriteDrawResized(x, y, units_per_pixel, spr);
    }
    // Draw centered line of sprites
    spr = get_frontend_sprite(GFS_hugearea_thn_cor_tl);
    x = startx;
    y += spr->SHeight * units_per_pixel / 16;
    {
        spr = get_frontend_sprite(GFS_hugearea_thc_cor_ml);
        LbSpriteDrawResized(x, y, units_per_pixel, spr);
        x += spr->SWidth * units_per_pixel / 16;
    }
    for (n=0; n < spritesx; n++)
    {
        spr = get_frontend_sprite((n % 4) + GFS_hugearea_thc_tx1_mc);
        LbSpriteDrawResized(x, y, units_per_pixel, spr);
        x += spr->SWidth * units_per_pixel / 16;
    }
    {
        spr = get_frontend_sprite(GFS_hugearea_thc_cor_mr);
        LbSpriteDrawResized(x, y, units_per_pixel, spr);
    }
    // Draw bottom line of sprites
    spr = get_frontend_sprite(GFS_hugearea_thc_cor_ml);
    x = startx;
    y += spr->SHeight * units_per_pixel / 16;
    {
        spr = get_frontend_sprite(GFS_hugearea_thn_cor_bl);
        LbSpriteDrawResized(x, y, units_per_pixel, spr);
        x += spr->SWidth * units_per_pixel / 16;
    }
    for (n=0; n < spritesx; n++)
    {
        spr = get_frontend_sprite((n % 4) + GFS_hugearea_thn_tx1_bc);
        LbSpriteDrawResized(x, y, units_per_pixel, spr);
        x += spr->SWidth * units_per_pixel / 16;
    }
    {
        spr = get_frontend_sprite(GFS_hugearea_thn_cor_br);
        LbSpriteDrawResized(x, y, units_per_pixel, spr);
    }
}

TbBool draw_text_box(const char *text)
{
    long spritesy;
    long spritesx;
    LbTextSetFont(frontend_font[1]);
    long n = LbTextStringWidth(text);
    if (n < (4*108)) {
        spritesy = 1;
        spritesx = n / 108;
    } else {
        spritesx = 4;
        spritesy = n / (3*108);
    }
    if (spritesy > 4) {
      ERRORLOG("Text too long for error box");
    }
    if (spritesx < 2) {
        spritesx = 2;
    } else
    if (spritesx > 4) {
        spritesx = 4;
    }
    long box_width = (108 * spritesx + 18) * units_per_pixel / 16;
    long box_height = 92 * units_per_pixel / 16;
    long startx = (lbDisplay.PhysicalScreenWidth - box_width) / 2;
    long starty = (lbDisplay.PhysicalScreenHeight - box_height) / 2;
    draw_message_box_at(startx, starty, box_width, box_height, spritesx, spritesy);
    // Draw the text inside box
    lbDisplay.DrawFlags = Lb_TEXT_HALIGN_CENTER;
    int tx_units_per_px = ((box_height / 4) * 13 / 11) * 16 / LbTextLineHeight();
    LbTextSetWindow(startx, starty, box_width, box_height);
    n = LbTextLineHeight() * tx_units_per_px / 16;
    return LbTextDrawResized(0, (box_height - spritesy * n) / 2, tx_units_per_px, text);
}

int scroll_box_get_units_per_px(struct GuiButton *gbtn)
{
    int width = 0;
    int spridx = GFS_hugearea_thc_cor_ml;
    const struct TbSprite* spr = get_frontend_sprite(spridx);
    for (int i = 6; i > 0; i--)
    {
        width += spr->SWidth;
        spr++;
    }
    return (gbtn->width * 16 + 8) / width;
}

void draw_scroll_box(struct GuiButton *gbtn, int units_per_px, int num_rows)
{
    const struct TbSprite *spr;
    int pos_x;
    int i;
    lbDisplay.DrawFlags = 0;
    int pos_y = gbtn->scr_pos_y;
    { // First row
        pos_x = gbtn->scr_pos_x;
        spr = get_frontend_sprite(GFS_hugearea_thn_cor_tl);
        for (i = 6; i > 0; i--)
        {
            LbSpriteDrawResized(pos_x, pos_y, units_per_px, spr);
            pos_x += spr->SWidth * units_per_px / 16;
            spr++;
        }
        spr = get_frontend_sprite(GFS_hugearea_thn_cor_tl);
        pos_y += spr->SHeight * units_per_px / 16;
    }
    // Further rows
    while (num_rows > 0)
    {
        int spridx = GFS_hugearea_thc_cor_ml;
        if (num_rows < 3)
          spridx = GFS_hugearea_thn_cor_ml;
        spr = get_frontend_sprite(spridx);
        pos_x = gbtn->scr_pos_x;
        for (i = 6; i > 0; i--)
        {
            LbSpriteDrawResized(pos_x, pos_y, units_per_px, spr);
            pos_x += spr->SWidth * units_per_px / 16;
            spr++;
        }
        spr = get_frontend_sprite(spridx);
        pos_y += spr->SHeight * units_per_px / 16;
        int delta = 3;
        if (num_rows < 3)
            delta = 1;
        num_rows -= delta;
    }
    // Last row
    spr = get_frontend_sprite(GFS_hugearea_thn_cor_bl);
    pos_x = gbtn->scr_pos_x;
    for (i = 6; i > 0; i--)
    {
        LbSpriteDrawResized(pos_x, pos_y, units_per_px, spr);
        pos_x += spr->SWidth * units_per_px / 16;
        spr++;
    }
}

void draw_gui_panel_sprite_left_player(long x, long y, int units_per_px, long spridx, PlayerNumber plyr_idx)
{
    spridx = get_player_colored_icon_idx(spridx,plyr_idx);
    const struct TbSprite* spr = get_panel_sprite(spridx);
    LbSpriteDrawResized(x, y, units_per_px, spr);
}

void draw_gui_panel_sprite_rmleft_player(long x, long y, int units_per_px, long spridx, unsigned long remap, PlayerNumber plyr_idx)
{
    spridx = get_player_colored_icon_idx(spridx, plyr_idx);
    const struct TbSprite* spr = get_panel_sprite(spridx);
    LbSpriteDrawResizedRemap(x, y, units_per_px, spr, &pixmap.fade_tables[remap*256]);
}

void draw_gui_panel_sprite_ocleft(long x, long y, int units_per_px, long spridx, TbPixel color)
{
    spridx = get_player_colored_icon_idx(spridx,my_player_number);
    const struct TbSprite* spr = get_panel_sprite(spridx);
    LbSpriteDrawResizedOneColour(x, y, units_per_px, spr, color);
}

void draw_gui_panel_sprite_centered(long x, long y, int units_per_px, long spridx)
{
    spridx = get_player_colored_icon_idx(spridx,my_player_number);
    const struct TbSprite* spr = get_panel_sprite(spridx);
    x -= ((spr->SWidth*units_per_px/16) >> 1);
    y -= ((spr->SHeight*units_per_px/16) >> 1);
    LbSpriteDrawResized(x, y, units_per_px, spr);
}

void draw_gui_panel_sprite_occentered(long x, long y, int units_per_px, long spridx, TbPixel color)
{
    spridx = get_player_colored_icon_idx(spridx,my_player_number);
    const struct TbSprite* spr = get_panel_sprite(spridx);
    x -= ((spr->SWidth*units_per_px/16) >> 1);
    y -= ((spr->SHeight*units_per_px/16) >> 1);
    LbSpriteDrawResizedOneColour(x, y, units_per_px, spr, color);
}

void draw_button_sprite_left(long x, long y, int units_per_px, long spridx)
{
    const struct TbSprite* spr = get_button_sprite_for_player(spridx, my_player_number);
    LbSpriteDrawResized(x, y, units_per_px, spr);
}

void draw_button_sprite_rmleft(long x, long y, int units_per_px, long spridx, unsigned long remap)
{
    const struct TbSprite* spr = get_button_sprite_for_player(spridx, my_player_number);
    LbSpriteDrawResizedRemap(x, y, units_per_px, spr, &pixmap.fade_tables[remap*256]);
}

void draw_frontend_sprite_left(long x, long y, int units_per_px, long spridx)
{
    const struct TbSprite* spr = get_frontend_sprite(spridx);
    LbSpriteDrawResized(x, y, units_per_px, spr);
}

void draw_string64k(long x, long y, int units_per_px, const char * text)
{
    unsigned short drwflags_mem = lbDisplay.DrawFlags;
    lbDisplay.DrawFlags &= ~Lb_TEXT_ONE_COLOR;
    LbTextDrawResized(x, y, units_per_px, text);
    lbDisplay.DrawFlags = drwflags_mem;
}

TbBool frontmenu_copy_background_at(const struct TbRect *bkgnd_area, int units_per_px)
{
    int img_width = 640;
    int img_height = 480;
    const unsigned char *srcbuf = frontend_background;
    // Only 8bpp supported for now
    if (LbGraphicsScreenBPP() != 8)
        return false;
    // Do the drawing
    copy_raw8_image_buffer(lbDisplay.WScreen,LbGraphicsScreenWidth(),LbGraphicsScreenHeight(),
        img_width*units_per_px/16,img_height*units_per_px/16,bkgnd_area->left,bkgnd_area->top,srcbuf,img_width,img_height);
    // Burning candle flames
    return true;
}

long get_frontmenu_background_area_rect(int rect_x, int rect_y, int rect_w, int rect_h, struct TbRect *bkgnd_area)
{
    int img_width = 640;
    int img_height = 480;
    // Parchment bitmap scaling
    int units_per_px = max(16 * rect_w / img_width, 16 * rect_h / img_height);
    int units_per_px_max = min(16 * 7 * rect_w / (6 * img_width), 16 * 4 * rect_h / (3 * img_height));
    if (units_per_px > units_per_px_max)
        units_per_px = units_per_px_max;
    // The image width can't be larger than video resolution
    if (units_per_px < 1) {
        units_per_px = 1;
    }
    // Set rectangle coords
    bkgnd_area->left = rect_x + (rect_w-units_per_px*img_width/16)/2;
    bkgnd_area->top = rect_y + (rect_h-units_per_px*img_height/16)/2;
    if (bkgnd_area->top < 0) bkgnd_area->top = 0;
    bkgnd_area->right = bkgnd_area->left + units_per_px*img_width/16;
    bkgnd_area->bottom = bkgnd_area->top + units_per_px*img_height/16;
    if (bkgnd_area->bottom > rect_y+rect_h) bkgnd_area->bottom = rect_y+rect_h;
    return units_per_px;
}

/**
 * Draws menu background.
 */
void draw_frontmenu_background(int rect_x,int rect_y,int rect_w,int rect_h)
{
    // Validate parameters with video mode
    TbScreenModeInfo *mdinfo = LbScreenGetModeInfo(LbScreenActiveMode());
    if (rect_w == POS_AUTO)
      rect_w = mdinfo->Width-rect_x;
    if (rect_h == POS_AUTO)
      rect_h = mdinfo->Height-rect_y;
    if (rect_w<0) rect_w=0;
    if (rect_h<0) rect_h=0;
    // Get background area rectangle
    struct TbRect bkgnd_area;
    int units_per_px = get_frontmenu_background_area_rect(rect_x, rect_y, rect_w, rect_h, &bkgnd_area);
    // Draw it
    frontmenu_copy_background_at(&bkgnd_area, units_per_px);
    SYNCDBG(9,"Done");
}
/******************************************************************************/
#ifdef __cplusplus
}
#endif
