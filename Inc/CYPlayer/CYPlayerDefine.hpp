/*
 * CYPlayer License
 * -----------
 *
 * CYPlayer is licensed under the terms of the MIT license reproduced below.
 * This means that CYPlayer is free software and can be used for both academic
 * and commercial purposes at absolutely no cost.
 *
 *
 * ===============================================================================
 *
 * Copyright (C) 2023-2026 ShiLiang.Hao <newhaosl@163.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * ===============================================================================
 */
 /*
  * AUTHORS:  ShiLiang.Hao <newhaosl@163.com>
  * VERSION:  1.0.0
  * PURPOSE:  Cross-platform efficient all-round player SDK.
  * CREATION: 2025.04.23
  * LCHANGE:  2025.04.23
  * LICENSE:  Expat/MIT License, See Copyright Notice at the begin of this file.
  */

#ifndef __CY_PLAYER_DEFINE_HPP__
#define __CY_PLAYER_DEFINE_HPP__

#include <stdint.h>

#define CYPLAYER_NAMESPACE_BEGIN        namespace cry {
#define CYPLAYER_NAMESPACE              cry
#define CYPLAYER_NAMESPACE_END          }

#ifdef _WIN32
#ifdef CYPLAYER_USE_DLL
#ifdef CYPLAYER_EXPORTS
#define CYPALYER_API __declspec(dllexport)
#else
#define CYPALYER_API __declspec(dllimport)
#endif
#else
#define CYPALYER_API
#endif
#else
#define CYPALYER_API __attribute__ ((visibility ("default")))
#endif

CYPLAYER_NAMESPACE_BEGIN

/**
 * Player Status.
 */
enum EStateType
{
    TYPE_STATUS_IDLE,           // Initial idle state
    TYPE_STATUS_INITIALIZED,    // Resources initialized
    TYPE_STATUS_PREPARED,       // Ready to start playback
    TYPE_STATUS_PLAYING,        // Currently playing
    TYPE_STATUS_PAUSED,         // Playback paused
    TYPE_STATUS_STOPPED,        // Playback stopped
    TYPE_STATUS_COMPLETED,      // Playback completed
    TYPE_STATUS_ERROR,          // An error occurred
};

/**
 * Player Event Type.
 */
enum EEventType
{
    TYPE_EVENT_PLAYBACK_FINISHED,
    TYPE_EVENT_ERROR_OCCURRED,
    TYPE_EVENT_BUFFERING_STARTED,
    TYPE_EVENT_BUFFERING_ENDED,
};

/**
 * Player Video Scale Type.
 */
enum EVideoScaleType
{
    TYPE_SCALE_VIDEO_ORIGINAL                       = 0,    // Keep the original size without scaling.
    TYPE_SCALE_VIDEO_PIXEL_PERFECT                  = 1,    // Perfect pixel scaling, using integer multiple scaling.
    TYPE_SCALE_VIDEO_ASPECT_FIT                     = 2,    // Maintain aspect ratio and adapt to windows.
    TYPE_SCALE_VIDEO_ASPECT_FILL                    = 3,    // Maintain aspect ratio and fill the window.
    TYPE_SCALE_VIDEO_STRETCH                        = 4,    // Stretch to fill the entire window.
    TYPE_SCALE_VIDEO_CENTER                         = 5,    // Center display without zooming.
    TYPE_SCALE_VIDEO_SMART                          = 6,    // Intelligent zoom, automatically select the best zoom method based on content.
    TYPE_SCALE_VIDEO_CUSTOM                         = 7,    // Customize scaling, using user-specified scaling.
    TYPE_SCALE_VIDEO_CROP                           = 8,    // Cropping mode.
    TYPE_SCALE_VIDEO_CENTER_CROP                    = 9,    // Center cut.
    TYPE_SCALE_VIDEO_CENTER_FIT                     = 10,   // Centered adaptation.
    TYPE_SCALE_VIDEO_UNIFORM                        = 11,   // Equal-scale zoom.
    TYPE_SCALE_VIDEO_NON_UNIFORM                    = 12,   // Non-equal scaling.
    TYPE_SCALE_VIDEO_SMART_FILL                     = 13,   // Smart fill.
    TYPE_SCALE_VIDEO_SMART_FIT                      = 14,   // Smart Fit.
    TYPE_SCALE_VIDEO_SMART_CROP                     = 15,   // Smart Crop.
    TYPE_SCALE_VIDEO_KEEP_ORIGINAL                  = 16,   // Keep Original Size.
    TYPE_SCALE_VIDEO_AUTO                           = 17,   // Auto Scale.
    TYPE_SCALE_VIDEO_MAX_FIT                        = 18,   // Max Fit.
    TYPE_SCALE_VIDEO_MIN_FIT                        = 19,   // Min Fit.
};

/**
 * Player Video Rotation.
 */
enum ERotationType
{
    TYPE_ROTATION_NONE                              = 0,    // No rotation, keep original orientation.
    TYPE_ROTATION_ROTATE_90                         = 90,   // Rotate 90 degrees clockwise.
    TYPE_ROTATION_ROTATE_180                        = 180,  // Rotate 180 degrees clockwise.
    TYPE_ROTATION_ROTATE_270                        = 270,  // Rotate 270 degrees clockwise.
    TYPE_ROTATION_FLIP_HORIZONTAL                   = 1,    // Flip horizontally.
    TYPE_ROTATION_FLIP_VERTICAL                     = 2,    // Flip vertically.
    TYPE_ROTATION_FLIP_HORIZONTAL_ROTATE_90         = 3,    // Flip horizontally and rotate 90 degrees
    TYPE_ROTATION_FLIP_HORIZONTAL_ROTATE_180        = 4,    // Flip horizontally and rotate 180 degrees
    TYPE_ROTATION_FLIP_HORIZONTAL_ROTATE_270        = 5,    // Flip horizontally and rotate 270 degrees
    TYPE_ROTATION_FLIP_VERTICAL_ROTATE_90           = 6,    // Flip vertically and rotate 90 degrees
    TYPE_ROTATION_FLIP_VERTICAL_ROTATE_180          = 7,    // Flip vertically and rotate 180 degrees
    TYPE_ROTATION_FLIP_VERTICAL_ROTATE_270          = 8,    // Flip vertically and rotate 270 degrees
    TYPE_ROTATION_AUTO                              = 9,    // Auto rotate based on video metadata
    TYPE_ROTATION_SMART                             = 10    // Smart rotation based on content analysis
};

