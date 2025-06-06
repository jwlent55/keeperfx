/******************************************************************************/
// Bullfrog Engine Emulation Library - for use to remake classic games like
// Syndicate Wars, Magic Carpet or Dungeon Keeper.
/******************************************************************************/
/** @file bflib_sndlib.h
 *     Header file for bflib_sndlib.c.
 * @par Purpose:
 *     Low-level sound and music related routines.
 * @par Comment:
 *     Just a header file - #defines, typedefs, function prototypes etc.
 * @author   KeeperFX Team
 * @date     16 Nov 2008 - 30 Dec 2008
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#ifndef BFLIB_SNDLIB_H
#define BFLIB_SNDLIB_H

#include "bflib_basics.h"
#include "bflib_sound.h"
#include "sounds.h"
#include "globals.h"

#define FIRST_REDBOOK_TRACK 2
#define LAST_REDBOOK_TRACK 7

#ifdef __cplusplus
extern "C" {
#endif

void FreeAudio(void);
void SetSoundMasterVolume(SoundVolume);
TbBool GetSoundInstalled(void);
void MonitorStreamedSoundTrack(void);
void * GetSoundDriver(void);
void StopAllSamples(void);
TbBool InitAudio(const struct SoundSettings *);
TbBool IsSamplePlaying(SoundMilesID);
SoundVolume GetCurrentSoundMasterVolume(void);
void SetSampleVolume(SoundEmitterID, SoundSmplTblID, SoundVolume);
void SetSamplePan(SoundEmitterID, SoundSmplTblID, SoundPan);
void SetSamplePitch(SoundEmitterID, SoundSmplTblID, SoundPitch);
void toggle_bbking_mode(void);

void set_music_volume(SoundVolume);
TbBool play_music(const char * fname);
TbBool play_music_track(int);
void pause_music(void);
void resume_music(void);
void stop_music(void);

#ifdef __cplusplus
}
#endif
#endif
