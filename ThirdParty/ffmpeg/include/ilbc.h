/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

/*
 * ilbc.h
 *
 * This header file contains all of the API's for iLBC.
 *
 */

#ifndef MODULES_AUDIO_CODING_CODECS_ILBC_ILBC_H_
#define MODULES_AUDIO_CODING_CODECS_ILBC_ILBC_H_

#include <stddef.h>
#include <stdint.h>

#include "ilbc_export.h"

/*
 * Solution to support multiple instances
 * Customer has to cast instance to proper type
 */

typedef struct iLBC_encinst_t_ IlbcEncoderInstance;

typedef struct iLBC_decinst_t_ IlbcDecoderInstance;

/*
 * Comfort noise constants
 */

#define ILBC_SPEECH 1
#define ILBC_CNG 2

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
 * WebRtcIlbcfix_XxxAssign(...)
 *
 * These functions assigns the encoder/decoder instance to the specified
 * memory location
 *
 * Input:
 *     - XXX_xxxinst       : Pointer to created instance that should be
 *                           assigned
 *     - ILBCXXX_inst_Addr : Pointer to the desired memory space
 *     - size              : The size that this structure occupies (in Word16)
 *
 * Return value             :  0 - Ok
 *                            -1 - Error
 */

ILBC_EXPORT
int16_t WebRtcIlbcfix_EncoderAssign(IlbcEncoderInstance** iLBC_encinst,
                                    int16_t* ILBCENC_inst_Addr,
                                    int16_t* size);
ILBC_EXPORT
int16_t WebRtcIlbcfix_DecoderAssign(IlbcDecoderInstance** iLBC_decinst,
                                    int16_t* ILBCDEC_inst_Addr,
                                    int16_t* size);

/****************************************************************************
 * WebRtcIlbcfix_XxxAssign(...)
 *
 * These functions create a instance to the specified structure
 *
 * Input:
 *      - XXX_inst        : Pointer to created instance that should be created
 *
 * Return value           :  0 - Ok
 *                          -1 - Error
 */

ILBC_EXPORT
int16_t WebRtcIlbcfix_EncoderCreate(IlbcEncoderInstance** iLBC_encinst);
ILBC_EXPORT
int16_t WebRtcIlbcfix_DecoderCreate(IlbcDecoderInstance** iLBC_decinst);

/****************************************************************************
 * WebRtcIlbcfix_XxxFree(...)
 *
 * These functions frees the dynamic memory of a specified instance
 *
 * Input:
 *      - XXX_inst          : Pointer to created instance that should be freed
 *
 * Return value             :  0 - Ok
 *                            -1 - Error
 */

ILBC_EXPORT
int16_t WebRtcIlbcfix_EncoderFree(IlbcEncoderInstance* iLBC_encinst);
ILBC_EXPORT
int16_t WebRtcIlbcfix_DecoderFree(IlbcDecoderInstance* iLBC_decinst);

/****************************************************************************
 * WebRtcIlbcfix_EncoderInit(...)
 *
 * This function initializes a iLBC instance
 *
 * Input:
 *      - iLBCenc_inst      : iLBC instance, i.e. the user that should receive
 *                            be initialized
 *      - frameLen          : The frame length of the codec 20/30 (ms)
 *
 * Return value             :  0 - Ok
 *                            -1 - Error
 */

ILBC_EXPORT
int16_t WebRtcIlbcfix_EncoderInit(IlbcEncoderInstance* iLBCenc_inst,
                                  int16_t frameLen);

/****************************************************************************
 * WebRtcIlbcfix_Encode(...)
 *
 * This function encodes one iLBC frame. Input speech length has be a
 * multiple of the frame length.
 *
 * Input:
 *      - iLBCenc_inst      : iLBC instance, i.e. the user that should encode
 *                            a package
 *      - speechIn          : Input speech vector
 *      - len               : Samples in speechIn (160, 240, 320 or 480)
 *
 * Output:
 *  - encoded               : The encoded data vector
 *
 * Return value             : >0 - Length (in bytes) of coded data
 *                            -1 - Error
 */