/**
 * Log Type.
 */
enum ELogType
{
    TYPE_LOG_DEBUG,      // Debug information
    TYPE_LOG_INFO,       // General information
    TYPE_LOG_WARNING,    // Warning messages
    TYPE_LOG_ERROR,      // Error messages
    TYPE_LOG_FATAL       // Fatal errors
};

/**
 * Video Render Type.
 */
enum EVideoRenderType
{
    TYPE_VIDEO_RENDER_SDL,
    TYPE_VIDEO_RENDER_OPENGL,
    TYPE_VIDEO_RENDER_D3D,
};

/**
 * Audio Render Type.
 */
enum EAudioRenderType
{
    TYPE_AUDIO_RENDER_SDL,
};

/**
 * Player reference clock.
 */
enum EPlayerRefClockType
{
    TYPE_SYNC_CLOCK_AUDIO,
    TYPE_SYNC_CLOCK_VIDEO,
    TYPE_SYNC_CLOCK_EXTERNAL,   // Synchronize to an external clock.
};

/**
 * Player Event Info.
 */
struct EPlayerEventInfo
{
    EEventType eEventType;
    int nErrorCode;
    char szMessage[512];
    double fBufferProgress;     // Buffer Progress£¬0~1
};

/**
 * Player Param.
 */
struct EPlayerParam
{
    EPlayerRefClockType eClockType;
    EVideoRenderType eVideoRenderType;
    EAudioRenderType eAudioRenderType;
    bool bDisableAudio = false;
    bool bDisableVideo = false;
    bool bDisableSubTitle = false;
    bool bAutoGenPTS = true;        // Automatically generate PTS.
    bool bFindStreamInfo = true;    // ffmpeg find stream info.
    bool bShowStatus = true;
    int nInfiniteBuffer = -1;       // don't limit the input buffer size (useful with bRealTime streams).
    int nFrameDrop = -1;            // drop frames when cpu is too slow
    bool bExitOnKeyDown = false;
    float fSeekInterval = 10;       // set seek interval for left/right keys, in seconds.
    int nSeekByBytes = -1;          // seek by bytes 0=off 1=on -1=auto
};

/**
 * Player Media Param
 */
struct EPlayerMediaParam
{
    char szInputFormat[256] = {0};
    int nStartVolume = 100;          // Start volume 0-100.
    int64_t nStartTime = -1;         // Start play time.
    int64_t nDuration = -1;          // play "duration" seconds of audio/video
#define AVMEDIA_TYPE_NUMBER 5
    const char* szStreamSpec[AVMEDIA_TYPE_NUMBER] = { 0 };
    int nLoop = 1;
    bool bAutoExit = false;
    int nLowRes = 0;

    bool bFastDecode = false;
    char szForceAudioCodecName[256] = {0};
    char szForceSubtitleCodecName[256] = { 0 };
    char szForceVideoCodecName[256] = { 0 };
    char szHWAccel[256] = { 0 };     // use HW accelerated decoding.
};

/**
 * Event callback pointer function.
 */
typedef void(*FunEventCallback)(const EPlayerEventInfo*);
typedef void(*FunStateCallBack)(EStateType eState);
typedef void(*FunPositionCallBack)(int64_t nPos, int64_t nFileDuration);
typedef void(*FunLogCallBack)(ELogType eType, const char* pszMsg, const char* pszFile, const char* szLocation, int nLine);

/**
 * Return Code.
 */
enum ERetCode
{
    ERR_SUCESS = 0,
    ERR_NOT_INIT = 1,
    ERR_DO_ERROR = 2,
    ERR_VARIABLE_CONVER_FAILED = 3,
    ERR_INIT_SDL_SUBSYSTEM_FAILED = 4,
    ERR_SDL_CREATE_WINDOW_FAILED = 5,
    ERR_SDL_VULKAN_PARAM_FAILED = 6,
    ERR_SDL_VULKAN_RENDERER_CREATE_FAILED = 7,
    ERR_SDL_CREATE_RENDERDER_FAILED = 8,
    ERR_UINIT_ERROR = 9,
    ERR_OPEN_ERROR = 10,
    ERR_SETWINDOW_ERROR = 11,
    ERR_PLAY_ERROR = 12,
    ERR_SEEK_FAILED = 13,
    ERR_SETMUTE_FAILED = 14,
    ERR_SETLOOP_FAILED = 15,
    ERR_SETSPEED_FAILED = 16,
    ERR_SETVIDEOSCALE_ERROR = 17,
    ERR_SETVIDEO_ROTATION_ERROR = 18,
    ERR_SETVIDEO_MIRROR_ERROR = 19,
    ERR_SETASPECT_RATIO_ERROR = 20,
    ERR_SETVOLUME_ERROR = 21,
    ERR_FIND_INPUT_FORMAT_ERROR = 22,
    ERR_PLAYER_PARAM_NOT_VARIABLE = 23, 
    ERR_OPEN_MEDIA_ERROR = 24,
};

CYPLAYER_NAMESPACE_END

#endif // __CY_PLAYER_DEFINE_HPP__