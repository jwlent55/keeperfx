/******************************************************************************/
// Bullfrog Engine Emulation Library - for use to remake classic games like
// Syndicate Wars, Magic Carpet or Dungeon Keeper.
/******************************************************************************/
/** @file bflib_drawsdk.h
 *     Header file for bflib_drawsdk.cpp.
 * @par Purpose:
 *     Graphics drawing support sdk class.
 * @par Comment:
 *     Just a header file - #defines, typedefs, function prototypes etc.
 * @author   Tomasz Lis
 * @date     16 Nov 2008 - 21 Nov 2009
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#ifndef BFLIB_DRAWSDK_H
#define BFLIB_DRAWSDK_H

#include "bflib_basics.h"
#include "bflib_video.h"
#include "bflib_drawbas.h"
#include <ddraw.h>

#pragma pack(1)
/******************************************************************************/

struct TSurface {
    LPDIRECTDRAWSURFACE lpDDSurf;
    long field_4;
    unsigned long locks_count;
    long field_C;
    long field_10;
    long field_14;
};

#pragma pack()
/******************************************************************************/

// Exported class
class TDDrawSdk : public TDDrawBaseClass {
 public:
    TDDrawSdk(void);
    virtual ~TDDrawSdk(void);
    // Virtual methods from superclass
    bool setup_window(void);
    long WindowProc(HWND hWnd, unsigned int message, WPARAM wParam, LPARAM lParam);
    void find_video_modes(void);
    bool get_palette(void *,unsigned long,unsigned long);
    bool set_palette(void *,unsigned long,unsigned long);
    bool setup_screen(TbScreenMode);
    bool lock_screen(void);
    bool unlock_screen(void);
    bool clear_screen(unsigned long);
    bool clear_window(long,long,unsigned long,unsigned long,unsigned long);
    bool swap_screen(void);
    bool reset_screen(void);
    bool restore_surfaces(void);
    void wait_vbi(void);
    bool swap_box(struct tagPOINT,struct tagRECT &);
    bool create_surface(struct TSurface *,unsigned long,unsigned long);
    bool release_surface(struct TSurface *);
    bool blt_surface(struct TSurface *,unsigned long,unsigned long,tagRECT *,unsigned long);
    void *lock_surface(struct TSurface *);
    bool unlock_surface(struct TSurface *);
    void LoresEmulation(bool);
    // Nonvirtual methods
    static HRESULT CALLBACK screen_mode_callback(LPDDSURFACEDESC lpDDSurf, LPVOID lpContext);
    static struct TbScreenModeInfo *get_mode_info(unsigned short mode);
    static enum TbScreenMode get_mode_info_by_str(char *str);
    bool setup_direct_draw(void);
    bool reset_direct_draw(void);
    bool setup_dds_double_video(void);
    bool setup_dds_single_video(void);
    bool setup_dds_system(void);
    bool setup_surfaces(short, short, short);
    bool release_surfaces(void);
    bool release_palettes(void);
    LPDIRECTDRAWSURFACE wscreen_surface(void);
    bool initFail(const char *msgtext);
    void SetIcon(void);
    static DWORD CALLBACK sdk_window_thread(LPVOID);
    bool create_sdk_window(void);
    bool remove_sdk_window(void);
 protected:
    LPCTSTR resource_mapping(int index);
    void SendDDMsg(int, void *);
    HRESULT ResultDDMsg(void);
    // Properties
    LPDIRECTDRAWSURFACE lpDDSurface2;
    LPDIRECTDRAWSURFACE lpDDSurface1;
    LPDIRECTDRAWPALETTE lpDDPalette1;
    LPDIRECTDRAWPALETTE lpDDPalette2;
    HRESULT ddResult;
    DDCAPS ddDriverCaps;
    DDSCAPS field_170;
    unsigned long vidMode;
    unsigned long resWidth;
    unsigned long resHeight;
  unsigned long field_180;
    int window_created;
    HANDLE hThread;
  unsigned long field_18C;
    LPDIRECTDRAW lpDDInterface;
    LPDIRECTDRAWSURFACE lpDDSurface3;
    };
/******************************************************************************/

#endif