ILBC_EXPORT
int WebRtcIlbcfix_Encode(IlbcEncoderInstance* iLBCenc_inst,
                         const int16_t* speechIn,
                         size_t len,
                         uint8_t* encoded);

/****************************************************************************
 * WebRtcIlbcfix_DecoderInit(...)
 *
 * This function initializes a iLBC instance with either 20 or 30 ms frames
 * Alternatively the WebRtcIlbcfix_DecoderInit_XXms can be used. Then it's
 * not needed to specify the frame length with a variable.
 *
 * Input:
 *      - IlbcDecoderInstance : iLBC decoder instance
 *      - frameLen            : The frame length of the codec 20/30 (ms)
 *
 * Return value               :  0 - Ok
 *                              -1 - Error
 */

ILBC_EXPORT
int16_t WebRtcIlbcfix_DecoderInit(IlbcDecoderInstance* iLBCdec_inst,
                                  int16_t frameLen);
ILBC_EXPORT
void WebRtcIlbcfix_DecoderInit20Ms(IlbcDecoderInstance* iLBCdec_inst);
ILBC_EXPORT
void WebRtcIlbcfix_Decoderinit30Ms(IlbcDecoderInstance* iLBCdec_inst);

/****************************************************************************
 * WebRtcIlbcfix_Decode(...)
 *
 * This function decodes a packet with iLBC frame(s). Output speech length
 * will be a multiple of 160 or 240 samples ((160 or 240)*frames/packet).
 *
 * Input:
 *      - iLBCdec_inst      : iLBC instance, i.e. the user that should decode
 *                            a packet
 *      - encoded           : Encoded iLBC frame(s)
 *      - len               : Bytes in encoded vector
 *
 * Output:
 *      - decoded           : The decoded vector
 *      - speechType        : 1 normal, 2 CNG
 *
 * Return value             : >0 - Samples in decoded vector
 *                            -1 - Error
 */

ILBC_EXPORT
int WebRtcIlbcfix_Decode(IlbcDecoderInstance* iLBCdec_inst,
                         const uint8_t* encoded,
                         size_t len,
                         int16_t* decoded,
                         int16_t* speechType);
ILBC_EXPORT
int WebRtcIlbcfix_Decode20Ms(IlbcDecoderInstance* iLBCdec_inst,
                             const uint8_t* encoded,
                             size_t len,
                             int16_t* decoded,
                             int16_t* speechType);
ILBC_EXPORT
int WebRtcIlbcfix_Decode30Ms(IlbcDecoderInstance* iLBCdec_inst,
                             const uint8_t* encoded,
                             size_t len,
                             int16_t* decoded,
                             int16_t* speechType);

/****************************************************************************
 * WebRtcIlbcfix_DecodePlc(...)
 *
 * This function conducts PLC for iLBC frame(s). Output speech length
 * will be a multiple of 160 or 240 samples.
 *
 * Input:
 *      - iLBCdec_inst      : iLBC instance, i.e. the user that should perform
 *                            a PLC
 *      - noOfLostFrames    : Number of PLC frames to produce
 *
 * Output:
 *      - decoded           : The "decoded" vector
 *
 * Return value             : Samples in decoded PLC vector
 */

ILBC_EXPORT
size_t WebRtcIlbcfix_DecodePlc(IlbcDecoderInstance* iLBCdec_inst,
                               int16_t* decoded,
                               size_t noOfLostFrames);

/****************************************************************************
 * WebRtcIlbcfix_NetEqPlc(...)
 *
 * This function updates the decoder when a packet loss has occured, but it
 * does not produce any PLC data. Function can be used if another PLC method
 * is used (i.e NetEq).
 *
 * Input:
 *      - iLBCdec_inst      : iLBC instance that should be updated
 *      - noOfLostFrames    : Number of lost frames
 *
 * Output:
 *      - decoded           : The "decoded" vector (nothing in this case)
 *
 * Return value             : Samples in decoded PLC vector
 */

ILBC_EXPORT
size_t WebRtcIlbcfix_NetEqPlc(IlbcDecoderInstance* iLBCdec_inst,
                              int16_t* decoded,
                              size_t noOfLostFrames);

/****************************************************************************
 * WebRtcIlbcfix_version(...)
 *
 * This function returns the version number of iLBC
 *
 * Output:
 *      - version           : Version number of iLBC (maximum 20 char)
 */

ILBC_EXPORT
void WebRtcIlbcfix_version(char* version);


/****************************************************************************
 * Compatibility with the library code from RFC3951.
 *
 */

/* general codec settings */

#define FS 8000
#define BLOCKL_20MS 160
#define BLOCKL_30MS 240
#define BLOCKL_MAX 240
#define NSUB_20MS 4
#define NSUB_30MS 6
#define NSUB_MAX 6
#define NASUB_20MS 2
#define NASUB_30MS 4
#define NASUB_MAX 4
#define SUBL 40
#define STATE_LEN 80
#define STATE_SHORT_LEN_30MS 58
#define STATE_SHORT_LEN_20MS 57

/* LPC settings */

#define LPC_FILTERORDER 10
#define LPC_LOOKBACK 60
#define LPC_N_20MS 1
#define LPC_N_30MS 2
#define LPC_N_MAX 2
#define LPC_ASYMDIFF 20
#define LSF_NSPLIT 3
#define LSF_NUMBER_OF_STEPS 4
#define LPC_HALFORDER 5
#define COS_GRID_POINTS 60

/* enhancer */

#define ENH_BLOCKL 80 /* block length */
#define ENH_BLOCKL_HALF (ENH_BLOCKL / 2)
#define ENH_HL                                                         \
  3 /* 2*ENH_HL+1 is number blocks                                     \
                                                        in said second \
       sequence */
#define ENH_SLOP                    \
  2 /* max difference estimated and \
                                                       correct pitch period */
#define ENH_PLOCSL                                                          \
  8 /* pitch-estimates and                                                  \
                                                     pitch-locations buffer \
       length */
#define ENH_OVERHANG 2
#define ENH_UPS0 4 /* upsampling rate */
#define ENH_FL0 3  /* 2*FLO+1 is the length of each filter */
#define ENH_FLO_MULT2_PLUS1 7
#define ENH_VECTL (ENH_BLOCKL + 2 * ENH_FL0)
#define ENH_CORRDIM (2 * ENH_SLOP + 1)
#define ENH_NBLOCKS (BLOCKL / ENH_BLOCKL)
#define ENH_NBLOCKS_EXTRA 5
#define ENH_NBLOCKS_TOT 8 /* ENH_NBLOCKS+ENH_NBLOCKS_EXTRA */
#define ENH_BUFL (ENH_NBLOCKS_TOT) * ENH_BLOCKL
#define ENH_BUFL_FILTEROVERHEAD 3
#define ENH_A0 819                      /* Q14 */
#define ENH_A0_MINUS_A0A0DIV4 848256041 /* Q34 */
#define ENH_A0DIV2 26843546             /* Q30 */

/* type definition encoder instance */
typedef struct IlbcEncoder_ {
  /* flag for frame size mode */
  int16_t mode;

  /* basic parameters for different frame sizes */
  size_t blockl;
  size_t nsub;
  int16_t nasub;
  size_t no_of_bytes, no_of_words;
  int16_t lpc_n;
  size_t state_short_len;

  /* analysis filter state */
  int16_t anaMem[LPC_FILTERORDER];

  /* Fix-point old lsf parameters for interpolation */
  int16_t lsfold[LPC_FILTERORDER];
  int16_t lsfdeqold[LPC_FILTERORDER];

  /* signal buffer for LP analysis */
  int16_t lpc_buffer[LPC_LOOKBACK + BLOCKL_MAX];

  /* state of input HP filter */
  int16_t hpimemx[2];
  int16_t hpimemy[4];

#ifdef SPLIT_10MS
  int16_t weightdenumbuf[66];
  int16_t past_samples[160];
  uint16_t bytes[25];
  int16_t section;
  int16_t Nfor_flag;
  int16_t Nback_flag;
  int16_t start_pos;
  size_t diff;
#endif

} IlbcEncoder;

/* type definition decoder instance */
typedef struct IlbcDecoder_ {
  /* flag for frame size mode */
  int16_t mode;

  /* basic parameters for different frame sizes */
  size_t blockl;
  size_t nsub;
  int16_t nasub;
  size_t no_of_bytes, no_of_words;
  int16_t lpc_n;
  size_t state_short_len;

  /* synthesis filter state */
  int16_t syntMem[LPC_FILTERORDER];

  /* old LSF for interpolation */
  int16_t lsfdeqold[LPC_FILTERORDER];

  /* pitch lag estimated in enhancer and used in PLC */
  size_t last_lag;

  /* PLC state information */
  int consPLICount, prev_enh_pl;
  int16_t perSquare;

  int16_t prevScale, prevPLI;
  size_t prevLag;
  int16_t prevLpc[LPC_FILTERORDER + 1];
  int16_t prevResidual[NSUB_MAX * SUBL];
  int16_t seed;

  /* previous synthesis filter parameters */

  int16_t old_syntdenum[(LPC_FILTERORDER + 1) * NSUB_MAX];

  /* state of output HP filter */
  int16_t hpimemx[2];
  int16_t hpimemy[4];

  /* enhancer state information */
  int use_enhancer;
  int16_t enh_buf[ENH_BUFL + ENH_BUFL_FILTEROVERHEAD];
  size_t enh_period[ENH_NBLOCKS_TOT];

} IlbcDecoder;

/*----------------------------------------------------------------*
 *  Initiation of decoder instance.
 *---------------------------------------------------------------*/

ILBC_EXPORT
int WebRtcIlbcfix_InitDecode(/* (o) Number of decoded samples */
                             IlbcDecoder*
                                 iLBCdec_inst, /* (i/o) Decoder instance */
                             int16_t mode,     /* (i) frame size mode */
                             int use_enhancer  /* (i) 1 to use enhancer
                                                  0 to run without enhancer */
                             );

/*----------------------------------------------------------------*
 *  Initiation of encoder instance.
 *---------------------------------------------------------------*/

ILBC_EXPORT
int WebRtcIlbcfix_InitEncode(/* (o) Number of bytes encoded */
                             IlbcEncoder*
                                 iLBCenc_inst, /* (i/o) Encoder instance */
                             int16_t mode      /* (i) frame size mode */
                             );

/*----------------------------------------------------------------*
 *  main decoder function
 *---------------------------------------------------------------*/

// Returns 0 on success, -1 on error.
ILBC_EXPORT
int WebRtcIlbcfix_DecodeImpl(
    int16_t* decblock,         /* (o) decoded signal block */
    const uint16_t* bytes,     /* (i) encoded signal bits */
    IlbcDecoder* iLBCdec_inst, /* (i/o) the decoder state
                                           structure */
    int16_t mode /* (i) 0: bad packet, PLC,
                        1: normal */
    );

/*----------------------------------------------------------------*
 *  main encoder function
 *---------------------------------------------------------------*/

ILBC_EXPORT
void WebRtcIlbcfix_EncodeImpl(
    uint16_t* bytes,      /* (o) encoded data bits iLBC */
    const int16_t* block, /* (i) speech vector to encode */
    IlbcEncoder* iLBCenc_inst /* (i/o) the general encoder
                                           state */
    );

// Version macros
#define LIBILBC_VERSION_MAJOR 3
#define LIBILBC_VERSION_MINOR 0
#define LIBILBC_VERSION_PATCH 3

// Old type names:
typedef ILBC_DEPRECATED IlbcEncoderInstance iLBC_encinst_t;
typedef ILBC_DEPRECATED IlbcDecoderInstance iLBC_decinst_t;
typedef ILBC_DEPRECATED IlbcEncoder iLBC_Enc_Inst_t;
typedef ILBC_DEPRECATED IlbcDecoder iLBC_Dec_Inst_t;

#ifdef __cplusplus
}
#endif

#endif  // MODULES_AUDIO_CODING_CODECS_ILBC_ILBC_H_
