/*
 * FLUENDO S.A.
 * Copyright (C) <2005 - 2011>  <support@fluendo.com>
 * 
 * This Source Code is licensed under MIT license and the explanations attached
 * in MIT License Statements.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 * MIT license Statements for Fluendo's mp3 plug-in Source Code
 * ------------------------------------------------------------
 *
 * Fluendo's mp3 software Source Code (the "Source Code") is licensed under the 
 * MIT license provisions.
 *
 * The MIT license is an open source license that permits the User to operate and
 * use in many forms the Source Code, which would be governed under its
 * regulations. 
 *
 * The purpose of this note is to clarify the intellectual property rights granted 
 * over the Source Code by Fluendo, as well as other legal issues that concern
 * your use of it.
 *
 * MIT license contents and provisions
 * -----------------------------------
 *
 * The MIT license allows you to do the following things with the Source Code:
 *
 * - Copy and use the Source Code alone or jointly with other code for any
 *   purposes. 
 *   Copy of the Source Code is not limited and is royalty-free.
 *
 * - Merge the Source Code with other code for developing new applications with no 
 *   limits.
 *
 * - Modifying the Source Code for developing the plug-in or for implementing the 
 *   plug-in in other applications for any purposes. The MIT License does not 
 *   require you to share these modifications with anyone. 
 *
 * - Publish, distribute, sublicense and sell copies of the Source Code to third 
 *   parties.
 *
 * - Permit anyone to whom the Source Code is licensed to enjoy the rights above 
 *   subject to the MIT license provisions. 
 *
 * By licensing this Source Code under the MIT License, Fluendo is offering to the 
 * community the rights set out above without restriction and without any
 * obligation for the User of the Source Code to release his/her modifications
 * back to the community. Anyone operating with the Source Code released from
 * Fluendo must grant the same MIT license rights to the community, except for any
 * modifications operated on the Source Code which can be granted under a
 * different license (even a proprietary license).
 *
 * All these rights granted to the User for the Source Code hold a limitation
 * which is to include MIT permission notice and the following copyright notice:
 * "Copyright 2005 Fluendo, S.L. This Source Code is licensed under MIT license
 * and the explanations attached in MIT License Statements". These notices shall
 * be included in all copies of the Source Code or in substantial parts of the
 * Source Code which may be released separately or with modifications. 
 *
 * Patents over the plug-in and/or Source Code
 * -------------------------------------------
 *
 * The binaries that can be created by compiling this Source Code released by
 * Fluendo might be covered by patents in various parts of the world.  Fluendo
 * does not own or claim to own any patents on the techniques used in the code.
 * (Such patents are owned or claimed to be owned by Thompson Licensing, S.A. and
 * some other entities as the case may be). 
 *
 * Fluendo has got the relevant licenses to cover its own activities with the
 * Source Code but it is not authorized to sublicense nor to grant the rights
 * which it has acquired over the patents. In this sense, you can work and deal
 * freely with the Source Code under MIT provisions set out above, bearing in mind
 * that some activities might not be allowed under applicable patent regulations
 * and that Fluendo is not granting any rights in relation to such patents.
 *
 * The patent license granted to Fluendo only covers Fluendo's own Software and
 * Source Code activities. In any case, this software license does not allow you
 * to redistribute or copy complete, ready to use mp3 software decoder binaries
 * made from the Source Code as made available by Fluendo. You can of course
 * distribute binaries you make yourself under any terms allowed by the MIT
 * license and whatever necessary rights you have or have acquired according to
 * applicable patent regulations.
 *
 * As Fluendo can not assure that any of the activities you undertake do not
 * infringe any patents or other industrial or intellectual property rights,
 * Fluendo hereby disclaims any liability for any patent infringement that may be
 * claimed to you or to any other person from any legitimate right’s owner, as
 * stated in MIT license. So it is your responsibility to get information and to
 * acquire the necessary patent licenses to undertake your activities legally.
 */

//
// Modifications and bug fixes copyright 2018 High Fidelity, Inc.
// Now passes ISO/IEC 11172-4 "full accuracy" compliance testing.
//

#include "flump3dec.h"

#include <stdlib.h>
#include <math.h>

namespace flump3dec {

/***********************************************************************
*
*  Global Definitions
*
***********************************************************************/

#ifdef __GNUC__
#define G_LIKELY(expr) (__builtin_expect ((expr), 1))
#define G_UNLIKELY(expr) (__builtin_expect ((expr), 0))
#else
#define G_LIKELY
#define G_UNLIKELY
#endif

#ifdef __GNUC__
#define ATTR_UNUSED __attribute__ ((unused))
#else
#define ATTR_UNUSED
#endif

#if defined(_MSC_VER)
#define STATIC_INLINE static __inline
#elif defined(__GNUC__)
#define STATIC_INLINE static __inline__
#else
#define STATIC_INLINE static inline
#endif

#define __CACHE_LINE_BYTES 16
#define __CACHE_LINE_ALIGN(a) ((((gsize)a) + __CACHE_LINE_BYTES - 1) & ~(__CACHE_LINE_BYTES - 1))

#if defined(_MSC_VER)
#define __CACHE_LINE_DECL_ALIGN(x) __declspec (align(__CACHE_LINE_BYTES)) x
#elif defined(__GNUC__)
#define __CACHE_LINE_DECL_ALIGN(x) x __attribute__ ((aligned (__CACHE_LINE_BYTES)))
#else
#define __CACHE_LINE_DECL_ALIGN(x) x
#endif

#if defined(_MSC_VER)
#define BYTESWAP32(p) _byteswap_ulong(*(uint32_t *)(p))
#elif defined(__GNUC__)
#define BYTESWAP32(p) __builtin_bswap32(*(uint32_t *)(p))
#else
#define BYTESWAP32(p) (uint32_t)(((p)[0] << 24) | ((p)[1] << 16) | ((p)[2] << 8) | ((p)[3] << 0))
#endif

#define SYNC_WORD_LNGTH 11
#define HEADER_LNGTH 21
#define SYNC_WORD ((guint32)(0x7ff))

/* General Definitions */
#ifndef PI
#define         PI                      3.141592653589793115997963468544185161590576171875
#endif

#define         PI4                     PI/4
#define         PI64                    PI/64

/* Maximum samples per sub-band */
#define         SSLIMIT                 18
/* Maximum number of sub-bands */
#define         SBLIMIT                 32

#define         HAN_SIZE                512
#define         SCALE_BLOCK             12
#define         SCALE_RANGE             64
#define         SCALE                   32768
#define         CRC16_POLYNOMIAL        0x8005

/* MPEG Header Definitions - ID Bit Values */
#define         MPEG_VERSION_1          0x03
#define         MPEG_VERSION_2          0x02
#define         MPEG_VERSION_2_5        0x00

/* MPEG Header Definitions - Mode Values */
#define         MPG_MD_STEREO           0
#define         MPG_MD_JOINT_STEREO     1
#define         MPG_MD_DUAL_CHANNEL     2
#define         MPG_MD_MONO             3

/***********************************************************************
*
*  Global Type Definitions
*
***********************************************************************/

/* Structure for Reading Layer II Allocation Tables from File */
typedef struct
{
  unsigned int steps;
  unsigned int bits;
  unsigned int group;
  unsigned int quant;
} sb_alloc;

typedef sb_alloc al_table[SBLIMIT][16];

/* Layer III side information. */
typedef struct
{
  guint part2_3_length;
  guint big_values;
  guint global_gain;
  guint scalefac_compress;
  guint window_switching_flag;
  guint block_type;
  guint mixed_block_flag;
  guint table_select[3];
  guint subblock_gain[3];
  guint region0_count;
  guint region1_count;
  guint preflag;
  guint scalefac_scale;
  guint count1table_select;
} gr_info_t;

typedef struct
{
  guint main_data_begin;
  guint private_bits;
  guint scfsi[4][2];            /* [band?][channel] */
  gr_info_t gr[2][2];           /* [group][channel] */
} III_side_info_t;

/* Layer III scale factors. */
typedef struct
{
  int l[22];                    /* [cb] */
  int s[3][13];                 /* [window][cb] */
} III_scalefac_t[2];            /* [ch] */

/* top level structure for frame decoding */
typedef struct
{
  fr_header header;             /* raw header information */
  int actual_mode;              /* when writing IS, may forget if 0 chs */
  int stereo;                   /* 1 for mono, 2 for stereo */
  int jsbound;                  /* first band of joint stereo coding */
  int sblimit;                  /* total number of sub bands */
  const al_table *alloc;        /* bit allocation table for the frame */

  /* Synthesis workspace */
  __CACHE_LINE_DECL_ALIGN(float filter[64][32]);
  __CACHE_LINE_DECL_ALIGN(float synbuf[2][2 * HAN_SIZE]);
  gint bufOffset[2];

  /* scale data */
  guint scalefac_buffer[54];
} frame_params;

/* Huffman decoder bit buffer decls */
#define HDBB_BUFSIZE 4096

typedef struct
{
  guint avail;
  guint buf_byte_idx;
  guint buf_bit_idx;
#if ENABLE_OPT_BS
  guint remaining;
  guint32 accumulator;
#endif
  guint8 *buf;
} huffdec_bitbuf;

/* Huffman Decoder bit buffer functions */
static void h_setbuf (huffdec_bitbuf * bb, guint8 * buf, guint size);
static void h_reset (huffdec_bitbuf * bb);

#if ENABLE_OPT_BS
static inline guint32 h_get1bit (huffdec_bitbuf * bb);
static inline void h_flushbits (huffdec_bitbuf * bb, guint N);
#else
#define h_get1bit(bb) (h_getbits ((bb), 1))
#define h_flushbits(bb,bits) (h_getbits ((bb), (bits)))
#endif

/* read N bits from the bit stream */
static inline guint32 h_getbits (huffdec_bitbuf * bb, guint N);

static void h_rewindNbits (huffdec_bitbuf * bb, guint N);

/* Return the current bit stream position (in bits) */
#if ENABLE_OPT_BS
#define h_sstell(bb) ((bb->buf_byte_idx * 8) - bb->buf_bit_idx)
#else
#define h_sstell(bb) ((bb->buf_byte_idx * 8) + (BS_ACUM_SIZE - bb->buf_bit_idx))
#endif

#if ENABLE_OPT_BS
/* This optimizazion assumes that N will be lesser than 32 */
static inline guint32
h_getbits (huffdec_bitbuf * bb, guint N)
{
  guint32 val = 0;
  static const guint32 h_mask_table[] = {
    0x00000000,
    0x00000001, 0x00000003, 0x00000007, 0x0000000f,
    0x0000001f, 0x0000003f, 0x0000007f, 0x000000ff,
    0x000001ff, 0x000003ff, 0x000007ff, 0x00000fff,
    0x00001fff, 0x00003fff, 0x00007fff, 0x0000ffff,
    0x0001ffff, 0x0003ffff, 0x0007ffff, 0x000fffff,
    0x001fffff, 0x003fffff, 0x007fffff, 0x00ffffff,
    0x01ffffff, 0x03ffffff, 0x07ffffff, 0x0fffffff,
    0x1fffffff, 0x3fffffff, 0x7fffffff, 0xffffffff
  };

  if (N == 0)
    return 0;

  /* Most common case will be when accumulator has enough bits */
  if (G_LIKELY (N <= bb->buf_bit_idx)) {
    /* first reduce buf_bit_idx by the number of bits that are taken */
    bb->buf_bit_idx -= N;
    /* Displace to right and mask to extract the desired number of bits */
    val = (bb->accumulator >> bb->buf_bit_idx) & h_mask_table[N];
    return (val);
  }

  /* Next cases will be when there's not enough data on the accumulator
     and there's atleast 4 bytes in the */
  if (bb->remaining >= 4) {
    /* First take all remaining bits */
    if (bb->buf_bit_idx > 0)
      val = bb->accumulator & h_mask_table[bb->buf_bit_idx];
    /* calculate how many more bits are required */
    N -= bb->buf_bit_idx;
    /* reload the accumulator */
    bb->buf_bit_idx = 32 - N;   /* subtract the remaining required bits */
    bb->remaining -= 4;

    /* we need reverse the byte order */
    bb->accumulator = BYTESWAP32(bb->buf + bb->buf_byte_idx);
    bb->buf_byte_idx += 4;

    val <<= N;
    val |= (bb->accumulator >> bb->buf_bit_idx) & h_mask_table[N];
    return (val);
  }

  /* Next case when remains less that one word on the buffer */
  if (bb->remaining > 0) {
    /* First take all remaining bits */
    if (bb->buf_bit_idx > 0)
      val = bb->accumulator & h_mask_table[bb->buf_bit_idx];
    /* calculate how many more bits are required */
    N -= bb->buf_bit_idx;
    /* reload the accumulator */
    bb->buf_bit_idx = (bb->remaining * 8) - N;  /* subtract the remaining required bits */

    bb->accumulator = 0;
    /* load remaining bytes into the accumulator in the right order */
    for (; bb->remaining > 0; bb->remaining--) {
      bb->accumulator <<= 8;
      bb->accumulator |= (guint32) bb->buf[bb->buf_byte_idx++];
    }
    val <<= N; 
    val |= (bb->accumulator >> bb->buf_bit_idx) & h_mask_table[N];
    return (val);
  }

  return 0;
}

static inline guint32
h_get1bit (huffdec_bitbuf * bb)
{
  guint32 val = 0;

  /* Most common case will be when accumulator has enough bits */
  if (G_LIKELY (bb->buf_bit_idx > 0)) {
    /* first reduce buf_bit_idx by the number of bits that are taken */
    bb->buf_bit_idx--;
    /* Displace to right and mask to extract the desired number of bits */
    val = (bb->accumulator >> bb->buf_bit_idx) & 0x1;
    return (val);
  }

  /* Next cases will be when there's not enough data on the accumulator
     and there's atleast 4 bytes in the */
  if (bb->remaining >= 4) {
    /* reload the accumulator */
    bb->buf_bit_idx = 31;       /* subtract 1 bit */
    bb->remaining -= 4;

    /* we need reverse the byte order */
    bb->accumulator = BYTESWAP32(bb->buf + bb->buf_byte_idx);
    bb->buf_byte_idx += 4;

    val = (bb->accumulator >> bb->buf_bit_idx) & 0x1;
    return (val);
  }

  /* Next case when remains less that one word on the buffer */
  if (bb->remaining > 0) {
    /* reload the accumulator */
    bb->buf_bit_idx = (bb->remaining * 8) - 1;  /* subtract 1 bit  */

    bb->accumulator = 0;
    /* load remaining bytes into the accumulator in the right order */
    for (; bb->remaining > 0; bb->remaining--) {
      bb->accumulator <<= 8;
      bb->accumulator |= (guint32) bb->buf[bb->buf_byte_idx++];
    }

    val = (bb->accumulator >> bb->buf_bit_idx) & 0x1;
    return (val);
  }

  return 0;
}

static inline void
h_flushbits (huffdec_bitbuf * bb, guint N)
{
  guint bits;
  guint bytes;

  if (N < 32) {
    bits = N;
  } else {
    N -= bb->buf_bit_idx;
    bytes = N >> 3;
    bits = N & 0x7;
    bb->buf_byte_idx += bytes;
    bb->remaining -= bytes;
    bb->buf_bit_idx = 0;

    if (bb->remaining >= 4) {
      /* reload the accumulator */
      bb->buf_bit_idx = 32;       /* subtract 1 bit */
      bb->remaining -= 4;

      /* we need reverse the byte order */
      bb->accumulator = BYTESWAP32(bb->buf + bb->buf_byte_idx);
      bb->buf_byte_idx += 4;
    } else if (bb->remaining > 0) {
      /* reload the accumulator */
      bb->buf_bit_idx = bb->remaining * 8;

      bb->accumulator = 0;
      /* load remaining bytes into the accumulator in the right order */
      for (; bb->remaining > 0; bb->remaining--) {
        bb->accumulator <<= 8;
        bb->accumulator |= (guint32) bb->buf[bb->buf_byte_idx++];
      }
    }
  }

  if (bits)
    h_getbits (bb, bits);
}

#else
static inline guint32
h_getbits (huffdec_bitbuf * bb, guint N)
{
  guint32 val = 0;
  guint j = N;
  guint k, tmp;
  guint mask;

  while (j > 0) {
    if (!bb->buf_bit_idx) {
      bb->buf_bit_idx = 8;
      bb->buf_byte_idx++;
      if (bb->buf_byte_idx > bb->avail) {
        return 0;
      }
    }
    k = MIN (j, bb->buf_bit_idx);

    mask = (1 << (bb->buf_bit_idx)) - 1;
    tmp = bb->buf[bb->buf_byte_idx % HDBB_BUFSIZE] & mask;
    tmp = tmp >> (bb->buf_bit_idx - k);
    val |= tmp << (j - k);
    bb->buf_bit_idx -= k;
    j -= k;
  }
  return (val);
}
#endif

typedef struct mp3cimpl_info mp3cimpl_info;

struct mp3cimpl_info
{
  huffdec_bitbuf bb;            /* huffman decoder bit buffer */
  guint8 hb_buf[HDBB_BUFSIZE];  /* Huffman decoder work buffer */
  guint main_data_end;          /* Number of bytes in the bit reservoir at the 
                                 * end of the last frame */

  /* Hybrid */
  gfloat prevblck[2][SBLIMIT][SSLIMIT];

  /* scale data */
  guint scalefac_buffer[54];
};

typedef short PCM[2][SSLIMIT][SBLIMIT];
typedef unsigned int SAM[2][3][SBLIMIT];
typedef float FRA[2][3][SBLIMIT];

/* Size of the temp buffer for output samples, max 2 channels * 
 * sub-bands * samples-per-sub-band * 2 buffers
 */
#define SAMPLE_BUF_SIZE (4 * 2 * SBLIMIT * SSLIMIT)

struct mp3tl
{
  void * alloc_memory;
  gboolean need_sync;
  gboolean need_header;
  gboolean at_eos;
  gboolean lost_sync;

  /* Bit stream to read the data from */
  Bit_stream_struc *bs;

  /* Layer number we're decoding, 0 if we don't know yet */
  guint8 stream_layer;

  guint64 frame_num;

  /* Bits consumed from the stream so far */
  gint64 bits_used;

  /* Number of samples output so far */
  guint32 sample_frames;

  /* Total number of errors encountered */
  guint error_count;

  /* Sample size configure for. Always 16-bits, for now */
  guint sample_size;

  /* Frame decoding info */
  frame_params fr_ps;

  /* Number of granules in this frame (1 or 2) */
  guint n_granules;
  /* CRC value read from the mpeg data */
  guint old_crc;

  __CACHE_LINE_DECL_ALIGN(PCM pcm_sample);
  __CACHE_LINE_DECL_ALIGN(SAM sample);
  __CACHE_LINE_DECL_ALIGN(FRA fraction);

  /* Output samples circular buffer and read and write ptrs */
  gint16 *sample_buf;
  guint32 sample_w;

  char *reason;                 /* Reason why an error was returned, if known */

  mp3cimpl_info c_impl;

  /*Free format bitrate */
  guint32 free_bitrate;

  /*Used for one time calculation of free bitrate */
  gboolean free_first;
};

/* Sample rates table, index by MPEG version, samplerate index */
static const gint s_rates[4][4] = {
  {11025, 12000, 8000, 0},      /* MPEG_VERSION_2_5 */
  {0, 0, 0, 0},                 /* Invalid MPEG version */
  {22050, 24000, 16000, 0},     /* MPEG_VERSION_2 */
  {44100, 48000, 32000, 0}      /* MPEG_VERSION_1 */
};

/* MPEG version 1 bitrates. indexed by layer, bitrate index */
static const gint bitrates_v1[3][15] = {
  {0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448},
  {0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384},
  {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320}
};

/* MPEG version 2 (LSF) and 2.5 bitrates. indexed by layer, bitrate index */
static const gint bitrates_v2[3][15] = {
  {0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256},
  {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160},
  {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160}
};

typedef struct
{
  gint sub_bands;               /* Number of sub-bands in the frame */

  const al_table alloc;         /* The quantisation table itself */
} bitalloc_table;

/* There are 5 allocation tables available based on
 * the bitrate and sample rate */
static const bitalloc_table ba_tables[] = 
{
 {
  /* alloc_0 */
  27,
  {
    {
      { 0, 4, 0, 0 }, { 3, 5, 1, 0 }, { 7, 3, 3, 2 }, { 15, 4, 3, 4 }, 
      { 31, 5, 3, 5 }, { 63, 6, 3, 6 }, { 127, 7, 3, 7 }, { 255, 8, 3, 8 }, 
      { 511, 9, 3, 9 }, { 1023, 10, 3, 10 }, { 2047, 11, 3, 11 }, 
      { 4095, 12, 3, 12 }, { 8191, 13, 3, 13 }, { 16383, 14, 3, 14 }, 
      { 32767, 15, 3, 15 }, { 65535, 16, 3, 16 }, 
    }, {
      { 0, 4, 0, 0 }, { 3, 5, 1, 0 }, { 7, 3, 3, 2 }, { 15, 4, 3, 4 }, 
      { 31, 5, 3, 5 }, { 63, 6, 3, 6 }, { 127, 7, 3, 7 }, { 255, 8, 3, 8 }, 
      { 511, 9, 3, 9 }, { 1023, 10, 3, 10 }, { 2047, 11, 3, 11 }, 
      { 4095, 12, 3, 12 }, { 8191, 13, 3, 13 }, { 16383, 14, 3, 14 }, 
      { 32767, 15, 3, 15 }, { 65535, 16, 3, 16 }
    }, {
      { 0, 4, 0, 0 }, { 3, 5, 1, 0 }, { 7, 3, 3, 2 }, { 15, 4, 3, 4 }, 
      { 31, 5, 3, 5 }, { 63, 6, 3, 6 }, { 127, 7, 3, 7 }, { 255, 8, 3, 8 }, 
      { 511, 9, 3, 9 }, { 1023, 10, 3, 10 }, { 2047, 11, 3, 11 }, 
      { 4095, 12, 3, 12 }, { 8191, 13, 3, 13 }, { 16383, 14, 3, 14 },
      { 32767, 15, 3, 15 }, { 65535, 16, 3, 16 }
    }, {
      { 0, 4, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 7, 3, 3, 2 }, 
      { 9, 10, 1, 3 }, { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 63, 6, 3, 6 }, 
      { 127, 7, 3, 7 }, { 255, 8, 3, 8 }, { 511, 9, 3, 9 }, 
      { 1023, 10, 3, 10 }, { 2047, 11, 3, 11 }, { 4095, 12, 3, 12 }, 
      { 8191, 13, 3, 13 }, { 65535, 16, 3, 16 }
    }, {
      { 0, 4, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 7, 3, 3, 2 }, 
      { 9, 10, 1, 3 }, { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 63, 6, 3, 6 }, 
      { 127, 7, 3, 7 }, { 255, 8, 3, 8 }, { 511, 9, 3, 9 }, 
      { 1023, 10, 3, 10 }, { 2047, 11, 3, 11 }, { 4095, 12, 3, 12 }, 
      { 8191, 13, 3, 13 }, { 65535, 16, 3, 16 }
    }, {
      { 0, 4, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 7, 3, 3, 2 }, 
      { 9, 10, 1, 3 }, { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 63, 6, 3, 6 }, 
      { 127, 7, 3, 7 }, { 255, 8, 3, 8 }, { 511, 9, 3, 9 }, 
      { 1023, 10, 3, 10 }, { 2047, 11, 3, 11 }, { 4095, 12, 3, 12 }, 
      { 8191, 13, 3, 13 }, { 65535, 16, 3, 16 }
    }, {
      { 0, 4, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 7, 3, 3, 2 }, 
      { 9, 10, 1, 3 }, { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 63, 6, 3, 6 }, 
      { 127, 7, 3, 7 }, { 255, 8, 3, 8 }, { 511, 9, 3, 9 },
      { 1023, 10, 3, 10 }, { 2047, 11, 3, 11 }, { 4095, 12, 3, 12 }, 
      { 8191, 13, 3, 13 }, { 65535, 16, 3, 16 }
    }, {
      { 0, 4, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 7, 3, 3, 2 }, 
      { 9, 10, 1, 3 }, { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 63, 6, 3, 6 }, 
      { 127, 7, 3, 7 }, { 255, 8, 3, 8 }, { 511, 9, 3, 9 }, 
      { 1023, 10, 3, 10 }, { 2047, 11, 3, 11 }, { 4095, 12, 3, 12 }, 
      { 8191, 13, 3, 13 }, { 65535, 16, 3, 16 }
    }, {
      { 0, 4, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 7, 3, 3, 2 }, 
      { 9, 10, 1, 3 }, { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 63, 6, 3, 6 }, 
      { 127, 7, 3, 7 }, { 255, 8, 3, 8 }, { 511, 9, 3, 9 },
      { 1023, 10, 3, 10 }, { 2047, 11, 3, 11 }, { 4095, 12, 3, 12 },
      { 8191, 13, 3, 13 }, { 65535, 16, 3, 16 }      
    }, {
      { 0, 4, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 7, 3, 3, 2 }, 
      { 9, 10, 1, 3 }, { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 63, 6, 3, 6 }, 
      { 127, 7, 3, 7 }, { 255, 8, 3, 8 }, { 511, 9, 3, 9 }, 
      { 1023, 10, 3, 10 }, { 2047, 11, 3, 11 }, { 4095, 12, 3, 12 }, 
      { 8191, 13, 3, 13 }, { 65535, 16, 3, 16 }      
    }, {
      { 0, 4, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 7, 3, 3, 2 }, 
      { 9, 10, 1, 3 }, { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 63, 6, 3, 6 }, 
      { 127, 7, 3, 7 }, { 255, 8, 3, 8 }, { 511, 9, 3, 9 }, 
      { 1023, 10, 3, 10 }, { 2047, 11, 3, 11 }, { 4095, 12, 3, 12 },
      { 8191, 13, 3, 13 }, { 65535, 16, 3, 16 }
    }, {
      { 0, 3, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 7, 3, 3, 2 }, 
      { 9, 10, 1, 3 }, { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 65535, 16, 3, 16 }
    }, {
      { 0, 3, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 7, 3, 3, 2 }, 
      { 9, 10, 1, 3 }, { 15, 4, 3, 4 }, { 31, 5, 3, 5 },
      { 65535, 16, 3, 16 } 
    }, {
      { 0, 3, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 7, 3, 3, 2 }, 
      { 9, 10, 1, 3 }, { 15, 4, 3, 4 }, { 31, 5, 3, 5 },
      { 65535, 16, 3, 16 }
    }, {
      { 0, 3, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 7, 3, 3, 2 }, 
      { 9, 10, 1, 3 }, { 15, 4, 3, 4 }, { 31, 5, 3, 5 },
      { 65535, 16, 3, 16 }
    }, {
      { 0, 3, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 7, 3, 3, 2 }, 
      { 9, 10, 1, 3 }, { 15, 4, 3, 4 }, { 31, 5, 3, 5 },
      { 65535, 16, 3, 16 }
    }, {
      { 0, 3, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 7, 3, 3, 2 }, 
      { 9, 10, 1, 3 }, { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, 
      { 65535, 16, 3, 16 }
    }, {
      { 0, 3, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 7, 3, 3, 2 }, 
      { 9, 10, 1, 3 }, { 15, 4, 3, 4 }, { 31, 5, 3, 5 },
      { 65535, 16, 3, 16 }
    }, {
      { 0, 3, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 7, 3, 3, 2 }, 
      { 9, 10, 1, 3 }, { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 65535, 16, 3, 16 }
    }, {
      { 0, 3, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 7, 3, 3, 2 }, 
      { 9, 10, 1, 3 }, { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 65535, 16, 3, 16 }
    }, {
      { 0, 3, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 7, 3, 3, 2 }, 
      { 9, 10, 1, 3 }, { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 65535, 16, 3, 16 }
    }, {
      { 0, 3, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 7, 3, 3, 2 }, 
      { 9, 10, 1, 3 }, { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 65535, 16, 3, 16 }
    }, {
      { 0, 3, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 7, 3, 3, 2 }, 
      { 9, 10, 1, 3 }, { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 65535, 16, 3, 16 }
    }, {
      { 0, 2, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 65535, 16, 3, 16 }
    }, {
      { 0, 2, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 65535, 16, 3, 16 }
    }, {
      { 0, 2, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 65535, 16, 3, 16 }
    }, {
      { 0, 2, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 65535, 16, 3, 16 }
    }
  }
 }, 
 {
  /* alloc_1 */
  30,
  {
    {
      { 0, 4, 0, 0 }, { 3, 5, 1, 0 }, { 7, 3, 3, 2 }, { 15, 4, 3, 4 }, 
      { 31, 5, 3, 5 }, { 63, 6, 3, 6 }, { 127, 7, 3, 7 }, { 255, 8, 3, 8 }, 
      { 511, 9, 3, 9 }, { 1023, 10, 3, 10 }, { 2047, 11, 3, 11 }, 
      { 4095, 12, 3, 12 }, { 8191, 13, 3, 13 }, { 16383, 14, 3, 14 }, 
      { 32767, 15, 3, 15 }, { 65535, 16, 3, 16 }
    }, {
      { 0, 4, 0, 0 }, { 3, 5, 1, 0 }, { 7, 3, 3, 2 }, { 15, 4, 3, 4 }, 
      { 31, 5, 3, 5 }, { 63, 6, 3, 6 }, { 127, 7, 3, 7 }, { 255, 8, 3, 8 }, 
      { 511, 9, 3, 9 }, { 1023, 10, 3, 10 }, { 2047, 11, 3, 11 },
      { 4095, 12, 3, 12 }, { 8191, 13, 3, 13 }, { 16383, 14, 3, 14 },
      { 32767, 15, 3, 15 }, { 65535, 16, 3, 16 }
    }, {
      { 0, 4, 0, 0 }, { 3, 5, 1, 0 }, { 7, 3, 3, 2 }, { 15, 4, 3, 4 }, 
      { 31, 5, 3, 5 }, { 63, 6, 3, 6 }, { 127, 7, 3, 7 }, { 255, 8, 3, 8 }, 
      { 511, 9, 3, 9 }, { 1023, 10, 3, 10 }, { 2047, 11, 3, 11 },
      { 4095, 12, 3, 12 }, { 8191, 13, 3, 13 }, { 16383, 14, 3, 14 }, 
      { 32767, 15, 3, 15 }, { 65535, 16, 3, 16 }
    }, {
      { 0, 4, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 7, 3, 3, 2 }, 
      { 9, 10, 1, 3 }, { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 63, 6, 3, 6 }, 
      { 127, 7, 3, 7 }, { 255, 8, 3, 8 }, { 511, 9, 3, 9 }, 
      { 1023, 10, 3, 10 }, { 2047, 11, 3, 11 }, { 4095, 12, 3, 12 }, 
      { 8191, 13, 3, 13 }, { 65535, 16, 3, 16 }
    }, {
      { 0, 4, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 7, 3, 3, 2 }, 
      { 9, 10, 1, 3 }, { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 63, 6, 3, 6 }, 
      { 127, 7, 3, 7 }, { 255, 8, 3, 8 }, { 511, 9, 3, 9 },
      { 1023, 10, 3, 10 }, { 2047, 11, 3, 11 }, { 4095, 12, 3, 12 },
      { 8191, 13, 3, 13 }, { 65535, 16, 3, 16 }
    }, {
      { 0, 4, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 7, 3, 3, 2 }, 
      { 9, 10, 1, 3 }, { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 63, 6, 3, 6 }, 
      { 127, 7, 3, 7 }, { 255, 8, 3, 8 }, { 511, 9, 3, 9 },
      { 1023, 10, 3, 10 }, { 2047, 11, 3, 11 }, { 4095, 12, 3, 12 },
      { 8191, 13, 3, 13 }, { 65535, 16, 3, 16 }
    }, {
      { 0, 4, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 7, 3, 3, 2 }, 
      { 9, 10, 1, 3 }, { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 63, 6, 3, 6 }, 
      { 127, 7, 3, 7 }, { 255, 8, 3, 8 }, { 511, 9, 3, 9 },
      { 1023, 10, 3, 10 }, { 2047, 11, 3, 11 }, { 4095, 12, 3, 12 },
      { 8191, 13, 3, 13 }, { 65535, 16, 3, 16 }
    }, {
      { 0, 4, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 7, 3, 3, 2 }, 
      { 9, 10, 1, 3 }, { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 63, 6, 3, 6 }, 
      { 127, 7, 3, 7 }, { 255, 8, 3, 8 }, { 511, 9, 3, 9 },
      { 1023, 10, 3, 10 }, { 2047, 11, 3, 11 }, { 4095, 12, 3, 12 }, 
      { 8191, 13, 3, 13 }, { 65535, 16, 3, 16 }
    }, {
      { 0, 4, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 7, 3, 3, 2 }, 
      { 9, 10, 1, 3 }, { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 63, 6, 3, 6 }, 
      { 127, 7, 3, 7 }, { 255, 8, 3, 8 }, { 511, 9, 3, 9 }, 
      { 1023, 10, 3, 10 }, { 2047, 11, 3, 11 }, { 4095, 12, 3, 12 }, 
      { 8191, 13, 3, 13 }, { 65535, 16, 3, 16 }
    }, {
      { 0, 4, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 7, 3, 3, 2 }, 
      { 9, 10, 1, 3 }, { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 63, 6, 3, 6 }, 
      { 127, 7, 3, 7 }, { 255, 8, 3, 8 }, { 511, 9, 3, 9 }, 
      { 1023, 10, 3, 10 }, { 2047, 11, 3, 11 }, { 4095, 12, 3, 12 }, 
      { 8191, 13, 3, 13 }, { 65535, 16, 3, 16 }
    }, {
      { 0, 4, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 7, 3, 3, 2 }, 
      { 9, 10, 1, 3 }, { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 63, 6, 3, 6 }, 
      { 127, 7, 3, 7 }, { 255, 8, 3, 8 }, { 511, 9, 3, 9 }, 
      { 1023, 10, 3, 10 }, { 2047, 11, 3, 11 }, { 4095, 12, 3, 12 },
      { 8191, 13, 3, 13 }, { 65535, 16, 3, 16 }
    }, {
      { 0, 3, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 7, 3, 3, 2 }, 
      { 9, 10, 1, 3 }, { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 65535, 16, 3, 16 }
    }, {
      { 0, 3, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 7, 3, 3, 2 }, 
      { 9, 10, 1, 3 }, { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 65535, 16, 3, 16 }
    }, {
      { 0, 3, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 7, 3, 3, 2 }, 
      { 9, 10, 1, 3 }, { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 65535, 16, 3, 16 }
    }, {
      { 0, 3, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 7, 3, 3, 2 }, 
      { 9, 10, 1, 3 }, { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 65535, 16, 3, 16 }
    }, {
      { 0, 3, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 7, 3, 3, 2 }, 
      { 9, 10, 1, 3 }, { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 65535, 16, 3, 16 }
    }, {
      { 0, 3, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 7, 3, 3, 2 }, 
      { 9, 10, 1, 3 }, { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 65535, 16, 3, 16 }
    }, {
      { 0, 3, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 7, 3, 3, 2 }, 
      { 9, 10, 1, 3 }, { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 65535, 16, 3, 16 }
    }, {
      { 0, 3, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 7, 3, 3, 2 }, 
      { 9, 10, 1, 3 }, { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 65535, 16, 3, 16 }
    }, {
      { 0, 3, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 7, 3, 3, 2 }, 
      { 9, 10, 1, 3 }, { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 65535, 16, 3, 16 }
    }, {
      { 0, 3, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 7, 3, 3, 2 }, 
      { 9, 10, 1, 3 }, { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 65535, 16, 3, 16 } 
    }, {
      { 0, 3, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 7, 3, 3, 2 }, 
      { 9, 10, 1, 3 }, { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 65535, 16, 3, 16 } 
    }, {
      { 0, 3, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 7, 3, 3, 2 }, 
      { 9, 10, 1, 3 }, { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 65535, 16, 3, 16 } 
    }, {
      { 0, 2, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 65535, 16, 3, 16 } 
    }, {
      { 0, 2, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 65535, 16, 3, 16 } 
    }, {
      { 0, 2, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 65535, 16, 3, 16 } 
    }, {
      { 0, 2, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 65535, 16, 3, 16 } 
    }, {
      { 0, 2, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 65535, 16, 3, 16 } 
    }, {
      { 0, 2, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 65535, 16, 3, 16 } 
    }, {
      { 0, 2, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 65535, 16, 3, 16 } 
    }
  }
 },
 {
  /* alloc_2 */
  8,
  {
    {
      { 0, 4, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 9, 10, 1, 3 }, 
      { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 63, 6, 3, 6 }, { 127, 7, 3, 7 }, 
      { 255, 8, 3, 8 }, { 511, 9, 3, 9 }, { 1023, 10, 3, 10 },
      { 2047, 11, 3, 11 }, { 4095, 12, 3, 12 }, { 8191, 13, 3, 13 }, 
      { 16383, 14, 3, 14 }, { 32767, 15, 3, 15 }
    }, {
      { 0, 4, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 9, 10, 1, 3 }, 
      { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 63, 6, 3, 6 }, { 127, 7, 3, 7 }, 
      { 255, 8, 3, 8 }, { 511, 9, 3, 9 }, { 1023, 10, 3, 10 }, 
      { 2047, 11, 3, 11 }, { 4095, 12, 3, 12 }, { 8191, 13, 3, 13 }, 
      { 16383, 14, 3, 14 }, { 32767, 15, 3, 15 }
    }, {
      { 0, 3, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 9, 10, 1, 3 }, 
      { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 63, 6, 3, 6 }, { 127, 7, 3, 7 }
    }, {
      { 0, 3, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 9, 10, 1, 3 }, 
      { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 63, 6, 3, 6 }, { 127, 7, 3, 7 }
    }, {
      { 0, 3, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 9, 10, 1, 3 }, 
      { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 63, 6, 3, 6 }, { 127, 7, 3, 7 } 
    }, {
      { 0, 3, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 9, 10, 1, 3 }, 
      { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 63, 6, 3, 6 }, { 127, 7, 3, 7 }
    }, {
      { 0, 3, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 9, 10, 1, 3 }, 
      { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 63, 6, 3, 6 }, { 127, 7, 3, 7 }
    }, {
      { 0, 3, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 9, 10, 1, 3 }, 
      { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 63, 6, 3, 6 }, { 127, 7, 3, 7 }
    }
  }
 },
 {
  /* alloc_3 */
  12,
  {
    {
      { 0, 4, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 9, 10, 1, 3 },
      { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 63, 6, 3, 6 }, { 127, 7, 3, 7 },
      { 255, 8, 3, 8 }, { 511, 9, 3, 9 }, { 1023, 10, 3, 10 },
      { 2047, 11, 3, 11 }, { 4095, 12, 3, 12 }, { 8191, 13, 3, 13 },
      { 16383, 14, 3, 14 }, { 32767, 15, 3, 15 }
    }, {
      { 0, 4, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 9, 10, 1, 3 }, 
      { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 63, 6, 3, 6 }, { 127, 7, 3, 7 }, 
      { 255, 8, 3, 8 }, { 511, 9, 3, 9 }, { 1023, 10, 3, 10 },
      { 2047, 11, 3, 11 }, { 4095, 12, 3, 12 }, { 8191, 13, 3, 13 },
      { 16383, 14, 3, 14 }, { 32767, 15, 3, 15 }
    }, {
      { 0, 3, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 9, 10, 1, 3 }, 
      { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 63, 6, 3, 6 }, { 127, 7, 3, 7 }
    }, {
      { 0, 3, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 9, 10, 1, 3 }, 
      { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 63, 6, 3, 6 }, { 127, 7, 3, 7 }
    }, {
      { 0, 3, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 9, 10, 1, 3 }, 
      { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 63, 6, 3, 6 }, { 127, 7, 3, 7 }
    }, {
      { 0, 3, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 9, 10, 1, 3 }, 
      { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 63, 6, 3, 6 }, { 127, 7, 3, 7 }
    }, {
      { 0, 3, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 9, 10, 1, 3 }, 
      { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 63, 6, 3, 6 }, { 127, 7, 3, 7 }
    }, {
      { 0, 3, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 9, 10, 1, 3 }, 
      { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 63, 6, 3, 6 }, { 127, 7, 3, 7 }
    }, {
      { 0, 3, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 9, 10, 1, 3 }, 
      { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 63, 6, 3, 6 }, { 127, 7, 3, 7 }
    }, {
      { 0, 3, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 9, 10, 1, 3 }, 
      { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 63, 6, 3, 6 }, { 127, 7, 3, 7 }
    }, {
      { 0, 3, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 9, 10, 1, 3 }, 
      { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 63, 6, 3, 6 }, { 127, 7, 3, 7 }
    }, {
      { 0, 3, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 9, 10, 1, 3 }, 
      { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 63, 6, 3, 6 }, { 127, 7, 3, 7 }
    }
  }
},
{
  /* alloc_4 */
  30,
  {
    {
      { 0, 4, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 7, 3, 3, 2 }, 
      { 9, 10, 1, 3 }, { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 63, 6, 3, 6 }, 
      { 127, 7, 3, 7 }, { 255, 8, 3, 8 }, { 511, 9, 3, 9 },
      { 1023, 10, 3, 10 }, { 2047, 11, 3, 11 }, { 4095, 12, 3, 12 },
      { 8191, 13, 3, 13 }, { 16383, 14, 3, 14 } 
    }, {
      { 0, 4, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 7, 3, 3, 2 }, 
      { 9, 10, 1, 3 }, { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 63, 6, 3, 6 }, 
      { 127, 7, 3, 7 }, { 255, 8, 3, 8 }, { 511, 9, 3, 9 },
      { 1023, 10, 3, 10 }, { 2047, 11, 3, 11 }, { 4095, 12, 3, 12 },
      { 8191, 13, 3, 13 }, { 16383, 14, 3, 14 } 
    }, {
      { 0, 4, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 7, 3, 3, 2 }, 
      { 9, 10, 1, 3 }, { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 63, 6, 3, 6 }, 
      { 127, 7, 3, 7 }, { 255, 8, 3, 8 }, { 511, 9, 3, 9 },
      { 1023, 10, 3, 10 }, { 2047, 11, 3, 11 }, { 4095, 12, 3, 12 }, 
      { 8191, 13, 3, 13 }, { 16383, 14, 3, 14 } 
    }, {
      { 0, 4, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 7, 3, 3, 2 }, 
      { 9, 10, 1, 3 }, { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 63, 6, 3, 6 }, 
      { 127, 7, 3, 7 }, { 255, 8, 3, 8 }, { 511, 9, 3, 9 }, 
      { 1023, 10, 3, 10 }, { 2047, 11, 3, 11 }, { 4095, 12, 3, 12 }, 
      { 8191, 13, 3, 13 }, { 16383, 14, 3, 14 }
    }, {
      { 0, 3, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 9, 10, 1, 3 }, 
      { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 63, 6, 3, 6 }, { 127, 7, 3, 7 } 
    }, {
      { 0, 3, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 9, 10, 1, 3 }, 
      { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 63, 6, 3, 6 }, { 127, 7, 3, 7 } 
    }, {
      { 0, 3, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 9, 10, 1, 3 }, 
      { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 63, 6, 3, 6 }, { 127, 7, 3, 7 } 
    }, {
      { 0, 3, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 9, 10, 1, 3 }, 
      { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 63, 6, 3, 6 }, { 127, 7, 3, 7 } 
    }, {
      { 0, 3, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 9, 10, 1, 3 }, 
      { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 63, 6, 3, 6 }, { 127, 7, 3, 7 } 
    }, {
      { 0, 3, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 9, 10, 1, 3 }, 
      { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 63, 6, 3, 6 }, { 127, 7, 3, 7 } 
    }, {
      { 0, 3, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 9, 10, 1, 3 }, 
      { 15, 4, 3, 4 }, { 31, 5, 3, 5 }, { 63, 6, 3, 6 }, { 127, 7, 3, 7 }
    }, {
      { 0, 2, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 9, 10, 1, 3 } 
    }, {
      { 0, 2, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 9, 10, 1, 3 } 
    }, {
      { 0, 2, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 9, 10, 1, 3 } 
    }, {
      { 0, 2, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 9, 10, 1, 3 } 
    }, {
      { 0, 2, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 9, 10, 1, 3 } 
    }, {
      { 0, 2, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 9, 10, 1, 3 } 
    }, {
      { 0, 2, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 9, 10, 1, 3 } 
    }, {
      { 0, 2, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 9, 10, 1, 3 } 
    }, {
      { 0, 2, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 9, 10, 1, 3 } 
    }, {
      { 0, 2, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 9, 10, 1, 3 } 
    }, {
      { 0, 2, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 9, 10, 1, 3 } 
    }, {
      { 0, 2, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 9, 10, 1, 3 } 
    }, {
      { 0, 2, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 9, 10, 1, 3 } 
    }, {
      { 0, 2, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 9, 10, 1, 3 } 
    }, {
      { 0, 2, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 9, 10, 1, 3 } 
    }, {
      { 0, 2, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 9, 10, 1, 3 } 
    }, {
      { 0, 2, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 9, 10, 1, 3 } 
    }, {
      { 0, 2, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 9, 10, 1, 3 }
    }, {
      { 0, 2, 0, 0 }, { 3, 5, 1, 0 }, { 5, 7, 1, 1 }, { 9, 10, 1, 3 }
    }
  }
 }
};

struct huffcodetab
{
  guint treelen;                /* length of decoder tree */
  gint xlen;                    /* max. x-index+ */
  gint ylen;                    /* max. y-index+ */
  guint linbits;                /* number of linbits */
  gboolean quad_table;          /* TRUE for quadruple tables (32 & 33) */
  const guchar (*val)[2];       /* decoder tree data */
};

/* Arrays of the table constants */
static const guchar huffbits_1[7][2] = {
  {0x02, 0x01}, {0x00, 0x00}, {0x02, 0x01}, {0x00, 0x10},
  {0x02, 0x01}, {0x00, 0x01}, {0x00, 0x11}
};

static const guchar huffbits_2[17][2] = {
  {0x02, 0x01}, {0x00, 0x00}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0x10}, {0x00, 0x01}, {0x02, 0x01}, {0x00, 0x11},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x20}, {0x00, 0x21},
  {0x02, 0x01}, {0x00, 0x12}, {0x02, 0x01}, {0x00, 0x02},
  {0x00, 0x22}
};

static const guchar huffbits_3[17][2] = {
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x00}, {0x00, 0x01},
  {0x02, 0x01}, {0x00, 0x11}, {0x02, 0x01}, {0x00, 0x10},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x20}, {0x00, 0x21},
  {0x02, 0x01}, {0x00, 0x12}, {0x02, 0x01}, {0x00, 0x02},
  {0x00, 0x22}
};

static const guchar huffbits_5[31][2] = {
  {0x02, 0x01}, {0x00, 0x00}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0x10}, {0x00, 0x01}, {0x02, 0x01}, {0x00, 0x11},
  {0x08, 0x01}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x20},
  {0x00, 0x02}, {0x02, 0x01}, {0x00, 0x21}, {0x00, 0x12},
  {0x08, 0x01}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x22},
  {0x00, 0x30}, {0x02, 0x01}, {0x00, 0x03}, {0x00, 0x13},
  {0x02, 0x01}, {0x00, 0x31}, {0x02, 0x01}, {0x00, 0x32},
  {0x02, 0x01}, {0x00, 0x23}, {0x00, 0x33}
};

static const guchar huffbits_6[31][2] = {
  {0x06, 0x01}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x00},
  {0x00, 0x10}, {0x00, 0x11}, {0x06, 0x01}, {0x02, 0x01},
  {0x00, 0x01}, {0x02, 0x01}, {0x00, 0x20}, {0x00, 0x21},
  {0x06, 0x01}, {0x02, 0x01}, {0x00, 0x12}, {0x02, 0x01},
  {0x00, 0x02}, {0x00, 0x22}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0x31}, {0x00, 0x13}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0x30}, {0x00, 0x32}, {0x02, 0x01}, {0x00, 0x23},
  {0x02, 0x01}, {0x00, 0x03}, {0x00, 0x33}
};

static const guchar huffbits_7[71][2] = {
  {0x02, 0x01}, {0x00, 0x00}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0x10}, {0x00, 0x01}, {0x08, 0x01}, {0x02, 0x01},
  {0x00, 0x11}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x20},
  {0x00, 0x02}, {0x00, 0x21}, {0x12, 0x01}, {0x06, 0x01},
  {0x02, 0x01}, {0x00, 0x12}, {0x02, 0x01}, {0x00, 0x22},
  {0x00, 0x30}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x31},
  {0x00, 0x13}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x03},
  {0x00, 0x32}, {0x02, 0x01}, {0x00, 0x23}, {0x00, 0x04},
  {0x0a, 0x01}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x40},
  {0x00, 0x41}, {0x02, 0x01}, {0x00, 0x14}, {0x02, 0x01},
  {0x00, 0x42}, {0x00, 0x24}, {0x0c, 0x01}, {0x06, 0x01},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x33}, {0x00, 0x43},
  {0x00, 0x50}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x34},
  {0x00, 0x05}, {0x00, 0x51}, {0x06, 0x01}, {0x02, 0x01},
  {0x00, 0x15}, {0x02, 0x01}, {0x00, 0x52}, {0x00, 0x25},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x44}, {0x00, 0x35},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x53}, {0x00, 0x54},
  {0x02, 0x01}, {0x00, 0x45}, {0x00, 0x55}
};

static const guchar huffbits_8[71][2] = {
  {0x06, 0x01}, {0x02, 0x01}, {0x00, 0x00}, {0x02, 0x01},
  {0x00, 0x10}, {0x00, 0x01}, {0x02, 0x01}, {0x00, 0x11},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x21}, {0x00, 0x12},
  {0x0e, 0x01}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x20},
  {0x00, 0x02}, {0x02, 0x01}, {0x00, 0x22}, {0x04, 0x01},
  {0x02, 0x01}, {0x00, 0x30}, {0x00, 0x03}, {0x02, 0x01},
  {0x00, 0x31}, {0x00, 0x13}, {0x0e, 0x01}, {0x08, 0x01},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x32}, {0x00, 0x23},
  {0x02, 0x01}, {0x00, 0x40}, {0x00, 0x04}, {0x02, 0x01},
  {0x00, 0x41}, {0x02, 0x01}, {0x00, 0x14}, {0x00, 0x42},
  {0x0c, 0x01}, {0x06, 0x01}, {0x02, 0x01}, {0x00, 0x24},
  {0x02, 0x01}, {0x00, 0x33}, {0x00, 0x50}, {0x04, 0x01},
  {0x02, 0x01}, {0x00, 0x43}, {0x00, 0x34}, {0x00, 0x51},
  {0x06, 0x01}, {0x02, 0x01}, {0x00, 0x15}, {0x02, 0x01},
  {0x00, 0x05}, {0x00, 0x52}, {0x06, 0x01}, {0x02, 0x01},
  {0x00, 0x25}, {0x02, 0x01}, {0x00, 0x44}, {0x00, 0x35},
  {0x02, 0x01}, {0x00, 0x53}, {0x02, 0x01}, {0x00, 0x45},
  {0x02, 0x01}, {0x00, 0x54}, {0x00, 0x55}
};

static const guchar huffbits_9[71][2] = {
  {0x08, 0x01}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x00},
  {0x00, 0x10}, {0x02, 0x01}, {0x00, 0x01}, {0x00, 0x11},
  {0x0a, 0x01}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x20},
  {0x00, 0x21}, {0x02, 0x01}, {0x00, 0x12}, {0x02, 0x01},
  {0x00, 0x02}, {0x00, 0x22}, {0x0c, 0x01}, {0x06, 0x01},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x30}, {0x00, 0x03},
  {0x00, 0x31}, {0x02, 0x01}, {0x00, 0x13}, {0x02, 0x01},
  {0x00, 0x32}, {0x00, 0x23}, {0x0c, 0x01}, {0x04, 0x01},
  {0x02, 0x01}, {0x00, 0x41}, {0x00, 0x14}, {0x04, 0x01},
  {0x02, 0x01}, {0x00, 0x40}, {0x00, 0x33}, {0x02, 0x01},
  {0x00, 0x42}, {0x00, 0x24}, {0x0a, 0x01}, {0x06, 0x01},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x04}, {0x00, 0x50},
  {0x00, 0x43}, {0x02, 0x01}, {0x00, 0x34}, {0x00, 0x51},
  {0x08, 0x01}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x15},
  {0x00, 0x52}, {0x02, 0x01}, {0x00, 0x25}, {0x00, 0x44},
  {0x06, 0x01}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x05},
  {0x00, 0x54}, {0x00, 0x53}, {0x02, 0x01}, {0x00, 0x35},
  {0x02, 0x01}, {0x00, 0x45}, {0x00, 0x55}
};

static const guchar huffbits_10[127][2] = {
  {0x02, 0x01}, {0x00, 0x00}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0x10}, {0x00, 0x01}, {0x0a, 0x01}, {0x02, 0x01},
  {0x00, 0x11}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x20},
  {0x00, 0x02}, {0x02, 0x01}, {0x00, 0x21}, {0x00, 0x12},
  {0x1c, 0x01}, {0x08, 0x01}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0x22}, {0x00, 0x30}, {0x02, 0x01}, {0x00, 0x31},
  {0x00, 0x13}, {0x08, 0x01}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0x03}, {0x00, 0x32}, {0x02, 0x01}, {0x00, 0x23},
  {0x00, 0x40}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x41},
  {0x00, 0x14}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x04},
  {0x00, 0x33}, {0x02, 0x01}, {0x00, 0x42}, {0x00, 0x24},
  {0x1c, 0x01}, {0x0a, 0x01}, {0x06, 0x01}, {0x04, 0x01},
  {0x02, 0x01}, {0x00, 0x50}, {0x00, 0x05}, {0x00, 0x60},
  {0x02, 0x01}, {0x00, 0x61}, {0x00, 0x16}, {0x0c, 0x01},
  {0x06, 0x01}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x43},
  {0x00, 0x34}, {0x00, 0x51}, {0x02, 0x01}, {0x00, 0x15},
  {0x02, 0x01}, {0x00, 0x52}, {0x00, 0x25}, {0x04, 0x01},
  {0x02, 0x01}, {0x00, 0x26}, {0x00, 0x36}, {0x00, 0x71},
  {0x14, 0x01}, {0x08, 0x01}, {0x02, 0x01}, {0x00, 0x17},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x44}, {0x00, 0x53},
  {0x00, 0x06}, {0x06, 0x01}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0x35}, {0x00, 0x45}, {0x00, 0x62}, {0x02, 0x01},
  {0x00, 0x70}, {0x02, 0x01}, {0x00, 0x07}, {0x00, 0x64},
  {0x0e, 0x01}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x72},
  {0x00, 0x27}, {0x06, 0x01}, {0x02, 0x01}, {0x00, 0x63},
  {0x02, 0x01}, {0x00, 0x54}, {0x00, 0x55}, {0x02, 0x01},
  {0x00, 0x46}, {0x00, 0x73}, {0x08, 0x01}, {0x04, 0x01},
  {0x02, 0x01}, {0x00, 0x37}, {0x00, 0x65}, {0x02, 0x01},
  {0x00, 0x56}, {0x00, 0x74}, {0x06, 0x01}, {0x02, 0x01},
  {0x00, 0x47}, {0x02, 0x01}, {0x00, 0x66}, {0x00, 0x75},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x57}, {0x00, 0x76},
  {0x02, 0x01}, {0x00, 0x67}, {0x00, 0x77}
};

static const guchar huffbits_11[127][2] = {
  {0x06, 0x01}, {0x02, 0x01}, {0x00, 0x00}, {0x02, 0x01},
  {0x00, 0x10}, {0x00, 0x01}, {0x08, 0x01}, {0x02, 0x01},
  {0x00, 0x11}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x20},
  {0x00, 0x02}, {0x00, 0x12}, {0x18, 0x01}, {0x08, 0x01},
  {0x02, 0x01}, {0x00, 0x21}, {0x02, 0x01}, {0x00, 0x22},
  {0x02, 0x01}, {0x00, 0x30}, {0x00, 0x03}, {0x04, 0x01},
  {0x02, 0x01}, {0x00, 0x31}, {0x00, 0x13}, {0x04, 0x01},
  {0x02, 0x01}, {0x00, 0x32}, {0x00, 0x23}, {0x04, 0x01},
  {0x02, 0x01}, {0x00, 0x40}, {0x00, 0x04}, {0x02, 0x01},
  {0x00, 0x41}, {0x00, 0x14}, {0x1e, 0x01}, {0x10, 0x01},
  {0x0a, 0x01}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x42},
  {0x00, 0x24}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x33},
  {0x00, 0x43}, {0x00, 0x50}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0x34}, {0x00, 0x51}, {0x00, 0x61}, {0x06, 0x01},
  {0x02, 0x01}, {0x00, 0x16}, {0x02, 0x01}, {0x00, 0x06},
  {0x00, 0x26}, {0x02, 0x01}, {0x00, 0x62}, {0x02, 0x01},
  {0x00, 0x15}, {0x02, 0x01}, {0x00, 0x05}, {0x00, 0x52},
  {0x10, 0x01}, {0x0a, 0x01}, {0x06, 0x01}, {0x04, 0x01},
  {0x02, 0x01}, {0x00, 0x25}, {0x00, 0x44}, {0x00, 0x60},
  {0x02, 0x01}, {0x00, 0x63}, {0x00, 0x36}, {0x04, 0x01},
  {0x02, 0x01}, {0x00, 0x70}, {0x00, 0x17}, {0x00, 0x71},
  {0x10, 0x01}, {0x06, 0x01}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0x07}, {0x00, 0x64}, {0x00, 0x72}, {0x02, 0x01},
  {0x00, 0x27}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x53},
  {0x00, 0x35}, {0x02, 0x01}, {0x00, 0x54}, {0x00, 0x45},
  {0x0a, 0x01}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x46},
  {0x00, 0x73}, {0x02, 0x01}, {0x00, 0x37}, {0x02, 0x01},
  {0x00, 0x65}, {0x00, 0x56}, {0x0a, 0x01}, {0x06, 0x01},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x55}, {0x00, 0x57},
  {0x00, 0x74}, {0x02, 0x01}, {0x00, 0x47}, {0x00, 0x66},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x75}, {0x00, 0x76},
  {0x02, 0x01}, {0x00, 0x67}, {0x00, 0x77}
};

static const guchar huffbits_12[127][2] = {
  {0x0c, 0x01}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x10},
  {0x00, 0x01}, {0x02, 0x01}, {0x00, 0x11}, {0x02, 0x01},
  {0x00, 0x00}, {0x02, 0x01}, {0x00, 0x20}, {0x00, 0x02},
  {0x10, 0x01}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x21},
  {0x00, 0x12}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x22},
  {0x00, 0x31}, {0x02, 0x01}, {0x00, 0x13}, {0x02, 0x01},
  {0x00, 0x30}, {0x02, 0x01}, {0x00, 0x03}, {0x00, 0x40},
  {0x1a, 0x01}, {0x08, 0x01}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0x32}, {0x00, 0x23}, {0x02, 0x01}, {0x00, 0x41},
  {0x00, 0x33}, {0x0a, 0x01}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0x14}, {0x00, 0x42}, {0x02, 0x01}, {0x00, 0x24},
  {0x02, 0x01}, {0x00, 0x04}, {0x00, 0x50}, {0x04, 0x01},
  {0x02, 0x01}, {0x00, 0x43}, {0x00, 0x34}, {0x02, 0x01},
  {0x00, 0x51}, {0x00, 0x15}, {0x1c, 0x01}, {0x0e, 0x01},
  {0x08, 0x01}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x52},
  {0x00, 0x25}, {0x02, 0x01}, {0x00, 0x53}, {0x00, 0x35},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x60}, {0x00, 0x16},
  {0x00, 0x61}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x62},
  {0x00, 0x26}, {0x06, 0x01}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0x05}, {0x00, 0x06}, {0x00, 0x44}, {0x02, 0x01},
  {0x00, 0x54}, {0x00, 0x45}, {0x12, 0x01}, {0x0a, 0x01},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x63}, {0x00, 0x36},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x70}, {0x00, 0x07},
  {0x00, 0x71}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x17},
  {0x00, 0x64}, {0x02, 0x01}, {0x00, 0x46}, {0x00, 0x72},
  {0x0a, 0x01}, {0x06, 0x01}, {0x02, 0x01}, {0x00, 0x27},
  {0x02, 0x01}, {0x00, 0x55}, {0x00, 0x73}, {0x02, 0x01},
  {0x00, 0x37}, {0x00, 0x56}, {0x08, 0x01}, {0x04, 0x01},
  {0x02, 0x01}, {0x00, 0x65}, {0x00, 0x74}, {0x02, 0x01},
  {0x00, 0x47}, {0x00, 0x66}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0x75}, {0x00, 0x57}, {0x02, 0x01}, {0x00, 0x76},
  {0x02, 0x01}, {0x00, 0x67}, {0x00, 0x77}
};

static const guchar huffbits_13[511][2] = {
  {0x02, 0x01}, {0x00, 0x00}, {0x06, 0x01}, {0x02, 0x01},
  {0x00, 0x10}, {0x02, 0x01}, {0x00, 0x01}, {0x00, 0x11},
  {0x1c, 0x01}, {0x08, 0x01}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0x20}, {0x00, 0x02}, {0x02, 0x01}, {0x00, 0x21},
  {0x00, 0x12}, {0x08, 0x01}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0x22}, {0x00, 0x30}, {0x02, 0x01}, {0x00, 0x03},
  {0x00, 0x31}, {0x06, 0x01}, {0x02, 0x01}, {0x00, 0x13},
  {0x02, 0x01}, {0x00, 0x32}, {0x00, 0x23}, {0x04, 0x01},
  {0x02, 0x01}, {0x00, 0x40}, {0x00, 0x04}, {0x00, 0x41},
  {0x46, 0x01}, {0x1c, 0x01}, {0x0e, 0x01}, {0x06, 0x01},
  {0x02, 0x01}, {0x00, 0x14}, {0x02, 0x01}, {0x00, 0x33},
  {0x00, 0x42}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x24},
  {0x00, 0x50}, {0x02, 0x01}, {0x00, 0x43}, {0x00, 0x34},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x51}, {0x00, 0x15},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x05}, {0x00, 0x52},
  {0x02, 0x01}, {0x00, 0x25}, {0x02, 0x01}, {0x00, 0x44},
  {0x00, 0x53}, {0x0e, 0x01}, {0x08, 0x01}, {0x04, 0x01},
  {0x02, 0x01}, {0x00, 0x60}, {0x00, 0x06}, {0x02, 0x01},
  {0x00, 0x61}, {0x00, 0x16}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0x80}, {0x00, 0x08}, {0x00, 0x81}, {0x10, 0x01},
  {0x08, 0x01}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x35},
  {0x00, 0x62}, {0x02, 0x01}, {0x00, 0x26}, {0x00, 0x54},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x45}, {0x00, 0x63},
  {0x02, 0x01}, {0x00, 0x36}, {0x00, 0x70}, {0x06, 0x01},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x07}, {0x00, 0x55},
  {0x00, 0x71}, {0x02, 0x01}, {0x00, 0x17}, {0x02, 0x01},
  {0x00, 0x27}, {0x00, 0x37}, {0x48, 0x01}, {0x18, 0x01},
  {0x0c, 0x01}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x18},
  {0x00, 0x82}, {0x02, 0x01}, {0x00, 0x28}, {0x04, 0x01},
  {0x02, 0x01}, {0x00, 0x64}, {0x00, 0x46}, {0x00, 0x72},
  {0x08, 0x01}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x84},
  {0x00, 0x48}, {0x02, 0x01}, {0x00, 0x90}, {0x00, 0x09},
  {0x02, 0x01}, {0x00, 0x91}, {0x00, 0x19}, {0x18, 0x01},
  {0x0e, 0x01}, {0x08, 0x01}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0x73}, {0x00, 0x65}, {0x02, 0x01}, {0x00, 0x56},
  {0x00, 0x74}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x47},
  {0x00, 0x66}, {0x00, 0x83}, {0x06, 0x01}, {0x02, 0x01},
  {0x00, 0x38}, {0x02, 0x01}, {0x00, 0x75}, {0x00, 0x57},
  {0x02, 0x01}, {0x00, 0x92}, {0x00, 0x29}, {0x0e, 0x01},
  {0x08, 0x01}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x67},
  {0x00, 0x85}, {0x02, 0x01}, {0x00, 0x58}, {0x00, 0x39},
  {0x02, 0x01}, {0x00, 0x93}, {0x02, 0x01}, {0x00, 0x49},
  {0x00, 0x86}, {0x06, 0x01}, {0x02, 0x01}, {0x00, 0xa0},
  {0x02, 0x01}, {0x00, 0x68}, {0x00, 0x0a}, {0x02, 0x01},
  {0x00, 0xa1}, {0x00, 0x1a}, {0x44, 0x01}, {0x18, 0x01},
  {0x0c, 0x01}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0xa2},
  {0x00, 0x2a}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x95},
  {0x00, 0x59}, {0x02, 0x01}, {0x00, 0xa3}, {0x00, 0x3a},
  {0x08, 0x01}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x4a},
  {0x00, 0x96}, {0x02, 0x01}, {0x00, 0xb0}, {0x00, 0x0b},
  {0x02, 0x01}, {0x00, 0xb1}, {0x00, 0x1b}, {0x14, 0x01},
  {0x08, 0x01}, {0x02, 0x01}, {0x00, 0xb2}, {0x04, 0x01},
  {0x02, 0x01}, {0x00, 0x76}, {0x00, 0x77}, {0x00, 0x94},
  {0x06, 0x01}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x87},
  {0x00, 0x78}, {0x00, 0xa4}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0x69}, {0x00, 0xa5}, {0x00, 0x2b}, {0x0c, 0x01},
  {0x06, 0x01}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x5a},
  {0x00, 0x88}, {0x00, 0xb3}, {0x02, 0x01}, {0x00, 0x3b},
  {0x02, 0x01}, {0x00, 0x79}, {0x00, 0xa6}, {0x06, 0x01},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x6a}, {0x00, 0xb4},
  {0x00, 0xc0}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x0c},
  {0x00, 0x98}, {0x00, 0xc1}, {0x3c, 0x01}, {0x16, 0x01},
  {0x0a, 0x01}, {0x06, 0x01}, {0x02, 0x01}, {0x00, 0x1c},
  {0x02, 0x01}, {0x00, 0x89}, {0x00, 0xb5}, {0x02, 0x01},
  {0x00, 0x5b}, {0x00, 0xc2}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0x2c}, {0x00, 0x3c}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0xb6}, {0x00, 0x6b}, {0x02, 0x01}, {0x00, 0xc4},
  {0x00, 0x4c}, {0x10, 0x01}, {0x08, 0x01}, {0x04, 0x01},
  {0x02, 0x01}, {0x00, 0xa8}, {0x00, 0x8a}, {0x02, 0x01},
  {0x00, 0xd0}, {0x00, 0x0d}, {0x02, 0x01}, {0x00, 0xd1},
  {0x02, 0x01}, {0x00, 0x4b}, {0x02, 0x01}, {0x00, 0x97},
  {0x00, 0xa7}, {0x0c, 0x01}, {0x06, 0x01}, {0x02, 0x01},
  {0x00, 0xc3}, {0x02, 0x01}, {0x00, 0x7a}, {0x00, 0x99},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0xc5}, {0x00, 0x5c},
  {0x00, 0xb7}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x1d},
  {0x00, 0xd2}, {0x02, 0x01}, {0x00, 0x2d}, {0x02, 0x01},
  {0x00, 0x7b}, {0x00, 0xd3}, {0x34, 0x01}, {0x1c, 0x01},
  {0x0c, 0x01}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x3d},
  {0x00, 0xc6}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x6c},
  {0x00, 0xa9}, {0x02, 0x01}, {0x00, 0x9a}, {0x00, 0xd4},
  {0x08, 0x01}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0xb8},
  {0x00, 0x8b}, {0x02, 0x01}, {0x00, 0x4d}, {0x00, 0xc7},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x7c}, {0x00, 0xd5},
  {0x02, 0x01}, {0x00, 0x5d}, {0x00, 0xe0}, {0x0a, 0x01},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0xe1}, {0x00, 0x1e},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x0e}, {0x00, 0x2e},
  {0x00, 0xe2}, {0x08, 0x01}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0xe3}, {0x00, 0x6d}, {0x02, 0x01}, {0x00, 0x8c},
  {0x00, 0xe4}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0xe5},
  {0x00, 0xba}, {0x00, 0xf0}, {0x26, 0x01}, {0x10, 0x01},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0xf1}, {0x00, 0x1f},
  {0x06, 0x01}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0xaa},
  {0x00, 0x9b}, {0x00, 0xb9}, {0x02, 0x01}, {0x00, 0x3e},
  {0x02, 0x01}, {0x00, 0xd6}, {0x00, 0xc8}, {0x0c, 0x01},
  {0x06, 0x01}, {0x02, 0x01}, {0x00, 0x4e}, {0x02, 0x01},
  {0x00, 0xd7}, {0x00, 0x7d}, {0x02, 0x01}, {0x00, 0xab},
  {0x02, 0x01}, {0x00, 0x5e}, {0x00, 0xc9}, {0x06, 0x01},
  {0x02, 0x01}, {0x00, 0x0f}, {0x02, 0x01}, {0x00, 0x9c},
  {0x00, 0x6e}, {0x02, 0x01}, {0x00, 0xf2}, {0x00, 0x2f},
  {0x20, 0x01}, {0x10, 0x01}, {0x06, 0x01}, {0x04, 0x01},
  {0x02, 0x01}, {0x00, 0xd8}, {0x00, 0x8d}, {0x00, 0x3f},
  {0x06, 0x01}, {0x02, 0x01}, {0x00, 0xf3}, {0x02, 0x01},
  {0x00, 0xe6}, {0x00, 0xca}, {0x02, 0x01}, {0x00, 0xf4},
  {0x00, 0x4f}, {0x08, 0x01}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0xbb}, {0x00, 0xac}, {0x02, 0x01}, {0x00, 0xe7},
  {0x00, 0xf5}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0xd9},
  {0x00, 0x9d}, {0x02, 0x01}, {0x00, 0x5f}, {0x00, 0xe8},
  {0x1e, 0x01}, {0x0c, 0x01}, {0x06, 0x01}, {0x02, 0x01},
  {0x00, 0x6f}, {0x02, 0x01}, {0x00, 0xf6}, {0x00, 0xcb},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0xbc}, {0x00, 0xad},
  {0x00, 0xda}, {0x08, 0x01}, {0x02, 0x01}, {0x00, 0xf7},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x7e}, {0x00, 0x7f},
  {0x00, 0x8e}, {0x06, 0x01}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0x9e}, {0x00, 0xae}, {0x00, 0xcc}, {0x02, 0x01},
  {0x00, 0xf8}, {0x00, 0x8f}, {0x12, 0x01}, {0x08, 0x01},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0xdb}, {0x00, 0xbd},
  {0x02, 0x01}, {0x00, 0xea}, {0x00, 0xf9}, {0x04, 0x01},
  {0x02, 0x01}, {0x00, 0x9f}, {0x00, 0xeb}, {0x02, 0x01},
  {0x00, 0xbe}, {0x02, 0x01}, {0x00, 0xcd}, {0x00, 0xfa},
  {0x0e, 0x01}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0xdd},
  {0x00, 0xec}, {0x06, 0x01}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0xe9}, {0x00, 0xaf}, {0x00, 0xdc}, {0x02, 0x01},
  {0x00, 0xce}, {0x00, 0xfb}, {0x08, 0x01}, {0x04, 0x01},
  {0x02, 0x01}, {0x00, 0xbf}, {0x00, 0xde}, {0x02, 0x01},
  {0x00, 0xcf}, {0x00, 0xee}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0xdf}, {0x00, 0xef}, {0x02, 0x01}, {0x00, 0xff},
  {0x02, 0x01}, {0x00, 0xed}, {0x02, 0x01}, {0x00, 0xfd},
  {0x02, 0x01}, {0x00, 0xfc}, {0x00, 0xfe}
};

static const guchar huffbits_15[511][2] = {
  {0x10, 0x01}, {0x06, 0x01}, {0x02, 0x01}, {0x00, 0x00},
  {0x02, 0x01}, {0x00, 0x10}, {0x00, 0x01}, {0x02, 0x01},
  {0x00, 0x11}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x20},
  {0x00, 0x02}, {0x02, 0x01}, {0x00, 0x21}, {0x00, 0x12},
  {0x32, 0x01}, {0x10, 0x01}, {0x06, 0x01}, {0x02, 0x01},
  {0x00, 0x22}, {0x02, 0x01}, {0x00, 0x30}, {0x00, 0x31},
  {0x06, 0x01}, {0x02, 0x01}, {0x00, 0x13}, {0x02, 0x01},
  {0x00, 0x03}, {0x00, 0x40}, {0x02, 0x01}, {0x00, 0x32},
  {0x00, 0x23}, {0x0e, 0x01}, {0x06, 0x01}, {0x04, 0x01},
  {0x02, 0x01}, {0x00, 0x04}, {0x00, 0x14}, {0x00, 0x41},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x33}, {0x00, 0x42},
  {0x02, 0x01}, {0x00, 0x24}, {0x00, 0x43}, {0x0a, 0x01},
  {0x06, 0x01}, {0x02, 0x01}, {0x00, 0x34}, {0x02, 0x01},
  {0x00, 0x50}, {0x00, 0x05}, {0x02, 0x01}, {0x00, 0x51},
  {0x00, 0x15}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x52},
  {0x00, 0x25}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x44},
  {0x00, 0x53}, {0x00, 0x61}, {0x5a, 0x01}, {0x24, 0x01},
  {0x12, 0x01}, {0x0a, 0x01}, {0x06, 0x01}, {0x02, 0x01},
  {0x00, 0x35}, {0x02, 0x01}, {0x00, 0x60}, {0x00, 0x06},
  {0x02, 0x01}, {0x00, 0x16}, {0x00, 0x62}, {0x04, 0x01},
  {0x02, 0x01}, {0x00, 0x26}, {0x00, 0x54}, {0x02, 0x01},
  {0x00, 0x45}, {0x00, 0x63}, {0x0a, 0x01}, {0x06, 0x01},
  {0x02, 0x01}, {0x00, 0x36}, {0x02, 0x01}, {0x00, 0x70},
  {0x00, 0x07}, {0x02, 0x01}, {0x00, 0x71}, {0x00, 0x55},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x17}, {0x00, 0x64},
  {0x02, 0x01}, {0x00, 0x72}, {0x00, 0x27}, {0x18, 0x01},
  {0x10, 0x01}, {0x08, 0x01}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0x46}, {0x00, 0x73}, {0x02, 0x01}, {0x00, 0x37},
  {0x00, 0x65}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x56},
  {0x00, 0x80}, {0x02, 0x01}, {0x00, 0x08}, {0x00, 0x74},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x81}, {0x00, 0x18},
  {0x02, 0x01}, {0x00, 0x82}, {0x00, 0x28}, {0x10, 0x01},
  {0x08, 0x01}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x47},
  {0x00, 0x66}, {0x02, 0x01}, {0x00, 0x83}, {0x00, 0x38},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x75}, {0x00, 0x57},
  {0x02, 0x01}, {0x00, 0x84}, {0x00, 0x48}, {0x06, 0x01},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x90}, {0x00, 0x19},
  {0x00, 0x91}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x92},
  {0x00, 0x76}, {0x02, 0x01}, {0x00, 0x67}, {0x00, 0x29},
  {0x5c, 0x01}, {0x24, 0x01}, {0x12, 0x01}, {0x0a, 0x01},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x85}, {0x00, 0x58},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x09}, {0x00, 0x77},
  {0x00, 0x93}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x39},
  {0x00, 0x94}, {0x02, 0x01}, {0x00, 0x49}, {0x00, 0x86},
  {0x0a, 0x01}, {0x06, 0x01}, {0x02, 0x01}, {0x00, 0x68},
  {0x02, 0x01}, {0x00, 0xa0}, {0x00, 0x0a}, {0x02, 0x01},
  {0x00, 0xa1}, {0x00, 0x1a}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0xa2}, {0x00, 0x2a}, {0x02, 0x01}, {0x00, 0x95},
  {0x00, 0x59}, {0x1a, 0x01}, {0x0e, 0x01}, {0x06, 0x01},
  {0x02, 0x01}, {0x00, 0xa3}, {0x02, 0x01}, {0x00, 0x3a},
  {0x00, 0x87}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x78},
  {0x00, 0xa4}, {0x02, 0x01}, {0x00, 0x4a}, {0x00, 0x96},
  {0x06, 0x01}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x69},
  {0x00, 0xb0}, {0x00, 0xb1}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0x1b}, {0x00, 0xa5}, {0x00, 0xb2}, {0x0e, 0x01},
  {0x08, 0x01}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x5a},
  {0x00, 0x2b}, {0x02, 0x01}, {0x00, 0x88}, {0x00, 0x97},
  {0x02, 0x01}, {0x00, 0xb3}, {0x02, 0x01}, {0x00, 0x79},
  {0x00, 0x3b}, {0x08, 0x01}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0x6a}, {0x00, 0xb4}, {0x02, 0x01}, {0x00, 0x4b},
  {0x00, 0xc1}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x98},
  {0x00, 0x89}, {0x02, 0x01}, {0x00, 0x1c}, {0x00, 0xb5},
  {0x50, 0x01}, {0x22, 0x01}, {0x10, 0x01}, {0x06, 0x01},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x5b}, {0x00, 0x2c},
  {0x00, 0xc2}, {0x06, 0x01}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0x0b}, {0x00, 0xc0}, {0x00, 0xa6}, {0x02, 0x01},
  {0x00, 0xa7}, {0x00, 0x7a}, {0x0a, 0x01}, {0x04, 0x01},
  {0x02, 0x01}, {0x00, 0xc3}, {0x00, 0x3c}, {0x04, 0x01},
  {0x02, 0x01}, {0x00, 0x0c}, {0x00, 0x99}, {0x00, 0xb6},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x6b}, {0x00, 0xc4},
  {0x02, 0x01}, {0x00, 0x4c}, {0x00, 0xa8}, {0x14, 0x01},
  {0x0a, 0x01}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x8a},
  {0x00, 0xc5}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0xd0},
  {0x00, 0x5c}, {0x00, 0xd1}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0xb7}, {0x00, 0x7b}, {0x02, 0x01}, {0x00, 0x1d},
  {0x02, 0x01}, {0x00, 0x0d}, {0x00, 0x2d}, {0x0c, 0x01},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0xd2}, {0x00, 0xd3},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x3d}, {0x00, 0xc6},
  {0x02, 0x01}, {0x00, 0x6c}, {0x00, 0xa9}, {0x06, 0x01},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x9a}, {0x00, 0xb8},
  {0x00, 0xd4}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x8b},
  {0x00, 0x4d}, {0x02, 0x01}, {0x00, 0xc7}, {0x00, 0x7c},
  {0x44, 0x01}, {0x22, 0x01}, {0x12, 0x01}, {0x0a, 0x01},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0xd5}, {0x00, 0x5d},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0xe0}, {0x00, 0x0e},
  {0x00, 0xe1}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x1e},
  {0x00, 0xe2}, {0x02, 0x01}, {0x00, 0xaa}, {0x00, 0x2e},
  {0x08, 0x01}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0xb9},
  {0x00, 0x9b}, {0x02, 0x01}, {0x00, 0xe3}, {0x00, 0xd6},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x6d}, {0x00, 0x3e},
  {0x02, 0x01}, {0x00, 0xc8}, {0x00, 0x8c}, {0x10, 0x01},
  {0x08, 0x01}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0xe4},
  {0x00, 0x4e}, {0x02, 0x01}, {0x00, 0xd7}, {0x00, 0x7d},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0xe5}, {0x00, 0xba},
  {0x02, 0x01}, {0x00, 0xab}, {0x00, 0x5e}, {0x08, 0x01},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0xc9}, {0x00, 0x9c},
  {0x02, 0x01}, {0x00, 0xf1}, {0x00, 0x1f}, {0x06, 0x01},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0xf0}, {0x00, 0x6e},
  {0x00, 0xf2}, {0x02, 0x01}, {0x00, 0x2f}, {0x00, 0xe6},
  {0x26, 0x01}, {0x12, 0x01}, {0x08, 0x01}, {0x04, 0x01},
  {0x02, 0x01}, {0x00, 0xd8}, {0x00, 0xf3}, {0x02, 0x01},
  {0x00, 0x3f}, {0x00, 0xf4}, {0x06, 0x01}, {0x02, 0x01},
  {0x00, 0x4f}, {0x02, 0x01}, {0x00, 0x8d}, {0x00, 0xd9},
  {0x02, 0x01}, {0x00, 0xbb}, {0x00, 0xca}, {0x08, 0x01},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0xac}, {0x00, 0xe7},
  {0x02, 0x01}, {0x00, 0x7e}, {0x00, 0xf5}, {0x08, 0x01},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x9d}, {0x00, 0x5f},
  {0x02, 0x01}, {0x00, 0xe8}, {0x00, 0x8e}, {0x02, 0x01},
  {0x00, 0xf6}, {0x00, 0xcb}, {0x22, 0x01}, {0x12, 0x01},
  {0x0a, 0x01}, {0x06, 0x01}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0x0f}, {0x00, 0xae}, {0x00, 0x6f}, {0x02, 0x01},
  {0x00, 0xbc}, {0x00, 0xda}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0xad}, {0x00, 0xf7}, {0x02, 0x01}, {0x00, 0x7f},
  {0x00, 0xe9}, {0x08, 0x01}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0x9e}, {0x00, 0xcc}, {0x02, 0x01}, {0x00, 0xf8},
  {0x00, 0x8f}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0xdb},
  {0x00, 0xbd}, {0x02, 0x01}, {0x00, 0xea}, {0x00, 0xf9},
  {0x10, 0x01}, {0x08, 0x01}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0x9f}, {0x00, 0xdc}, {0x02, 0x01}, {0x00, 0xcd},
  {0x00, 0xeb}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0xbe},
  {0x00, 0xfa}, {0x02, 0x01}, {0x00, 0xaf}, {0x00, 0xdd},
  {0x0e, 0x01}, {0x06, 0x01}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0xec}, {0x00, 0xce}, {0x00, 0xfb}, {0x04, 0x01},
  {0x02, 0x01}, {0x00, 0xbf}, {0x00, 0xed}, {0x02, 0x01},
  {0x00, 0xde}, {0x00, 0xfc}, {0x06, 0x01}, {0x04, 0x01},
  {0x02, 0x01}, {0x00, 0xcf}, {0x00, 0xfd}, {0x00, 0xee},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0xdf}, {0x00, 0xfe},
  {0x02, 0x01}, {0x00, 0xef}, {0x00, 0xff}
};

static const guchar huffbits_16[511][2] = {
  {0x02, 0x01}, {0x00, 0x00}, {0x06, 0x01}, {0x02, 0x01},
  {0x00, 0x10}, {0x02, 0x01}, {0x00, 0x01}, {0x00, 0x11},
  {0x2a, 0x01}, {0x08, 0x01}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0x20}, {0x00, 0x02}, {0x02, 0x01}, {0x00, 0x21},
  {0x00, 0x12}, {0x0a, 0x01}, {0x06, 0x01}, {0x02, 0x01},
  {0x00, 0x22}, {0x02, 0x01}, {0x00, 0x30}, {0x00, 0x03},
  {0x02, 0x01}, {0x00, 0x31}, {0x00, 0x13}, {0x0a, 0x01},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x32}, {0x00, 0x23},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x40}, {0x00, 0x04},
  {0x00, 0x41}, {0x06, 0x01}, {0x02, 0x01}, {0x00, 0x14},
  {0x02, 0x01}, {0x00, 0x33}, {0x00, 0x42}, {0x04, 0x01},
  {0x02, 0x01}, {0x00, 0x24}, {0x00, 0x50}, {0x02, 0x01},
  {0x00, 0x43}, {0x00, 0x34}, {0x8a, 0x01}, {0x28, 0x01},
  {0x10, 0x01}, {0x06, 0x01}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0x05}, {0x00, 0x15}, {0x00, 0x51}, {0x04, 0x01},
  {0x02, 0x01}, {0x00, 0x52}, {0x00, 0x25}, {0x04, 0x01},
  {0x02, 0x01}, {0x00, 0x44}, {0x00, 0x35}, {0x00, 0x53},
  {0x0a, 0x01}, {0x06, 0x01}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0x60}, {0x00, 0x06}, {0x00, 0x61}, {0x02, 0x01},
  {0x00, 0x16}, {0x00, 0x62}, {0x08, 0x01}, {0x04, 0x01},
  {0x02, 0x01}, {0x00, 0x26}, {0x00, 0x54}, {0x02, 0x01},
  {0x00, 0x45}, {0x00, 0x63}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0x36}, {0x00, 0x70}, {0x00, 0x71}, {0x28, 0x01},
  {0x12, 0x01}, {0x08, 0x01}, {0x02, 0x01}, {0x00, 0x17},
  {0x02, 0x01}, {0x00, 0x07}, {0x02, 0x01}, {0x00, 0x55},
  {0x00, 0x64}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x72},
  {0x00, 0x27}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x46},
  {0x00, 0x65}, {0x00, 0x73}, {0x0a, 0x01}, {0x06, 0x01},
  {0x02, 0x01}, {0x00, 0x37}, {0x02, 0x01}, {0x00, 0x56},
  {0x00, 0x08}, {0x02, 0x01}, {0x00, 0x80}, {0x00, 0x81},
  {0x06, 0x01}, {0x02, 0x01}, {0x00, 0x18}, {0x02, 0x01},
  {0x00, 0x74}, {0x00, 0x47}, {0x02, 0x01}, {0x00, 0x82},
  {0x02, 0x01}, {0x00, 0x28}, {0x00, 0x66}, {0x18, 0x01},
  {0x0e, 0x01}, {0x08, 0x01}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0x83}, {0x00, 0x38}, {0x02, 0x01}, {0x00, 0x75},
  {0x00, 0x84}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x48},
  {0x00, 0x90}, {0x00, 0x91}, {0x06, 0x01}, {0x02, 0x01},
  {0x00, 0x19}, {0x02, 0x01}, {0x00, 0x09}, {0x00, 0x76},
  {0x02, 0x01}, {0x00, 0x92}, {0x00, 0x29}, {0x0e, 0x01},
  {0x08, 0x01}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x85},
  {0x00, 0x58}, {0x02, 0x01}, {0x00, 0x93}, {0x00, 0x39},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0xa0}, {0x00, 0x0a},
  {0x00, 0x1a}, {0x08, 0x01}, {0x02, 0x01}, {0x00, 0xa2},
  {0x02, 0x01}, {0x00, 0x67}, {0x02, 0x01}, {0x00, 0x57},
  {0x00, 0x49}, {0x06, 0x01}, {0x02, 0x01}, {0x00, 0x94},
  {0x02, 0x01}, {0x00, 0x77}, {0x00, 0x86}, {0x02, 0x01},
  {0x00, 0xa1}, {0x02, 0x01}, {0x00, 0x68}, {0x00, 0x95},
  {0xdc, 0x01}, {0x7e, 0x01}, {0x32, 0x01}, {0x1a, 0x01},
  {0x0c, 0x01}, {0x06, 0x01}, {0x02, 0x01}, {0x00, 0x2a},
  {0x02, 0x01}, {0x00, 0x59}, {0x00, 0x3a}, {0x02, 0x01},
  {0x00, 0xa3}, {0x02, 0x01}, {0x00, 0x87}, {0x00, 0x78},
  {0x08, 0x01}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0xa4},
  {0x00, 0x4a}, {0x02, 0x01}, {0x00, 0x96}, {0x00, 0x69},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0xb0}, {0x00, 0x0b},
  {0x00, 0xb1}, {0x0a, 0x01}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0x1b}, {0x00, 0xb2}, {0x02, 0x01}, {0x00, 0x2b},
  {0x02, 0x01}, {0x00, 0xa5}, {0x00, 0x5a}, {0x06, 0x01},
  {0x02, 0x01}, {0x00, 0xb3}, {0x02, 0x01}, {0x00, 0xa6},
  {0x00, 0x6a}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0xb4},
  {0x00, 0x4b}, {0x02, 0x01}, {0x00, 0x0c}, {0x00, 0xc1},
  {0x1e, 0x01}, {0x0e, 0x01}, {0x06, 0x01}, {0x04, 0x01},
  {0x02, 0x01}, {0x00, 0xb5}, {0x00, 0xc2}, {0x00, 0x2c},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0xa7}, {0x00, 0xc3},
  {0x02, 0x01}, {0x00, 0x6b}, {0x00, 0xc4}, {0x08, 0x01},
  {0x02, 0x01}, {0x00, 0x1d}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0x88}, {0x00, 0x97}, {0x00, 0x3b}, {0x04, 0x01},
  {0x02, 0x01}, {0x00, 0xd1}, {0x00, 0xd2}, {0x02, 0x01},
  {0x00, 0x2d}, {0x00, 0xd3}, {0x12, 0x01}, {0x06, 0x01},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x1e}, {0x00, 0x2e},
  {0x00, 0xe2}, {0x06, 0x01}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0x79}, {0x00, 0x98}, {0x00, 0xc0}, {0x02, 0x01},
  {0x00, 0x1c}, {0x02, 0x01}, {0x00, 0x89}, {0x00, 0x5b},
  {0x0e, 0x01}, {0x06, 0x01}, {0x02, 0x01}, {0x00, 0x3c},
  {0x02, 0x01}, {0x00, 0x7a}, {0x00, 0xb6}, {0x04, 0x01},
  {0x02, 0x01}, {0x00, 0x4c}, {0x00, 0x99}, {0x02, 0x01},
  {0x00, 0xa8}, {0x00, 0x8a}, {0x06, 0x01}, {0x02, 0x01},
  {0x00, 0x0d}, {0x02, 0x01}, {0x00, 0xc5}, {0x00, 0x5c},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x3d}, {0x00, 0xc6},
  {0x02, 0x01}, {0x00, 0x6c}, {0x00, 0x9a}, {0x58, 0x01},
  {0x56, 0x01}, {0x24, 0x01}, {0x10, 0x01}, {0x08, 0x01},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x8b}, {0x00, 0x4d},
  {0x02, 0x01}, {0x00, 0xc7}, {0x00, 0x7c}, {0x04, 0x01},
  {0x02, 0x01}, {0x00, 0xd5}, {0x00, 0x5d}, {0x02, 0x01},
  {0x00, 0xe0}, {0x00, 0x0e}, {0x08, 0x01}, {0x02, 0x01},
  {0x00, 0xe3}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0xd0},
  {0x00, 0xb7}, {0x00, 0x7b}, {0x06, 0x01}, {0x04, 0x01},
  {0x02, 0x01}, {0x00, 0xa9}, {0x00, 0xb8}, {0x00, 0xd4},
  {0x02, 0x01}, {0x00, 0xe1}, {0x02, 0x01}, {0x00, 0xaa},
  {0x00, 0xb9}, {0x18, 0x01}, {0x0a, 0x01}, {0x06, 0x01},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x9b}, {0x00, 0xd6},
  {0x00, 0x6d}, {0x02, 0x01}, {0x00, 0x3e}, {0x00, 0xc8},
  {0x06, 0x01}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x8c},
  {0x00, 0xe4}, {0x00, 0x4e}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0xd7}, {0x00, 0xe5}, {0x02, 0x01}, {0x00, 0xba},
  {0x00, 0xab}, {0x0c, 0x01}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0x9c}, {0x00, 0xe6}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0x6e}, {0x00, 0xd8}, {0x02, 0x01}, {0x00, 0x8d},
  {0x00, 0xbb}, {0x08, 0x01}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0xe7}, {0x00, 0x9d}, {0x02, 0x01}, {0x00, 0xe8},
  {0x00, 0x8e}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0xcb},
  {0x00, 0xbc}, {0x00, 0x9e}, {0x00, 0xf1}, {0x02, 0x01},
  {0x00, 0x1f}, {0x02, 0x01}, {0x00, 0x0f}, {0x00, 0x2f},
  {0x42, 0x01}, {0x38, 0x01}, {0x02, 0x01}, {0x00, 0xf2},
  {0x34, 0x01}, {0x32, 0x01}, {0x14, 0x01}, {0x08, 0x01},
  {0x02, 0x01}, {0x00, 0xbd}, {0x02, 0x01}, {0x00, 0x5e},
  {0x02, 0x01}, {0x00, 0x7d}, {0x00, 0xc9}, {0x06, 0x01},
  {0x02, 0x01}, {0x00, 0xca}, {0x02, 0x01}, {0x00, 0xac},
  {0x00, 0x7e}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0xda},
  {0x00, 0xad}, {0x00, 0xcc}, {0x0a, 0x01}, {0x06, 0x01},
  {0x02, 0x01}, {0x00, 0xae}, {0x02, 0x01}, {0x00, 0xdb},
  {0x00, 0xdc}, {0x02, 0x01}, {0x00, 0xcd}, {0x00, 0xbe},
  {0x06, 0x01}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0xeb},
  {0x00, 0xed}, {0x00, 0xee}, {0x06, 0x01}, {0x04, 0x01},
  {0x02, 0x01}, {0x00, 0xd9}, {0x00, 0xea}, {0x00, 0xe9},
  {0x02, 0x01}, {0x00, 0xde}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0xdd}, {0x00, 0xec}, {0x00, 0xce}, {0x00, 0x3f},
  {0x00, 0xf0}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0xf3},
  {0x00, 0xf4}, {0x02, 0x01}, {0x00, 0x4f}, {0x02, 0x01},
  {0x00, 0xf5}, {0x00, 0x5f}, {0x0a, 0x01}, {0x02, 0x01},
  {0x00, 0xff}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0xf6},
  {0x00, 0x6f}, {0x02, 0x01}, {0x00, 0xf7}, {0x00, 0x7f},
  {0x0c, 0x01}, {0x06, 0x01}, {0x02, 0x01}, {0x00, 0x8f},
  {0x02, 0x01}, {0x00, 0xf8}, {0x00, 0xf9}, {0x04, 0x01},
  {0x02, 0x01}, {0x00, 0x9f}, {0x00, 0xfa}, {0x00, 0xaf},
  {0x08, 0x01}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0xfb},
  {0x00, 0xbf}, {0x02, 0x01}, {0x00, 0xfc}, {0x00, 0xcf},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0xfd}, {0x00, 0xdf},
  {0x02, 0x01}, {0x00, 0xfe}, {0x00, 0xef}
};

static const guchar huffbits_24[512][2] = {
  {0x3c, 0x01}, {0x08, 0x01}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0x00}, {0x00, 0x10}, {0x02, 0x01}, {0x00, 0x01},
  {0x00, 0x11}, {0x0e, 0x01}, {0x06, 0x01}, {0x04, 0x01},
  {0x02, 0x01}, {0x00, 0x20}, {0x00, 0x02}, {0x00, 0x21},
  {0x02, 0x01}, {0x00, 0x12}, {0x02, 0x01}, {0x00, 0x22},
  {0x02, 0x01}, {0x00, 0x30}, {0x00, 0x03}, {0x0e, 0x01},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x31}, {0x00, 0x13},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x32}, {0x00, 0x23},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x40}, {0x00, 0x04},
  {0x00, 0x41}, {0x08, 0x01}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0x14}, {0x00, 0x33}, {0x02, 0x01}, {0x00, 0x42},
  {0x00, 0x24}, {0x06, 0x01}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0x43}, {0x00, 0x34}, {0x00, 0x51}, {0x06, 0x01},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x50}, {0x00, 0x05},
  {0x00, 0x15}, {0x02, 0x01}, {0x00, 0x52}, {0x00, 0x25},
  {0xfa, 0x01}, {0x62, 0x01}, {0x22, 0x01}, {0x12, 0x01},
  {0x0a, 0x01}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x44},
  {0x00, 0x53}, {0x02, 0x01}, {0x00, 0x35}, {0x02, 0x01},
  {0x00, 0x60}, {0x00, 0x06}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0x61}, {0x00, 0x16}, {0x02, 0x01}, {0x00, 0x62},
  {0x00, 0x26}, {0x08, 0x01}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0x54}, {0x00, 0x45}, {0x02, 0x01}, {0x00, 0x63},
  {0x00, 0x36}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x71},
  {0x00, 0x55}, {0x02, 0x01}, {0x00, 0x64}, {0x00, 0x46},
  {0x20, 0x01}, {0x0e, 0x01}, {0x06, 0x01}, {0x02, 0x01},
  {0x00, 0x72}, {0x02, 0x01}, {0x00, 0x27}, {0x00, 0x37},
  {0x02, 0x01}, {0x00, 0x73}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0x70}, {0x00, 0x07}, {0x00, 0x17}, {0x0a, 0x01},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x65}, {0x00, 0x56},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x80}, {0x00, 0x08},
  {0x00, 0x81}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x74},
  {0x00, 0x47}, {0x02, 0x01}, {0x00, 0x18}, {0x00, 0x82},
  {0x10, 0x01}, {0x08, 0x01}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0x28}, {0x00, 0x66}, {0x02, 0x01}, {0x00, 0x83},
  {0x00, 0x38}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x75},
  {0x00, 0x57}, {0x02, 0x01}, {0x00, 0x84}, {0x00, 0x48},
  {0x08, 0x01}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x91},
  {0x00, 0x19}, {0x02, 0x01}, {0x00, 0x92}, {0x00, 0x76},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x67}, {0x00, 0x29},
  {0x02, 0x01}, {0x00, 0x85}, {0x00, 0x58}, {0x5c, 0x01},
  {0x22, 0x01}, {0x10, 0x01}, {0x08, 0x01}, {0x04, 0x01},
  {0x02, 0x01}, {0x00, 0x93}, {0x00, 0x39}, {0x02, 0x01},
  {0x00, 0x94}, {0x00, 0x49}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0x77}, {0x00, 0x86}, {0x02, 0x01}, {0x00, 0x68},
  {0x00, 0xa1}, {0x08, 0x01}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0xa2}, {0x00, 0x2a}, {0x02, 0x01}, {0x00, 0x95},
  {0x00, 0x59}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0xa3},
  {0x00, 0x3a}, {0x02, 0x01}, {0x00, 0x87}, {0x02, 0x01},
  {0x00, 0x78}, {0x00, 0x4a}, {0x16, 0x01}, {0x0c, 0x01},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0xa4}, {0x00, 0x96},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x69}, {0x00, 0xb1},
  {0x02, 0x01}, {0x00, 0x1b}, {0x00, 0xa5}, {0x06, 0x01},
  {0x02, 0x01}, {0x00, 0xb2}, {0x02, 0x01}, {0x00, 0x5a},
  {0x00, 0x2b}, {0x02, 0x01}, {0x00, 0x88}, {0x00, 0xb3},
  {0x10, 0x01}, {0x0a, 0x01}, {0x06, 0x01}, {0x02, 0x01},
  {0x00, 0x90}, {0x02, 0x01}, {0x00, 0x09}, {0x00, 0xa0},
  {0x02, 0x01}, {0x00, 0x97}, {0x00, 0x79}, {0x04, 0x01},
  {0x02, 0x01}, {0x00, 0xa6}, {0x00, 0x6a}, {0x00, 0xb4},
  {0x0c, 0x01}, {0x06, 0x01}, {0x02, 0x01}, {0x00, 0x1a},
  {0x02, 0x01}, {0x00, 0x0a}, {0x00, 0xb0}, {0x02, 0x01},
  {0x00, 0x3b}, {0x02, 0x01}, {0x00, 0x0b}, {0x00, 0xc0},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x4b}, {0x00, 0xc1},
  {0x02, 0x01}, {0x00, 0x98}, {0x00, 0x89}, {0x43, 0x01},
  {0x22, 0x01}, {0x10, 0x01}, {0x08, 0x01}, {0x04, 0x01},
  {0x02, 0x01}, {0x00, 0x1c}, {0x00, 0xb5}, {0x02, 0x01},
  {0x00, 0x5b}, {0x00, 0xc2}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0x2c}, {0x00, 0xa7}, {0x02, 0x01}, {0x00, 0x7a},
  {0x00, 0xc3}, {0x0a, 0x01}, {0x06, 0x01}, {0x02, 0x01},
  {0x00, 0x3c}, {0x02, 0x01}, {0x00, 0x0c}, {0x00, 0xd0},
  {0x02, 0x01}, {0x00, 0xb6}, {0x00, 0x6b}, {0x04, 0x01},
  {0x02, 0x01}, {0x00, 0xc4}, {0x00, 0x4c}, {0x02, 0x01},
  {0x00, 0x99}, {0x00, 0xa8}, {0x10, 0x01}, {0x08, 0x01},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x8a}, {0x00, 0xc5},
  {0x02, 0x01}, {0x00, 0x5c}, {0x00, 0xd1}, {0x04, 0x01},
  {0x02, 0x01}, {0x00, 0xb7}, {0x00, 0x7b}, {0x02, 0x01},
  {0x00, 0x1d}, {0x00, 0xd2}, {0x09, 0x01}, {0x04, 0x01},
  {0x02, 0x01}, {0x00, 0x2d}, {0x00, 0xd3}, {0x02, 0x01},
  {0x00, 0x3d}, {0x00, 0xc6}, {0x55, 0xfa}, {0x04, 0x01},
  {0x02, 0x01}, {0x00, 0x6c}, {0x00, 0xa9}, {0x02, 0x01},
  {0x00, 0x9a}, {0x00, 0xd4}, {0x20, 0x01}, {0x10, 0x01},
  {0x08, 0x01}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0xb8},
  {0x00, 0x8b}, {0x02, 0x01}, {0x00, 0x4d}, {0x00, 0xc7},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x7c}, {0x00, 0xd5},
  {0x02, 0x01}, {0x00, 0x5d}, {0x00, 0xe1}, {0x08, 0x01},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x1e}, {0x00, 0xe2},
  {0x02, 0x01}, {0x00, 0xaa}, {0x00, 0xb9}, {0x04, 0x01},
  {0x02, 0x01}, {0x00, 0x9b}, {0x00, 0xe3}, {0x02, 0x01},
  {0x00, 0xd6}, {0x00, 0x6d}, {0x14, 0x01}, {0x0a, 0x01},
  {0x06, 0x01}, {0x02, 0x01}, {0x00, 0x3e}, {0x02, 0x01},
  {0x00, 0x2e}, {0x00, 0x4e}, {0x02, 0x01}, {0x00, 0xc8},
  {0x00, 0x8c}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0xe4},
  {0x00, 0xd7}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x7d},
  {0x00, 0xab}, {0x00, 0xe5}, {0x0a, 0x01}, {0x04, 0x01},
  {0x02, 0x01}, {0x00, 0xba}, {0x00, 0x5e}, {0x02, 0x01},
  {0x00, 0xc9}, {0x02, 0x01}, {0x00, 0x9c}, {0x00, 0x6e},
  {0x08, 0x01}, {0x02, 0x01}, {0x00, 0xe6}, {0x02, 0x01},
  {0x00, 0x0d}, {0x02, 0x01}, {0x00, 0xe0}, {0x00, 0x0e},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0xd8}, {0x00, 0x8d},
  {0x02, 0x01}, {0x00, 0xbb}, {0x00, 0xca}, {0x4a, 0x01},
  {0x02, 0x01}, {0x00, 0xff}, {0x40, 0x01}, {0x3a, 0x01},
  {0x20, 0x01}, {0x10, 0x01}, {0x08, 0x01}, {0x04, 0x01},
  {0x02, 0x01}, {0x00, 0xac}, {0x00, 0xe7}, {0x02, 0x01},
  {0x00, 0x7e}, {0x00, 0xd9}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0x9d}, {0x00, 0xe8}, {0x02, 0x01}, {0x00, 0x8e},
  {0x00, 0xcb}, {0x08, 0x01}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0xbc}, {0x00, 0xda}, {0x02, 0x01}, {0x00, 0xad},
  {0x00, 0xe9}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x9e},
  {0x00, 0xcc}, {0x02, 0x01}, {0x00, 0xdb}, {0x00, 0xbd},
  {0x10, 0x01}, {0x08, 0x01}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0xea}, {0x00, 0xae}, {0x02, 0x01}, {0x00, 0xdc},
  {0x00, 0xcd}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0xeb},
  {0x00, 0xbe}, {0x02, 0x01}, {0x00, 0xdd}, {0x00, 0xec},
  {0x08, 0x01}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0xce},
  {0x00, 0xed}, {0x02, 0x01}, {0x00, 0xde}, {0x00, 0xee},
  {0x00, 0x0f}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0xf0},
  {0x00, 0x1f}, {0x00, 0xf1}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0xf2}, {0x00, 0x2f}, {0x02, 0x01}, {0x00, 0xf3},
  {0x00, 0x3f}, {0x12, 0x01}, {0x08, 0x01}, {0x04, 0x01},
  {0x02, 0x01}, {0x00, 0xf4}, {0x00, 0x4f}, {0x02, 0x01},
  {0x00, 0xf5}, {0x00, 0x5f}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0xf6}, {0x00, 0x6f}, {0x02, 0x01}, {0x00, 0xf7},
  {0x02, 0x01}, {0x00, 0x7f}, {0x00, 0x8f}, {0x0a, 0x01},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0xf8}, {0x00, 0xf9},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x9f}, {0x00, 0xaf},
  {0x00, 0xfa}, {0x08, 0x01}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0xfb}, {0x00, 0xbf}, {0x02, 0x01}, {0x00, 0xfc},
  {0x00, 0xcf}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0xfd},
  {0x00, 0xdf}, {0x02, 0x01}, {0x00, 0xfe}, {0x00, 0xef}

};

static const guchar huffbits_32[31][2] = {
  {0x02, 0x01}, {0x00, 0x00}, {0x08, 0x01}, {0x04, 0x01},
  {0x02, 0x01}, {0x00, 0x08}, {0x00, 0x04}, {0x02, 0x01},
  {0x00, 0x01}, {0x00, 0x02}, {0x08, 0x01}, {0x04, 0x01},
  {0x02, 0x01}, {0x00, 0x0c}, {0x00, 0x0a}, {0x02, 0x01},
  {0x00, 0x03}, {0x00, 0x06}, {0x06, 0x01}, {0x02, 0x01},
  {0x00, 0x09}, {0x02, 0x01}, {0x00, 0x05}, {0x00, 0x07},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x0e}, {0x00, 0x0d},
  {0x02, 0x01}, {0x00, 0x0f}, {0x00, 0x0b}
};

static const guchar huffbits_33[31][2] = {
  {0x10, 0x01}, {0x08, 0x01}, {0x04, 0x01}, {0x02, 0x01},
  {0x00, 0x00}, {0x00, 0x01}, {0x02, 0x01}, {0x00, 0x02},
  {0x00, 0x03}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x04},
  {0x00, 0x05}, {0x02, 0x01}, {0x00, 0x06}, {0x00, 0x07},
  {0x08, 0x01}, {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x08},
  {0x00, 0x09}, {0x02, 0x01}, {0x00, 0x0a}, {0x00, 0x0b},
  {0x04, 0x01}, {0x02, 0x01}, {0x00, 0x0c}, {0x00, 0x0d},
  {0x02, 0x01}, {0x00, 0x0e}, {0x00, 0x0f}
};

/* Array of decoder table structures */
static const struct huffcodetab huff_tables[] = {
  /* 0 */ {0, 0, 0, 0, FALSE, NULL},
  /* 1 */ {7, 2, 2, 0, FALSE, huffbits_1},
  /* 2 */ {17, 3, 3, 0, FALSE, huffbits_2},
  /* 3 */ {17, 3, 3, 0, FALSE, huffbits_3},
  /* 4 */ {0, 0, 0, 0, FALSE, NULL},
  /* 5 */ {31, 4, 4, 0, FALSE, huffbits_5},
  /* 6 */ {31, 4, 4, 0, FALSE, huffbits_6},
  /* 7 */ {71, 6, 6, 0, FALSE, huffbits_7},
  /* 8 */ {71, 6, 6, 0, FALSE, huffbits_8},
  /* 9 */ {71, 6, 6, 0, FALSE, huffbits_9},
  /* 10 */ {127, 8, 8, 0, FALSE, huffbits_10},
  /* 11 */ {127, 8, 8, 0, FALSE, huffbits_11},
  /* 12 */ {127, 8, 8, 0, FALSE, huffbits_12},
  /* 13 */ {511, 16, 16, 0, FALSE, huffbits_13},
  /* 14 */ {0, 0, 0, 0, FALSE, NULL},
  /* 15 */ {511, 16, 16, 0, FALSE, huffbits_15},
  /* 16 */ {511, 16, 16, 1, FALSE, huffbits_16},
  /* 17 */ {511, 16, 16, 2, FALSE, huffbits_16},
  /* 18 */ {511, 16, 16, 3, FALSE, huffbits_16},
  /* 19 */ {511, 16, 16, 4, FALSE, huffbits_16},
  /* 20 */ {511, 16, 16, 6, FALSE, huffbits_16},
  /* 21 */ {511, 16, 16, 8, FALSE, huffbits_16},
  /* 22 */ {511, 16, 16, 10, FALSE, huffbits_16},
  /* 23 */ {511, 16, 16, 13, FALSE, huffbits_16},
  /* 24 */ {512, 16, 16, 4, FALSE, huffbits_24},
  /* 25 */ {512, 16, 16, 5, FALSE, huffbits_24},
  /* 26 */ {512, 16, 16, 6, FALSE, huffbits_24},
  /* 27 */ {512, 16, 16, 7, FALSE, huffbits_24},
  /* 28 */ {512, 16, 16, 8, FALSE, huffbits_24},
  /* 29 */ {512, 16, 16, 9, FALSE, huffbits_24},
  /* 30 */ {512, 16, 16, 11, FALSE, huffbits_24},
  /* 31 */ {512, 16, 16, 13, FALSE, huffbits_24},
  /* 32 */ {31, 1, 16, 0, TRUE, huffbits_32},
  /* 33 */ {31, 1, 16, 0, TRUE, huffbits_33}
};

static const gfloat pow_2_table[] = {
0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 
0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 
0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 
0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 
0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 
0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 
0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 
0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 
0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 
0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 
0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 
0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 
0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 
0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 
0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 
0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 
0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 
0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 
0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 
0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 
0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 
0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 
0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 
0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 
0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 
0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 
0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 
0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 
0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 
0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 
0.000000000000000f, 0.000000000000000f, 0.000000000000000f, 0.000000000000001f, 
0.000000000000001f, 0.000000000000001f, 0.000000000000001f, 0.000000000000001f, 
0.000000000000001f, 0.000000000000001f, 0.000000000000002f, 0.000000000000002f, 
0.000000000000003f, 0.000000000000003f, 0.000000000000004f, 0.000000000000004f, 
0.000000000000005f, 0.000000000000006f, 0.000000000000007f, 0.000000000000008f, 
0.000000000000010f, 0.000000000000012f, 0.000000000000014f, 0.000000000000017f, 
0.000000000000020f, 0.000000000000024f, 0.000000000000028f, 0.000000000000034f, 
0.000000000000040f, 0.000000000000048f, 0.000000000000057f, 0.000000000000068f, 
0.000000000000080f, 0.000000000000096f, 0.000000000000114f, 0.000000000000135f, 
0.000000000000161f, 0.000000000000191f, 0.000000000000227f, 0.000000000000270f, 
0.000000000000322f, 0.000000000000382f, 0.000000000000455f, 0.000000000000541f, 
0.000000000000643f, 0.000000000000765f, 0.000000000000909f, 0.000000000001082f, 
0.000000000001286f, 0.000000000001530f, 0.000000000001819f, 0.000000000002163f, 
0.000000000002572f, 0.000000000003059f, 0.000000000003638f, 0.000000000004326f, 
0.000000000005145f, 0.000000000006118f, 0.000000000007276f, 0.000000000008653f, 
0.000000000010290f, 0.000000000012237f, 0.000000000014552f, 0.000000000017305f, 
0.000000000020580f, 0.000000000024473f, 0.000000000029104f, 0.000000000034610f, 
0.000000000041159f, 0.000000000048947f, 0.000000000058208f, 0.000000000069221f, 
0.000000000082318f, 0.000000000097893f, 0.000000000116415f, 0.000000000138442f, 
0.000000000164636f, 0.000000000195786f, 0.000000000232831f, 0.000000000276884f, 
0.000000000329272f, 0.000000000391573f, 0.000000000465661f, 0.000000000553768f, 
0.000000000658544f, 0.000000000783146f, 0.000000000931323f, 0.000000001107535f, 
0.000000001317089f, 0.000000001566292f, 0.000000001862645f, 0.000000002215071f, 
0.000000002634178f, 0.000000003132583f, 0.000000003725290f, 0.000000004430142f, 
0.000000005268356f, 0.000000006265167f, 0.000000007450581f, 0.000000008860283f, 
0.000000010536712f, 0.000000012530333f, 0.000000014901161f, 0.000000017720566f, 
0.000000021073424f, 0.000000025060666f, 0.000000029802322f, 0.000000035441133f, 
0.000000042146848f, 0.000000050121333f, 0.000000059604645f, 0.000000070882265f, 
0.000000084293696f, 0.000000100242666f, 0.000000119209290f, 0.000000141764531f, 
0.000000168587391f, 0.000000200485331f, 0.000000238418579f, 0.000000283529062f, 
0.000000337174782f, 0.000000400970663f, 0.000000476837158f, 0.000000567058123f, 
0.000000674349565f, 0.000000801941326f, 0.000000953674316f, 0.000001134116246f, 
0.000001348699129f, 0.000001603882652f, 0.000001907348633f, 0.000002268232492f, 
0.000002697398259f, 0.000003207765303f, 0.000003814697266f, 0.000004536464985f, 
0.000005394796517f, 0.000006415530606f, 0.000007629394531f, 0.000009072929970f, 
0.000010789593034f, 0.000012831061213f, 0.000015258789062f, 0.000018145859940f, 
0.000021579186068f, 0.000025662122425f, 0.000030517578125f, 0.000036291719880f, 
0.000043158372137f, 0.000051324244851f, 0.000061035156250f, 0.000072583439760f, 
0.000086316744273f, 0.000102648489701f, 0.000122070312500f, 0.000145166879520f, 
0.000172633488546f, 0.000205296979402f, 0.000244140625000f, 0.000290333759040f, 
0.000345266977092f, 0.000410593958804f, 0.000488281250000f, 0.000580667518079f, 
0.000690533954185f, 0.000821187917609f, 0.000976562500000f, 0.001161335036159f, 
0.001381067908369f, 0.001642375835218f, 0.001953125000000f, 0.002322670072317f, 
0.002762135816738f, 0.003284751670435f, 0.003906250000000f, 0.004645340144634f, 
0.005524271633476f, 0.006569503340870f, 0.007812500000000f, 0.009290680289268f, 
0.011048543266952f, 0.013139006681740f, 0.015625000000000f, 0.018581360578537f, 
0.022097086533904f, 0.026278013363481f, 0.031250000000000f, 0.037162721157074f, 
0.044194173067808f, 0.052556026726961f, 0.062500000000000f, 0.074325442314148f, 
0.088388346135616f, 0.105112053453922f, 0.125000000000000f, 0.148650884628296f, 
0.176776692271233f, 0.210224106907845f, 0.250000000000000f, 0.297301769256592f, 
0.353553384542465f, 0.420448213815689f, 0.500000000000000f, 0.594603538513184f, 
0.707106769084930f, 0.840896427631378f, 1.000000000000000f, 1.189207077026367f, 
1.414213538169861f, 1.681792855262756f, 2.000000000000000f, 2.378414154052734f, 
2.828427076339722f, 3.363585710525513f, 4.000000000000000f, 4.756828308105469f, 
5.656854152679443f, 6.727171421051025f, 8.000000000000000f, 9.513656616210938f, 
11.313708305358887f, 13.454342842102051f, 16.000000000000000f, 19.027313232421875f, 
22.627416610717773f, 26.908685684204102f, 32.000000000000000f, 38.054626464843750f, 
45.254833221435547f, 53.817371368408203f, 64.000000000000000f, 76.109252929687500f, 
90.509666442871094f, 107.634742736816406f, 128.000000000000000f, 152.218505859375000f, 
181.019332885742188f, 215.269485473632812f, 256.000000000000000f, 304.437011718750000f, 
362.038665771484375f, 430.538970947265625f, 512.000000000000000f, 608.874023437500000f, 
724.077331542968750f, 861.077941894531250f, 1024.000000000000000f, 1217.748046875000000f, 
1448.154663085937500f, 1722.155883789062500f, 2048.000000000000000f, 2435.496093750000000f
};

static const gfloat pow_43_table[] = {
0.0000000000f, 1.0000000000f, 2.5198421478f, 4.3267488480f, 
6.3496046066f, 8.5498800278f, 10.9027242661f, 13.3905191422f, 
16.0000019073f, 18.7207565308f, 21.5443496704f, 24.4637832642f, 
27.4731445312f, 30.5673542023f, 33.7419967651f, 36.9931869507f, 
40.3174781799f, 43.7117919922f, 47.1733512878f, 50.6996383667f, 
54.2883605957f, 57.9374160767f, 61.6448745728f, 65.4089508057f, 
69.2279891968f, 73.1004562378f, 77.0249099731f, 81.0000076294f, 
85.0245056152f, 89.0971984863f, 93.2169876099f, 97.3828125000f, 
101.5936813354f, 105.8486480713f, 110.1468200684f, 114.4873352051f, 
118.8694000244f, 123.2922286987f, 127.7550811768f, 132.2572631836f, 
136.7980957031f, 141.3769226074f, 145.9931335449f, 150.6461334229f, 
155.3353576660f, 160.0602264404f, 164.8202209473f, 169.6148529053f, 
174.4436035156f, 179.3060150146f, 184.2015991211f, 189.1299438477f, 
194.0906066895f, 199.0831756592f, 204.1072387695f, 209.1624145508f, 
214.2483215332f, 219.3645935059f, 224.5108795166f, 229.6868286133f, 
234.8920898438f, 240.1263732910f, 245.3893127441f, 250.6806488037f, 
256.0000305176f, 261.3472290039f, 266.7218933105f, 272.1237792969f, 
277.5525817871f, 283.0080871582f, 288.4900207520f, 293.9981079102f, 
299.5321350098f, 305.0918273926f, 310.6769409180f, 316.2872924805f, 
321.9226379395f, 327.5827636719f, 333.2674255371f, 338.9764404297f, 
344.7096252441f, 350.4667053223f, 356.2475585938f, 362.0519409180f, 
367.8796691895f, 373.7305908203f, 379.6044921875f, 385.5012207031f, 
391.4205627441f, 397.3623962402f, 403.3265075684f, 409.3127441406f, 
415.3209533691f, 421.3509826660f, 427.4026489258f, 433.4758300781f, 
439.5703430176f, 445.6860656738f, 451.8228454590f, 457.9805297852f, 
464.1589660645f, 470.3580322266f, 476.5776062012f, 482.8175354004f, 
489.0776977539f, 495.3579711914f, 501.6581726074f, 507.9782409668f, 
514.3180541992f, 520.6774291992f, 527.0562744141f, 533.4545288086f, 
539.8719482422f, 546.3085327148f, 552.7641601562f, 559.2387084961f, 
565.7319946289f, 572.2439575195f, 578.7745361328f, 585.3236083984f, 
591.8909912109f, 598.4766845703f, 605.0805664062f, 611.7024536133f, 
618.3423461914f, 625.0001220703f, 631.6756591797f, 638.3688964844f, 
645.0797119141f, 651.8080444336f, 658.5537109375f, 665.3167724609f, 
672.0970458984f, 678.8944702148f, 685.7089233398f, 692.5404052734f, 
699.3887329102f, 706.2538452148f, 713.1357421875f, 720.0342407227f, 
726.9493408203f, 733.8808593750f, 740.8288574219f, 747.7931518555f, 
754.7736816406f, 761.7703857422f, 768.7832031250f, 775.8120727539f, 
782.8568725586f, 789.9176025391f, 796.9940795898f, 804.0863647461f, 
811.1942749023f, 818.3178100586f, 825.4568481445f, 832.6113891602f, 
839.7813110352f, 846.9666137695f, 854.1671752930f, 861.3829345703f, 
868.6138305664f, 875.8598022461f, 883.1207885742f, 890.3967285156f, 
897.6875610352f, 904.9932861328f, 912.3137207031f, 919.6488647461f, 
926.9987182617f, 934.3631591797f, 941.7420654297f, 949.1355590820f, 
956.5433959961f, 963.9656372070f, 971.4022216797f, 978.8530273438f, 
986.3180541992f, 993.7972412109f, 1001.2904663086f, 1008.7977905273f, 
1016.3190917969f, 1023.8543701172f, 1031.4035644531f, 1038.9665527344f, 
1046.5432128906f, 1054.1337890625f, 1061.7379150391f, 1069.3558349609f, 
1076.9871826172f, 1084.6322021484f, 1092.2906494141f, 1099.9626464844f, 
1107.6479492188f, 1115.3465576172f, 1123.0585937500f, 1130.7838134766f, 
1138.5222167969f, 1146.2739257812f, 1154.0385742188f, 1161.8164062500f, 
1169.6072998047f, 1177.4112548828f, 1185.2280273438f, 1193.0577392578f, 
1200.9003906250f, 1208.7558593750f, 1216.6240234375f, 1224.5050048828f, 
1232.3986816406f, 1240.3049316406f, 1248.2238769531f, 1256.1553955078f, 
1264.0994873047f, 1272.0560302734f, 1280.0250244141f, 1288.0064697266f, 
1296.0002441406f, 1304.0064697266f, 1312.0249023438f, 1320.0556640625f, 
1328.0986328125f, 1336.1538085938f, 1344.2211914062f, 1352.3006591797f, 
1360.3922119141f, 1368.4957275391f, 1376.6113281250f, 1384.7388916016f, 
1392.8784179688f, 1401.0299072266f, 1409.1932373047f, 1417.3684082031f, 
1425.5552978516f, 1433.7540283203f, 1441.9644775391f, 1450.1866455078f, 
1458.4205322266f, 1466.6660156250f, 1474.9230957031f, 1483.1917724609f, 
1491.4719238281f, 1499.7636718750f, 1508.0667724609f, 1516.3814697266f, 
1524.7075195312f, 1533.0449218750f, 1541.3936767578f, 1549.7537841797f, 
1558.1251220703f, 1566.5078125000f, 1574.9016113281f, 1583.3067626953f, 
1591.7230224609f, 1600.1503906250f, 1608.5888671875f, 1617.0384521484f, 
1625.4990234375f, 1633.9707031250f, 1642.4533691406f, 1650.9468994141f, 
1659.4515380859f, 1667.9669189453f, 1676.4932861328f, 1685.0305175781f, 
1693.5784912109f, 1702.1373291016f, 1710.7069091797f, 1719.2872314453f, 
1727.8782958984f, 1736.4801025391f, 1745.0925292969f, 1753.7155761719f, 
1762.3492431641f, 1770.9934082031f, 1779.6483154297f, 1788.3135986328f, 
1796.9895019531f, 1805.6759033203f, 1814.3726806641f, 1823.0798339844f, 
1831.7974853516f, 1840.5256347656f, 1849.2639160156f, 1858.0126953125f, 
1866.7717285156f, 1875.5410156250f, 1884.3206787109f, 1893.1104736328f, 
1901.9105224609f, 1910.7207031250f, 1919.5411376953f, 1928.3717041016f, 
1937.2124023438f, 1946.0631103516f, 1954.9239501953f, 1963.7949218750f, 
1972.6757812500f, 1981.5666503906f, 1990.4676513672f, 1999.3785400391f, 
2008.2993164062f, 2017.2299804688f, 2026.1706542969f, 2035.1212158203f, 
2044.0815429688f, 2053.0517578125f, 2062.0317382812f, 2071.0214843750f, 
2080.0209960938f, 2089.0302734375f, 2098.0493164062f, 2107.0781250000f, 
2116.1164550781f, 2125.1645507812f, 2134.2221679688f, 2143.2895507812f, 
2152.3664550781f, 2161.4528808594f, 2170.5490722656f, 2179.6545410156f, 
2188.7697753906f, 2197.8942871094f, 2207.0283203125f, 2216.1718750000f, 
2225.3249511719f, 2234.4873046875f, 2243.6591796875f, 2252.8405761719f, 
2262.0310058594f, 2271.2309570312f, 2280.4401855469f, 2289.6586914062f, 
2298.8864746094f, 2308.1237792969f, 2317.3701171875f, 2326.6257324219f, 
2335.8903808594f, 2345.1645507812f, 2354.4475097656f, 2363.7399902344f, 
2373.0415039062f, 2382.3520507812f, 2391.6718750000f, 2401.0004882812f, 
2410.3383789062f, 2419.6853027344f, 2429.0412597656f, 2438.4062500000f, 
2447.7802734375f, 2457.1630859375f, 2466.5551757812f, 2475.9560546875f, 
2485.3657226562f, 2494.7844238281f, 2504.2121582031f, 2513.6486816406f, 
2523.0939941406f, 2532.5483398438f, 2542.0112304688f, 2551.4831542969f, 
2560.9638671875f, 2570.4531250000f, 2579.9514160156f, 2589.4584960938f, 
2598.9741210938f, 2608.4985351562f, 2618.0314941406f, 2627.5734863281f, 
2637.1237792969f, 2646.6828613281f, 2656.2507324219f, 2665.8271484375f, 
2675.4121093750f, 2685.0056152344f, 2694.6079101562f, 2704.2185058594f, 
2713.8378906250f, 2723.4655761719f, 2733.1020507812f, 2742.7468261719f, 
2752.4001464844f, 2762.0617675781f, 2771.7321777344f, 2781.4108886719f, 
2791.0979003906f, 2800.7934570312f, 2810.4973144531f, 2820.2097167969f, 
2829.9301757812f, 2839.6594238281f, 2849.3967285156f, 2859.1423339844f, 
2868.8962402344f, 2878.6586914062f, 2888.4291992188f, 2898.2080078125f, 
2907.9951171875f, 2917.7905273438f, 2927.5942382812f, 2937.4060058594f, 
2947.2258300781f, 2957.0541992188f, 2966.8903808594f, 2976.7348632812f, 
2986.5876464844f, 2996.4484863281f, 3006.3173828125f, 3016.1943359375f, 
3026.0793457031f, 3035.9726562500f, 3045.8737792969f, 3055.7832031250f, 
3065.7004394531f, 3075.6259765625f, 3085.5593261719f, 3095.5007324219f, 
3105.4499511719f, 3115.4074707031f, 3125.3728027344f, 3135.3459472656f, 
3145.3271484375f, 3155.3161621094f, 3165.3132324219f, 3175.3183593750f, 
3185.3310546875f, 3195.3518066406f, 3205.3803710938f, 3215.4167480469f, 
3225.4609375000f, 3235.5131835938f, 3245.5729980469f, 3255.6406250000f, 
3265.7160644531f, 3275.7993164062f, 3285.8903808594f, 3295.9892578125f, 
3306.0957031250f, 3316.2099609375f, 3326.3320312500f, 3336.4616699219f, 
3346.5988769531f, 3356.7441406250f, 3366.8967285156f, 3377.0571289062f, 
3387.2250976562f, 3397.4008789062f, 3407.5839843750f, 3417.7749023438f, 
3427.9736328125f, 3438.1796875000f, 3448.3933105469f, 3458.6145019531f, 
3468.8432617188f, 3479.0795898438f, 3489.3234863281f, 3499.5749511719f, 
3509.8339843750f, 3520.1003417969f, 3530.3742675781f, 3540.6555175781f, 
3550.9445800781f, 3561.2407226562f, 3571.5446777344f, 3581.8557128906f, 
3592.1743164062f, 3602.5004882812f, 3612.8339843750f, 3623.1748046875f, 
3633.5229492188f, 3643.8786621094f, 3654.2414550781f, 3664.6118164062f, 
3674.9895019531f, 3685.3745117188f, 3695.7668457031f, 3706.1665039062f, 
3716.5732421875f, 3726.9875488281f, 3737.4091796875f, 3747.8378906250f, 
3758.2739257812f, 3768.7170410156f, 3779.1677246094f, 3789.6254882812f, 
3800.0903320312f, 3810.5625000000f, 3821.0419921875f, 3831.5285644531f, 
3842.0222167969f, 3852.5231933594f, 3863.0312500000f, 3873.5463867188f, 
3884.0688476562f, 3894.5981445312f, 3905.1347656250f, 3915.6787109375f, 
3926.2294921875f, 3936.7873535156f, 3947.3522949219f, 3957.9245605469f, 
3968.5036621094f, 3979.0898437500f, 3989.6831054688f, 4000.2834472656f, 
4010.8906250000f, 4021.5048828125f, 4032.1262207031f, 4042.7546386719f, 
4053.3898925781f, 4064.0322265625f, 4074.6816406250f, 4085.3378906250f, 
4096.0009765625f, 4106.6713867188f, 4117.3481445312f, 4128.0322265625f, 
4138.7231445312f, 4149.4208984375f, 4160.1254882812f, 4170.8374023438f, 
4181.5556640625f, 4192.2812500000f, 4203.0136718750f, 4213.7524414062f, 
4224.4985351562f, 4235.2514648438f, 4246.0107421875f, 4256.7773437500f, 
4267.5502929688f, 4278.3305664062f, 4289.1171875000f, 4299.9111328125f, 
4310.7114257812f, 4321.5185546875f, 4332.3325195312f, 4343.1533203125f, 
4353.9804687500f, 4364.8149414062f, 4375.6557617188f, 4386.5034179688f, 
4397.3574218750f, 4408.2187500000f, 4419.0864257812f, 4429.9609375000f, 
4440.8417968750f, 4451.7294921875f, 4462.6240234375f, 4473.5249023438f, 
4484.4326171875f, 4495.3471679688f, 4506.2680664062f, 4517.1958007812f, 
4528.1298828125f, 4539.0708007812f, 4550.0180664062f, 4560.9721679688f, 
4571.9326171875f, 4582.8999023438f, 4593.8735351562f, 4604.8540039062f, 
4615.8408203125f, 4626.8339843750f, 4637.8339843750f, 4648.8403320312f, 
4659.8535156250f, 4670.8725585938f, 4681.8989257812f, 4692.9311523438f, 
4703.9702148438f, 4715.0156250000f, 4726.0673828125f, 4737.1259765625f, 
4748.1904296875f, 4759.2617187500f, 4770.3398437500f, 4781.4238281250f, 
4792.5141601562f, 4803.6113281250f, 4814.7148437500f, 4825.8247070312f, 
4836.9409179688f, 4848.0634765625f, 4859.1923828125f, 4870.3276367188f, 
4881.4692382812f, 4892.6176757812f, 4903.7719726562f, 4914.9326171875f, 
4926.1000976562f, 4937.2734375000f, 4948.4531250000f, 4959.6391601562f, 
4970.8315429688f, 4982.0302734375f, 4993.2353515625f, 5004.4467773438f, 
5015.6640625000f, 5026.8881835938f, 5038.1181640625f, 5049.3544921875f, 
5060.5971679688f, 5071.8461914062f, 5083.1010742188f, 5094.3627929688f, 
5105.6303710938f, 5116.9042968750f, 5128.1840820312f, 5139.4702148438f, 
5150.7626953125f, 5162.0615234375f, 5173.3662109375f, 5184.6772460938f, 
5195.9946289062f, 5207.3178710938f, 5218.6469726562f, 5229.9829101562f, 
5241.3247070312f, 5252.6723632812f, 5264.0268554688f, 5275.3867187500f, 
5286.7529296875f, 5298.1254882812f, 5309.5039062500f, 5320.8886718750f, 
5332.2792968750f, 5343.6762695312f, 5355.0791015625f, 5366.4882812500f, 
5377.9028320312f, 5389.3242187500f, 5400.7514648438f, 5412.1845703125f, 
5423.6235351562f, 5435.0688476562f, 5446.5200195312f, 5457.9775390625f, 
5469.4409179688f, 5480.9101562500f, 5492.3857421875f, 5503.8666992188f, 
5515.3540039062f, 5526.8476562500f, 5538.3466796875f, 5549.8520507812f, 
5561.3632812500f, 5572.8803710938f, 5584.4038085938f, 5595.9326171875f, 
5607.4677734375f, 5619.0087890625f, 5630.5556640625f, 5642.1083984375f, 
5653.6669921875f, 5665.2319335938f, 5676.8022460938f, 5688.3789062500f, 
5699.9609375000f, 5711.5493164062f, 5723.1435546875f, 5734.7436523438f, 
5746.3491210938f, 5757.9609375000f, 5769.5786132812f, 5781.2021484375f, 
5792.8315429688f, 5804.4663085938f, 5816.1074218750f, 5827.7543945312f, 
5839.4067382812f, 5851.0649414062f, 5862.7294921875f, 5874.3994140625f, 
5886.0751953125f, 5897.7568359375f, 5909.4443359375f, 5921.1376953125f, 
5932.8364257812f, 5944.5410156250f, 5956.2514648438f, 5967.9677734375f, 
5979.6899414062f, 5991.4174804688f, 6003.1513671875f, 6014.8906250000f, 
6026.6352539062f, 6038.3862304688f, 6050.1425781250f, 6061.9047851562f, 
6073.6723632812f, 6085.4458007812f, 6097.2250976562f, 6109.0102539062f, 
6120.8007812500f, 6132.5971679688f, 6144.3989257812f, 6156.2065429688f, 
6168.0200195312f, 6179.8388671875f, 6191.6635742188f, 6203.4936523438f, 
6215.3295898438f, 6227.1713867188f, 6239.0185546875f, 6250.8710937500f, 
6262.7294921875f, 6274.5937500000f, 6286.4633789062f, 6298.3383789062f, 
6310.2192382812f, 6322.1059570312f, 6333.9980468750f, 6345.8955078125f, 
6357.7988281250f, 6369.7075195312f, 6381.6215820312f, 6393.5415039062f, 
6405.4672851562f, 6417.3984375000f, 6429.3349609375f, 6441.2768554688f, 
6453.2246093750f, 6465.1777343750f, 6477.1362304688f, 6489.1005859375f, 
6501.0703125000f, 6513.0454101562f, 6525.0263671875f, 6537.0126953125f, 
6549.0043945312f, 6561.0019531250f, 6573.0043945312f, 6585.0126953125f, 
6597.0263671875f, 6609.0454101562f, 6621.0703125000f, 6633.1000976562f, 
6645.1357421875f, 6657.1767578125f, 6669.2231445312f, 6681.2753906250f, 
6693.3325195312f, 6705.3955078125f, 6717.4633789062f, 6729.5371093750f, 
6741.6162109375f, 6753.7006835938f, 6765.7905273438f, 6777.8857421875f, 
6789.9863281250f, 6802.0927734375f, 6814.2041015625f, 6826.3208007812f, 
6838.4428710938f, 6850.5708007812f, 6862.7036132812f, 6874.8417968750f, 
6886.9858398438f, 6899.1347656250f, 6911.2890625000f, 6923.4487304688f, 
6935.6137695312f, 6947.7841796875f, 6959.9599609375f, 6972.1411132812f, 
6984.3276367188f, 6996.5190429688f, 7008.7163085938f, 7020.9184570312f, 
7033.1259765625f, 7045.3388671875f, 7057.5571289062f, 7069.7807617188f, 
7082.0097656250f, 7094.2436523438f, 7106.4829101562f, 7118.7275390625f, 
7130.9775390625f, 7143.2329101562f, 7155.4931640625f, 7167.7587890625f, 
7180.0297851562f, 7192.3061523438f, 7204.5874023438f, 7216.8740234375f, 
7229.1660156250f, 7241.4628906250f, 7253.7656250000f, 7266.0732421875f, 
7278.3857421875f, 7290.7036132812f, 7303.0268554688f, 7315.3554687500f, 
7327.6889648438f, 7340.0278320312f, 7352.3715820312f, 7364.7207031250f, 
7377.0751953125f, 7389.4345703125f, 7401.7993164062f, 7414.1689453125f, 
7426.5439453125f, 7438.9243164062f, 7451.3095703125f, 7463.7001953125f, 
7476.0957031250f, 7488.4965820312f, 7500.9023437500f, 7513.3134765625f, 
7525.7294921875f, 7538.1503906250f, 7550.5771484375f, 7563.0083007812f, 
7575.4453125000f, 7587.8867187500f, 7600.3334960938f, 7612.7856445312f, 
7625.2426757812f, 7637.7045898438f, 7650.1718750000f, 7662.6440429688f, 
7675.1215820312f, 7687.6040039062f, 7700.0913085938f, 7712.5839843750f, 
7725.0815429688f, 7737.5839843750f, 7750.0917968750f, 7762.6044921875f, 
7775.1225585938f, 7787.6450195312f, 7800.1728515625f, 7812.7060546875f, 
7825.2441406250f, 7837.7871093750f, 7850.3349609375f, 7862.8876953125f, 
7875.4458007812f, 7888.0087890625f, 7900.5771484375f, 7913.1499023438f, 
7925.7280273438f, 7938.3110351562f, 7950.8989257812f, 7963.4921875000f, 
7976.0898437500f, 7988.6928710938f, 8001.3007812500f, 8013.9135742188f, 
8026.5317382812f, 8039.1542968750f, 8051.7822265625f, 8064.4150390625f, 
8077.0527343750f, 8089.6953125000f, 8102.3427734375f, 8114.9951171875f, 
8127.6528320312f, 8140.3149414062f, 8152.9824218750f, 8165.6542968750f, 
8178.3315429688f, 8191.0136718750f, 8203.7001953125f, 8216.3925781250f, 
8229.0888671875f, 8241.7910156250f, 8254.4970703125f, 8267.2089843750f, 
8279.9248046875f, 8292.6464843750f, 8305.3730468750f, 8318.1035156250f, 
8330.8398437500f, 8343.5800781250f, 8356.3261718750f, 8369.0761718750f, 
8381.8310546875f, 8394.5917968750f, 8407.3564453125f, 8420.1269531250f, 
8432.9013671875f, 8445.6806640625f, 8458.4648437500f, 8471.2539062500f, 
8484.0488281250f, 8496.8476562500f, 8509.6513671875f, 8522.4589843750f, 
8535.2724609375f, 8548.0908203125f, 8560.9140625000f, 8573.7412109375f, 
8586.5742187500f, 8599.4111328125f, 8612.2539062500f, 8625.1005859375f, 
8637.9521484375f, 8650.8085937500f, 8663.6699218750f, 8676.5361328125f, 
8689.4072265625f, 8702.2822265625f, 8715.1630859375f, 8728.0478515625f, 
8740.9375000000f, 8753.8320312500f, 8766.7314453125f, 8779.6357421875f, 
8792.5449218750f, 8805.4580078125f, 8818.3769531250f, 8831.2998046875f, 
8844.2275390625f, 8857.1601562500f, 8870.0976562500f, 8883.0390625000f, 
8895.9853515625f, 8908.9375000000f, 8921.8935546875f, 8934.8544921875f, 
8947.8193359375f, 8960.7900390625f, 8973.7646484375f, 8986.7441406250f, 
8999.7285156250f, 9012.7177734375f, 9025.7109375000f, 9038.7099609375f, 
9051.7128906250f, 9064.7197265625f, 9077.7324218750f, 9090.7500000000f, 
9103.7714843750f, 9116.7978515625f, 9129.8281250000f, 9142.8642578125f, 
9155.9042968750f, 9168.9492187500f, 9181.9990234375f, 9195.0527343750f, 
9208.1123046875f, 9221.1757812500f, 9234.2431640625f, 9247.3164062500f, 
9260.3935546875f, 9273.4755859375f, 9286.5625000000f, 9299.6533203125f, 
9312.7490234375f, 9325.8496093750f, 9338.9541015625f, 9352.0644531250f, 
9365.1787109375f, 9378.2968750000f, 9391.4208984375f, 9404.5488281250f, 
9417.6806640625f, 9430.8183593750f, 9443.9599609375f, 9457.1064453125f, 
9470.2568359375f, 9483.4121093750f, 9496.5722656250f, 9509.7373046875f, 
9522.9062500000f, 9536.0800781250f, 9549.2578125000f, 9562.4404296875f, 
9575.6279296875f, 9588.8193359375f, 9602.0166015625f, 9615.2167968750f, 
9628.4228515625f, 9641.6328125000f, 9654.8466796875f, 9668.0664062500f, 
9681.2890625000f, 9694.5175781250f, 9707.7500000000f, 9720.9873046875f, 
9734.2285156250f, 9747.4746093750f, 9760.7255859375f, 9773.9804687500f, 
9787.2402343750f, 9800.5039062500f, 9813.7724609375f, 9827.0458984375f, 
9840.3232421875f, 9853.6054687500f, 9866.8916015625f, 9880.1826171875f, 
9893.4785156250f, 9906.7783203125f, 9920.0830078125f, 9933.3916015625f, 
9946.7050781250f, 9960.0224609375f, 9973.3447265625f, 9986.6718750000f, 
10000.0029296875f, 10013.3378906250f, 10026.6787109375f, 10040.0224609375f, 
10053.3720703125f, 10066.7246093750f, 10080.0830078125f, 10093.4453125000f, 
10106.8115234375f, 10120.1826171875f, 10133.5576171875f, 10146.9375000000f, 
10160.3222656250f, 10173.7109375000f, 10187.1035156250f, 10200.5009765625f, 
10213.9033203125f, 10227.3095703125f, 10240.7197265625f, 10254.1347656250f, 
10267.5546875000f, 10280.9785156250f, 10294.4062500000f, 10307.8388671875f, 
10321.2763671875f, 10334.7177734375f, 10348.1630859375f, 10361.6132812500f, 
10375.0673828125f, 10388.5263671875f, 10401.9892578125f, 10415.4570312500f, 
10428.9287109375f, 10442.4052734375f, 10455.8857421875f, 10469.3710937500f, 
10482.8603515625f, 10496.3535156250f, 10509.8515625000f, 10523.3544921875f, 
10536.8603515625f, 10550.3720703125f, 10563.8867187500f, 10577.4062500000f, 
10590.9306640625f, 10604.4589843750f, 10617.9912109375f, 10631.5283203125f, 
10645.0693359375f, 10658.6152343750f, 10672.1650390625f, 10685.7187500000f, 
10699.2773437500f, 10712.8398437500f, 10726.4072265625f, 10739.9785156250f, 
10753.5537109375f, 10767.1337890625f, 10780.7177734375f, 10794.3066406250f, 
10807.8984375000f, 10821.4960937500f, 10835.0966796875f, 10848.7031250000f, 
10862.3125000000f, 10875.9267578125f, 10889.5449218750f, 10903.1669921875f, 
10916.7939453125f, 10930.4257812500f, 10944.0605468750f, 10957.7001953125f, 
10971.3437500000f, 10984.9921875000f, 10998.6445312500f, 11012.3007812500f, 
11025.9619140625f, 11039.6269531250f, 11053.2958984375f, 11066.9697265625f, 
11080.6474609375f, 11094.3291015625f, 11108.0156250000f, 11121.7060546875f, 
11135.4003906250f, 11149.0986328125f, 11162.8017578125f, 11176.5087890625f, 
11190.2207031250f, 11203.9365234375f, 11217.6562500000f, 11231.3798828125f, 
11245.1083984375f, 11258.8408203125f, 11272.5771484375f, 11286.3183593750f, 
11300.0625000000f, 11313.8115234375f, 11327.5654296875f, 11341.3232421875f, 
11355.0839843750f, 11368.8505859375f, 11382.6201171875f, 11396.3945312500f, 
11410.1728515625f, 11423.9550781250f, 11437.7421875000f, 11451.5322265625f, 
11465.3271484375f, 11479.1269531250f, 11492.9296875000f, 11506.7373046875f, 
11520.5488281250f, 11534.3642578125f, 11548.1845703125f, 11562.0087890625f, 
11575.8359375000f, 11589.6689453125f, 11603.5048828125f, 11617.3457031250f, 
11631.1904296875f, 11645.0390625000f, 11658.8916015625f, 11672.7480468750f, 
11686.6093750000f, 11700.4746093750f, 11714.3437500000f, 11728.2177734375f, 
11742.0947265625f, 11755.9765625000f, 11769.8623046875f, 11783.7519531250f, 
11797.6455078125f, 11811.5439453125f, 11825.4462890625f, 11839.3525390625f, 
11853.2626953125f, 11867.1767578125f, 11881.0947265625f, 11895.0175781250f, 
11908.9443359375f, 11922.8750000000f, 11936.8095703125f, 11950.7480468750f, 
11964.6914062500f, 11978.6376953125f, 11992.5888671875f, 12006.5439453125f, 
12020.5029296875f, 12034.4658203125f, 12048.4335937500f, 12062.4042968750f, 
12076.3798828125f, 12090.3593750000f, 12104.3427734375f, 12118.3300781250f, 
12132.3212890625f, 12146.3164062500f, 12160.3164062500f, 12174.3193359375f, 
12188.3271484375f, 12202.3388671875f, 12216.3544921875f, 12230.3740234375f, 
12244.3974609375f, 12258.4257812500f, 12272.4570312500f, 12286.4931640625f, 
12300.5322265625f, 12314.5761718750f, 12328.6240234375f, 12342.6757812500f, 
12356.7314453125f, 12370.7910156250f, 12384.8544921875f, 12398.9228515625f, 
12412.9941406250f, 12427.0703125000f, 12441.1494140625f, 12455.2333984375f, 
12469.3212890625f, 12483.4121093750f, 12497.5078125000f, 12511.6074218750f, 
12525.7109375000f, 12539.8183593750f, 12553.9296875000f, 12568.0458984375f, 
12582.1650390625f, 12596.2880859375f, 12610.4160156250f, 12624.5468750000f, 
12638.6826171875f, 12652.8212890625f, 12666.9648437500f, 12681.1113281250f, 
12695.2626953125f, 12709.4179687500f, 12723.5771484375f, 12737.7392578125f, 
12751.9062500000f, 12766.0771484375f, 12780.2519531250f, 12794.4306640625f, 
12808.6132812500f, 12822.7998046875f, 12836.9902343750f, 12851.1845703125f, 
12865.3828125000f, 12879.5849609375f, 12893.7910156250f, 12908.0009765625f, 
12922.2148437500f, 12936.4326171875f, 12950.6542968750f, 12964.8798828125f, 
12979.1093750000f, 12993.3427734375f, 13007.5800781250f, 13021.8212890625f, 
13036.0664062500f, 13050.3154296875f, 13064.5683593750f, 13078.8251953125f, 
13093.0859375000f, 13107.3505859375f, 13121.6191406250f, 13135.8906250000f, 
13150.1669921875f, 13164.4472656250f, 13178.7314453125f, 13193.0195312500f, 
13207.3105468750f, 13221.6064453125f, 13235.9062500000f, 13250.2089843750f, 
13264.5166015625f, 13278.8271484375f, 13293.1425781250f, 13307.4609375000f, 
13321.7832031250f, 13336.1103515625f, 13350.4404296875f, 13364.7744140625f, 
13379.1123046875f, 13393.4541015625f, 13407.7998046875f, 13422.1494140625f, 
13436.5029296875f, 13450.8593750000f, 13465.2207031250f, 13479.5849609375f, 
13493.9541015625f, 13508.3261718750f, 13522.7031250000f, 13537.0830078125f, 
13551.4667968750f, 13565.8544921875f, 13580.2460937500f, 13594.6416015625f, 
13609.0410156250f, 13623.4433593750f, 13637.8505859375f, 13652.2617187500f, 
13666.6757812500f, 13681.0937500000f, 13695.5156250000f, 13709.9414062500f, 
13724.3710937500f, 13738.8046875000f, 13753.2421875000f, 13767.6826171875f, 
13782.1279296875f, 13796.5761718750f, 13811.0283203125f, 13825.4843750000f, 
13839.9443359375f, 13854.4082031250f, 13868.8759765625f, 13883.3466796875f, 
13897.8222656250f, 13912.3007812500f, 13926.7832031250f, 13941.2695312500f, 
13955.7597656250f, 13970.2539062500f, 13984.7509765625f, 13999.2529296875f, 
14013.7578125000f, 14028.2666015625f, 14042.7792968750f, 14057.2958984375f, 
14071.8154296875f, 14086.3398437500f, 14100.8671875000f, 14115.3984375000f, 
14129.9335937500f, 14144.4726562500f, 14159.0156250000f, 14173.5615234375f, 
14188.1113281250f, 14202.6650390625f, 14217.2226562500f, 14231.7841796875f, 
14246.3486328125f, 14260.9179687500f, 14275.4902343750f, 14290.0664062500f, 
14304.6464843750f, 14319.2294921875f, 14333.8164062500f, 14348.4082031250f, 
14363.0029296875f, 14377.6005859375f, 14392.2031250000f, 14406.8085937500f, 
14421.4179687500f, 14436.0312500000f, 14450.6484375000f, 14465.2695312500f, 
14479.8935546875f, 14494.5214843750f, 14509.1533203125f, 14523.7880859375f, 
14538.4277343750f, 14553.0703125000f, 14567.7167968750f, 14582.3671875000f, 
14597.0205078125f, 14611.6777343750f, 14626.3388671875f, 14641.0039062500f, 
14655.6728515625f, 14670.3447265625f, 14685.0205078125f, 14699.7001953125f, 
14714.3837890625f, 14729.0703125000f, 14743.7607421875f, 14758.4550781250f, 
14773.1523437500f, 14787.8544921875f, 14802.5595703125f, 14817.2685546875f, 
14831.9804687500f, 14846.6962890625f, 14861.4160156250f, 14876.1396484375f, 
14890.8671875000f, 14905.5976562500f, 14920.3320312500f, 14935.0693359375f, 
14949.8115234375f, 14964.5566406250f, 14979.3056640625f, 14994.0576171875f, 
15008.8144531250f, 15023.5742187500f, 15038.3369140625f, 15053.1044921875f, 
15067.8750000000f, 15082.6494140625f, 15097.4267578125f, 15112.2080078125f, 
15126.9931640625f, 15141.7822265625f, 15156.5742187500f, 15171.3701171875f, 
15186.1699218750f, 15200.9726562500f, 15215.7792968750f, 15230.5898437500f, 
15245.4042968750f, 15260.2216796875f, 15275.0429687500f, 15289.8671875000f, 
15304.6962890625f, 15319.5273437500f, 15334.3632812500f, 15349.2021484375f, 
15364.0449218750f, 15378.8916015625f, 15393.7412109375f, 15408.5947265625f, 
15423.4521484375f, 15438.3125000000f, 15453.1767578125f, 15468.0439453125f, 
15482.9160156250f, 15497.7900390625f, 15512.6689453125f, 15527.5507812500f, 
15542.4365234375f, 15557.3261718750f, 15572.2187500000f, 15587.1152343750f, 
15602.0146484375f, 15616.9179687500f, 15631.8251953125f, 15646.7353515625f, 
15661.6494140625f, 15676.5673828125f, 15691.4882812500f, 15706.4130859375f, 
15721.3417968750f, 15736.2734375000f, 15751.2089843750f, 15766.1474609375f, 
15781.0898437500f, 15796.0361328125f, 15810.9853515625f, 15825.9384765625f, 
15840.8955078125f, 15855.8554687500f, 15870.8193359375f, 15885.7861328125f, 
15900.7568359375f, 15915.7314453125f, 15930.7089843750f, 15945.6904296875f, 
15960.6748046875f, 15975.6630859375f, 15990.6552734375f, 16005.6503906250f, 
16020.6494140625f, 16035.6513671875f, 16050.6572265625f, 16065.6669921875f, 
16080.6796875000f, 16095.6962890625f, 16110.7158203125f, 16125.7392578125f, 
16140.7666015625f, 16155.7968750000f, 16170.8310546875f, 16185.8681640625f, 
16200.9091796875f, 16215.9531250000f, 16231.0009765625f, 16246.0527343750f, 
16261.1074218750f, 16276.1660156250f, 16291.2275390625f, 16306.2929687500f, 
16321.3613281250f, 16336.4335937500f, 16351.5097656250f, 16366.5888671875f, 
16381.6708984375f, 16396.7578125000f, 16411.8476562500f, 16426.9394531250f, 
16442.0371093750f, 16457.1367187500f, 16472.2402343750f, 16487.3476562500f, 
16502.4570312500f, 16517.5722656250f, 16532.6894531250f, 16547.8105468750f, 
16562.9335937500f, 16578.0605468750f, 16593.1933593750f, 16608.3281250000f, 
16623.4648437500f, 16638.6074218750f, 16653.7519531250f, 16668.9003906250f, 
16684.0527343750f, 16699.2070312500f, 16714.3652343750f, 16729.5273437500f, 
16744.6933593750f, 16759.8632812500f, 16775.0351562500f, 16790.2109375000f, 
16805.3906250000f, 16820.5722656250f, 16835.7597656250f, 16850.9492187500f, 
16866.1425781250f, 16881.3378906250f, 16896.5371093750f, 16911.7421875000f, 
16926.9472656250f, 16942.1582031250f, 16957.3710937500f, 16972.5878906250f, 
16987.8085937500f, 17003.0332031250f, 17018.2597656250f, 17033.4902343750f, 
17048.7246093750f, 17063.9609375000f, 17079.2031250000f, 17094.4472656250f, 
17109.6933593750f, 17124.9453125000f, 17140.1992187500f, 17155.4570312500f, 
17170.7187500000f, 17185.9824218750f, 17201.2519531250f, 17216.5234375000f, 
17231.7968750000f, 17247.0761718750f, 17262.3574218750f, 17277.6425781250f, 
17292.9296875000f, 17308.2207031250f, 17323.5156250000f, 17338.8144531250f, 
17354.1171875000f, 17369.4218750000f, 17384.7304687500f, 17400.0429687500f, 
17415.3574218750f, 17430.6757812500f, 17445.9980468750f, 17461.3242187500f, 
17476.6523437500f, 17491.9843750000f, 17507.3203125000f, 17522.6582031250f, 
17538.0000000000f, 17553.3457031250f, 17568.6953125000f, 17584.0468750000f, 
17599.4023437500f, 17614.7617187500f, 17630.1250000000f, 17645.4902343750f, 
17660.8593750000f, 17676.2304687500f, 17691.6074218750f, 17706.9863281250f, 
17722.3671875000f, 17737.7539062500f, 17753.1425781250f, 17768.5351562500f, 
17783.9296875000f, 17799.3300781250f, 17814.7324218750f, 17830.1367187500f, 
17845.5468750000f, 17860.9589843750f, 17876.3750000000f, 17891.7929687500f, 
17907.2167968750f, 17922.6406250000f, 17938.0703125000f, 17953.5019531250f, 
17968.9375000000f, 17984.3769531250f, 17999.8183593750f, 18015.2656250000f, 
18030.7128906250f, 18046.1660156250f, 18061.6210937500f, 18077.0800781250f, 
18092.5429687500f, 18108.0078125000f, 18123.4765625000f, 18138.9472656250f, 
18154.4238281250f, 18169.9023437500f, 18185.3828125000f, 18200.8691406250f, 
18216.3574218750f, 18231.8496093750f, 18247.3437500000f, 18262.8417968750f, 
18278.3437500000f, 18293.8496093750f, 18309.3574218750f, 18324.8691406250f, 
18340.3828125000f, 18355.9023437500f, 18371.4218750000f, 18386.9472656250f, 
18402.4746093750f, 18418.0058593750f, 18433.5410156250f, 18449.0781250000f, 
18464.6191406250f, 18480.1640625000f, 18495.7109375000f, 18511.2617187500f, 
18526.8164062500f, 18542.3730468750f, 18557.9335937500f, 18573.4980468750f, 
18589.0644531250f, 18604.6347656250f, 18620.2089843750f, 18635.7851562500f, 
18651.3652343750f, 18666.9492187500f, 18682.5351562500f, 18698.1250000000f, 
18713.7187500000f, 18729.3144531250f, 18744.9160156250f, 18760.5175781250f, 
18776.1250000000f, 18791.7343750000f, 18807.3457031250f, 18822.9609375000f, 
18838.5800781250f, 18854.2031250000f, 18869.8281250000f, 18885.4570312500f, 
18901.0898437500f, 18916.7246093750f, 18932.3632812500f, 18948.0058593750f, 
18963.6503906250f, 18979.2988281250f, 18994.9492187500f, 19010.6035156250f, 
19026.2617187500f, 19041.9238281250f, 19057.5878906250f, 19073.2558593750f, 
19088.9257812500f, 19104.5996093750f, 19120.2773437500f, 19135.9570312500f, 
19151.6406250000f, 19167.3281250000f, 19183.0175781250f, 19198.7109375000f, 
19214.4082031250f, 19230.1074218750f, 19245.8105468750f, 19261.5156250000f, 
19277.2246093750f, 19292.9375000000f, 19308.6542968750f, 19324.3730468750f, 
19340.0937500000f, 19355.8203125000f, 19371.5488281250f, 19387.2792968750f, 
19403.0156250000f, 19418.7519531250f, 19434.4941406250f, 19450.2382812500f, 
19465.9863281250f, 19481.7363281250f, 19497.4902343750f, 19513.2480468750f, 
19529.0078125000f, 19544.7714843750f, 19560.5390625000f, 19576.3085937500f, 
19592.0820312500f, 19607.8574218750f, 19623.6367187500f, 19639.4199218750f, 
19655.2050781250f, 19670.9941406250f, 19686.7851562500f, 19702.5820312500f, 
19718.3789062500f, 19734.1816406250f, 19749.9863281250f, 19765.7929687500f, 
19781.6054687500f, 19797.4199218750f, 19813.2363281250f, 19829.0566406250f, 
19844.8808593750f, 19860.7070312500f, 19876.5371093750f, 19892.3710937500f, 
19908.2070312500f, 19924.0468750000f, 19939.8886718750f, 19955.7343750000f, 
19971.5839843750f, 19987.4355468750f, 20003.2910156250f, 20019.1484375000f, 
20035.0097656250f, 20050.8750000000f, 20066.7421875000f, 20082.6132812500f, 
20098.4882812500f, 20114.3652343750f, 20130.2460937500f, 20146.1289062500f, 
20162.0156250000f, 20177.9042968750f, 20193.7968750000f, 20209.6933593750f, 
20225.5937500000f, 20241.4941406250f, 20257.4003906250f, 20273.3085937500f, 
20289.2207031250f, 20305.1347656250f, 20321.0527343750f, 20336.9746093750f, 
20352.8984375000f, 20368.8242187500f, 20384.7558593750f, 20400.6875000000f, 
20416.6250000000f, 20432.5644531250f, 20448.5078125000f, 20464.4531250000f, 
20480.4023437500f, 20496.3535156250f, 20512.3085937500f, 20528.2675781250f, 
20544.2285156250f, 20560.1933593750f, 20576.1601562500f, 20592.1308593750f, 
20608.1054687500f, 20624.0820312500f, 20640.0625000000f, 20656.0449218750f, 
20672.0312500000f, 20688.0195312500f, 20704.0117187500f, 20720.0078125000f, 
20736.0058593750f, 20752.0078125000f, 20768.0117187500f, 20784.0195312500f, 
20800.0312500000f, 20816.0449218750f, 20832.0625000000f, 20848.0820312500f, 
20864.1054687500f, 20880.1308593750f, 20896.1601562500f, 20912.1933593750f, 
20928.2285156250f, 20944.2656250000f, 20960.3085937500f, 20976.3535156250f, 
20992.4003906250f, 21008.4511718750f, 21024.5058593750f, 21040.5625000000f, 
21056.6210937500f, 21072.6855468750f, 21088.7519531250f, 21104.8203125000f, 
21120.8925781250f, 21136.9667968750f, 21153.0468750000f, 21169.1269531250f, 
21185.2109375000f, 21201.2988281250f, 21217.3906250000f, 21233.4843750000f, 
21249.5800781250f, 21265.6796875000f, 21281.7832031250f, 21297.8886718750f, 
21313.9980468750f, 21330.1093750000f, 21346.2246093750f, 21362.3417968750f, 
21378.4628906250f, 21394.5878906250f, 21410.7148437500f, 21426.8437500000f, 
21442.9765625000f, 21459.1132812500f, 21475.2519531250f, 21491.3945312500f, 
21507.5410156250f, 21523.6894531250f, 21539.8398437500f, 21555.9941406250f, 
21572.1523437500f, 21588.3125000000f, 21604.4746093750f, 21620.6425781250f, 
21636.8125000000f, 21652.9843750000f, 21669.1601562500f, 21685.3378906250f, 
21701.5195312500f, 21717.7050781250f, 21733.8925781250f, 21750.0820312500f, 
21766.2753906250f, 21782.4726562500f, 21798.6718750000f, 21814.8750000000f, 
21831.0800781250f, 21847.2890625000f, 21863.5019531250f, 21879.7167968750f, 
21895.9335937500f, 21912.1542968750f, 21928.3789062500f, 21944.6054687500f, 
21960.8339843750f, 21977.0664062500f, 21993.3027343750f, 22009.5410156250f, 
22025.7832031250f, 22042.0273437500f, 22058.2753906250f, 22074.5273437500f, 
22090.7792968750f, 22107.0371093750f, 22123.2968750000f, 22139.5585937500f, 
22155.8242187500f, 22172.0937500000f, 22188.3652343750f, 22204.6386718750f, 
22220.9179687500f, 22237.1972656250f, 22253.4804687500f, 22269.7675781250f, 
22286.0566406250f, 22302.3496093750f, 22318.6445312500f, 22334.9433593750f, 
22351.2441406250f, 22367.5488281250f, 22383.8574218750f, 22400.1660156250f, 
22416.4804687500f, 22432.7968750000f, 22449.1152343750f, 22465.4375000000f, 
22481.7636718750f, 22498.0917968750f, 22514.4218750000f, 22530.7558593750f, 
22547.0937500000f, 22563.4335937500f, 22579.7753906250f, 22596.1210937500f, 
22612.4707031250f, 22628.8222656250f, 22645.1777343750f, 22661.5351562500f, 
22677.8964843750f, 22694.2597656250f, 22710.6250000000f, 22726.9960937500f, 
22743.3671875000f, 22759.7421875000f, 22776.1210937500f, 22792.5019531250f, 
22808.8867187500f, 22825.2734375000f, 22841.6640625000f, 22858.0566406250f, 
22874.4531250000f, 22890.8515625000f, 22907.2539062500f, 22923.6582031250f, 
22940.0664062500f, 22956.4765625000f, 22972.8906250000f, 22989.3066406250f, 
23005.7265625000f, 23022.1484375000f, 23038.5742187500f, 23055.0019531250f, 
23071.4335937500f, 23087.8671875000f, 23104.3046875000f, 23120.7460937500f, 
23137.1875000000f, 23153.6347656250f, 23170.0820312500f, 23186.5332031250f, 
23202.9882812500f, 23219.4453125000f, 23235.9062500000f, 23252.3691406250f, 
23268.8359375000f, 23285.3046875000f, 23301.7773437500f, 23318.2519531250f, 
23334.7304687500f, 23351.2109375000f, 23367.6953125000f, 23384.1816406250f, 
23400.6699218750f, 23417.1640625000f, 23433.6582031250f, 23450.1562500000f, 
23466.6582031250f, 23483.1621093750f, 23499.6679687500f, 23516.1777343750f, 
23532.6914062500f, 23549.2070312500f, 23565.7246093750f, 23582.2460937500f, 
23598.7714843750f, 23615.2988281250f, 23631.8281250000f, 23648.3613281250f, 
23664.8964843750f, 23681.4355468750f, 23697.9785156250f, 23714.5214843750f, 
23731.0703125000f, 23747.6191406250f, 23764.1738281250f, 23780.7285156250f, 
23797.2890625000f, 23813.8496093750f, 23830.4140625000f, 23846.9824218750f, 
23863.5527343750f, 23880.1269531250f, 23896.7031250000f, 23913.2812500000f, 
23929.8632812500f, 23946.4492187500f, 23963.0351562500f, 23979.6269531250f, 
23996.2207031250f, 24012.8164062500f, 24029.4160156250f, 24046.0175781250f, 
24062.6230468750f, 24079.2304687500f, 24095.8417968750f, 24112.4550781250f, 
24129.0703125000f, 24145.6894531250f, 24162.3125000000f, 24178.9375000000f, 
24195.5644531250f, 24212.1953125000f, 24228.8300781250f, 24245.4648437500f, 
24262.1054687500f, 24278.7480468750f, 24295.3925781250f, 24312.0390625000f, 
24328.6914062500f, 24345.3437500000f, 24362.0000000000f, 24378.6601562500f, 
24395.3222656250f, 24411.9863281250f, 24428.6542968750f, 24445.3242187500f, 
24461.9980468750f, 24478.6738281250f, 24495.3535156250f, 24512.0351562500f, 
24528.7207031250f, 24545.4082031250f, 24562.0976562500f, 24578.7910156250f, 
24595.4882812500f, 24612.1875000000f, 24628.8886718750f, 24645.5937500000f, 
24662.3007812500f, 24679.0117187500f, 24695.7246093750f, 24712.4394531250f, 
24729.1582031250f, 24745.8808593750f, 24762.6054687500f, 24779.3320312500f, 
24796.0625000000f, 24812.7949218750f, 24829.5312500000f, 24846.2695312500f, 
24863.0117187500f, 24879.7558593750f, 24896.5019531250f, 24913.2519531250f, 
24930.0039062500f, 24946.7597656250f, 24963.5175781250f, 24980.2792968750f, 
24997.0429687500f, 25013.8105468750f, 25030.5800781250f, 25047.3515625000f, 
25064.1269531250f, 25080.9042968750f, 25097.6855468750f, 25114.4687500000f, 
25131.2558593750f, 25148.0449218750f, 25164.8359375000f, 25181.6308593750f, 
25198.4277343750f, 25215.2285156250f, 25232.0312500000f, 25248.8378906250f, 
25265.6464843750f, 25282.4589843750f, 25299.2734375000f, 25316.0898437500f, 
25332.9101562500f, 25349.7324218750f, 25366.5585937500f, 25383.3867187500f, 
25400.2167968750f, 25417.0507812500f, 25433.8886718750f, 25450.7265625000f, 
25467.5703125000f, 25484.4140625000f, 25501.2617187500f, 25518.1132812500f, 
25534.9667968750f, 25551.8222656250f, 25568.6816406250f, 25585.5429687500f, 
25602.4082031250f, 25619.2753906250f, 25636.1445312500f, 25653.0175781250f, 
25669.8925781250f, 25686.7714843750f, 25703.6523437500f, 25720.5371093750f, 
25737.4238281250f, 25754.3125000000f, 25771.2050781250f, 25788.0996093750f, 
25804.9980468750f, 25821.8984375000f, 25838.8027343750f, 25855.7070312500f, 
25872.6171875000f, 25889.5273437500f, 25906.4433593750f, 25923.3593750000f, 
25940.2792968750f, 25957.2031250000f, 25974.1269531250f, 25991.0566406250f, 
26007.9863281250f, 26024.9199218750f, 26041.8574218750f, 26058.7968750000f, 
26075.7382812500f, 26092.6816406250f, 26109.6308593750f, 26126.5800781250f, 
26143.5332031250f, 26160.4882812500f, 26177.4472656250f, 26194.4082031250f, 
26211.3730468750f, 26228.3398437500f, 26245.3085937500f, 26262.2812500000f, 
26279.2558593750f, 26296.2324218750f, 26313.2128906250f, 26330.1972656250f, 
26347.1816406250f, 26364.1718750000f, 26381.1621093750f, 26398.1562500000f, 
26415.1523437500f, 26432.1523437500f, 26449.1542968750f, 26466.1601562500f, 
26483.1679687500f, 26500.1777343750f, 26517.1914062500f, 26534.2070312500f, 
26551.2265625000f, 26568.2480468750f, 26585.2714843750f, 26602.2988281250f, 
26619.3281250000f, 26636.3593750000f, 26653.3945312500f, 26670.4335937500f, 
26687.4726562500f, 26704.5156250000f, 26721.5625000000f, 26738.6113281250f, 
26755.6621093750f, 26772.7167968750f, 26789.7734375000f, 26806.8320312500f, 
26823.8945312500f, 26840.9589843750f, 26858.0273437500f, 26875.0976562500f, 
26892.1699218750f, 26909.2460937500f, 26926.3242187500f, 26943.4062500000f, 
26960.4902343750f, 26977.5761718750f, 26994.6660156250f, 27011.7578125000f, 
27028.8515625000f, 27045.9492187500f, 27063.0507812500f, 27080.1523437500f, 
27097.2578125000f, 27114.3671875000f, 27131.4765625000f, 27148.5917968750f, 
27165.7070312500f, 27182.8261718750f, 27199.9472656250f, 27217.0722656250f, 
27234.1992187500f, 27251.3300781250f, 27268.4609375000f, 27285.5976562500f, 
27302.7343750000f, 27319.8750000000f, 27337.0175781250f, 27354.1640625000f, 
27371.3125000000f, 27388.4648437500f, 27405.6191406250f, 27422.7753906250f, 
27439.9335937500f, 27457.0957031250f, 27474.2617187500f, 27491.4277343750f, 
27508.5976562500f, 27525.7714843750f, 27542.9472656250f, 27560.1250000000f, 
27577.3046875000f, 27594.4882812500f, 27611.6757812500f, 27628.8632812500f, 
27646.0546875000f, 27663.2500000000f, 27680.4472656250f, 27697.6464843750f, 
27714.8476562500f, 27732.0527343750f, 27749.2597656250f, 27766.4707031250f, 
27783.6835937500f, 27800.8984375000f, 27818.1171875000f, 27835.3378906250f, 
27852.5605468750f, 27869.7871093750f, 27887.0156250000f, 27904.2480468750f, 
27921.4824218750f, 27938.7187500000f, 27955.9589843750f, 27973.2011718750f, 
27990.4453125000f, 28007.6933593750f, 28024.9433593750f, 28042.1953125000f, 
28059.4511718750f, 28076.7089843750f, 28093.9707031250f, 28111.2324218750f, 
28128.5000000000f, 28145.7675781250f, 28163.0390625000f, 28180.3125000000f, 
28197.5898437500f, 28214.8691406250f, 28232.1503906250f, 28249.4355468750f, 
28266.7226562500f, 28284.0117187500f, 28301.3046875000f, 28318.5996093750f, 
28335.8984375000f, 28353.1992187500f, 28370.5019531250f, 28387.8066406250f, 
28405.1152343750f, 28422.4257812500f, 28439.7402343750f, 28457.0566406250f, 
28474.3750000000f, 28491.6972656250f, 28509.0214843750f, 28526.3476562500f, 
28543.6757812500f, 28561.0078125000f, 28578.3437500000f, 28595.6816406250f, 
28613.0214843750f, 28630.3632812500f, 28647.7089843750f, 28665.0566406250f, 
28682.4062500000f, 28699.7597656250f, 28717.1152343750f, 28734.4726562500f, 
28751.8339843750f, 28769.1972656250f, 28786.5644531250f, 28803.9335937500f, 
28821.3046875000f, 28838.6777343750f, 28856.0546875000f, 28873.4335937500f, 
28890.8164062500f, 28908.2011718750f, 28925.5878906250f, 28942.9765625000f, 
28960.3691406250f, 28977.7636718750f, 28995.1621093750f, 29012.5625000000f, 
29029.9648437500f, 29047.3710937500f, 29064.7773437500f, 29082.1894531250f, 
29099.6015625000f, 29117.0175781250f, 29134.4355468750f, 29151.8574218750f, 
29169.2812500000f, 29186.7070312500f, 29204.1347656250f, 29221.5664062500f, 
29239.0019531250f, 29256.4375000000f, 29273.8769531250f, 29291.3183593750f, 
29308.7636718750f, 29326.2109375000f, 29343.6601562500f, 29361.1113281250f, 
29378.5664062500f, 29396.0234375000f, 29413.4843750000f, 29430.9472656250f, 
29448.4121093750f, 29465.8789062500f, 29483.3496093750f, 29500.8222656250f, 
29518.2988281250f, 29535.7753906250f, 29553.2578125000f, 29570.7402343750f, 
29588.2265625000f, 29605.7148437500f, 29623.2050781250f, 29640.6992187500f, 
29658.1953125000f, 29675.6933593750f, 29693.1953125000f, 29710.6992187500f, 
29728.2050781250f, 29745.7148437500f, 29763.2265625000f, 29780.7402343750f, 
29798.2578125000f, 29815.7773437500f, 29833.2988281250f, 29850.8222656250f, 
29868.3496093750f, 29885.8808593750f, 29903.4121093750f, 29920.9472656250f, 
29938.4843750000f, 29956.0234375000f, 29973.5664062500f, 29991.1113281250f, 
30008.6601562500f, 30026.2089843750f, 30043.7617187500f, 30061.3183593750f, 
30078.8750000000f, 30096.4355468750f, 30114.0000000000f, 30131.5644531250f, 
30149.1328125000f, 30166.7031250000f, 30184.2773437500f, 30201.8535156250f, 
30219.4316406250f, 30237.0117187500f, 30254.5957031250f, 30272.1816406250f, 
30289.7695312500f, 30307.3613281250f, 30324.9550781250f, 30342.5507812500f, 
30360.1503906250f, 30377.7519531250f, 30395.3554687500f, 30412.9609375000f, 
30430.5703125000f, 30448.1816406250f, 30465.7968750000f, 30483.4140625000f, 
30501.0332031250f, 30518.6542968750f, 30536.2792968750f, 30553.9042968750f, 
30571.5351562500f, 30589.1660156250f, 30606.8007812500f, 30624.4375000000f, 
30642.0781250000f, 30659.7187500000f, 30677.3632812500f, 30695.0117187500f, 
30712.6601562500f, 30730.3125000000f, 30747.9687500000f, 30765.6250000000f, 
30783.2851562500f, 30800.9472656250f, 30818.6132812500f, 30836.2792968750f, 
30853.9492187500f, 30871.6230468750f, 30889.2968750000f, 30906.9746093750f, 
30924.6542968750f, 30942.3378906250f, 30960.0234375000f, 30977.7109375000f, 
30995.4003906250f, 31013.0937500000f, 31030.7890625000f, 31048.4863281250f, 
31066.1855468750f, 31083.8886718750f, 31101.5937500000f, 31119.3027343750f, 
31137.0117187500f, 31154.7246093750f, 31172.4414062500f, 31190.1582031250f, 
31207.8789062500f, 31225.6015625000f, 31243.3281250000f, 31261.0546875000f, 
31278.7851562500f, 31296.5195312500f, 31314.2539062500f, 31331.9921875000f, 
31349.7324218750f, 31367.4765625000f, 31385.2226562500f, 31402.9707031250f, 
31420.7207031250f, 31438.4726562500f, 31456.2285156250f, 31473.9863281250f, 
31491.7480468750f, 31509.5117187500f, 31527.2773437500f, 31545.0449218750f, 
31562.8144531250f, 31580.5878906250f, 31598.3632812500f, 31616.1425781250f, 
31633.9218750000f, 31651.7050781250f, 31669.4921875000f, 31687.2792968750f, 
31705.0703125000f, 31722.8632812500f, 31740.6582031250f, 31758.4570312500f, 
31776.2578125000f, 31794.0605468750f, 31811.8652343750f, 31829.6738281250f, 
31847.4843750000f, 31865.2968750000f, 31883.1132812500f, 31900.9316406250f, 
31918.7519531250f, 31936.5742187500f, 31954.4003906250f, 31972.2285156250f, 
31990.0585937500f, 32007.8906250000f, 32025.7265625000f, 32043.5644531250f, 
32061.4042968750f, 32079.2480468750f, 32097.0937500000f, 32114.9414062500f, 
32132.7910156250f, 32150.6445312500f, 32168.5000000000f, 32186.3574218750f, 
32204.2167968750f, 32222.0800781250f, 32239.9453125000f, 32257.8125000000f, 
32275.6835937500f, 32293.5566406250f, 32311.4316406250f, 32329.3085937500f, 
32347.1875000000f, 32365.0703125000f, 32382.9550781250f, 32400.8437500000f, 
32418.7324218750f, 32436.6250000000f, 32454.5195312500f, 32472.4179687500f, 
32490.3183593750f, 32508.2207031250f, 32526.1250000000f, 32544.0312500000f, 
32561.9414062500f, 32579.8535156250f, 32597.7675781250f, 32615.6855468750f, 
32633.6035156250f, 32651.5253906250f, 32669.4511718750f, 32687.3769531250f, 
32705.3066406250f, 32723.2382812500f, 32741.1738281250f, 32759.1093750000f, 
32777.0468750000f, 32794.9921875000f, 32812.9335937500f, 32830.8789062500f, 
32848.8281250000f, 32866.7812500000f, 32884.7343750000f, 32902.6914062500f, 
32920.6484375000f, 32938.6132812500f, 32956.5742187500f, 32974.5429687500f, 
32992.5078125000f, 33010.4804687500f, 33028.4531250000f, 33046.4296875000f, 
33064.4101562500f, 33082.3906250000f, 33100.3710937500f, 33118.3593750000f, 
33136.3476562500f, 33154.3359375000f, 33172.3281250000f, 33190.3242187500f, 
33208.3242187500f, 33226.3242187500f, 33244.3242187500f, 33262.3320312500f, 
33280.3398437500f, 33298.3476562500f, 33316.3632812500f, 33334.3750000000f, 
33352.3945312500f, 33370.4140625000f, 33388.4375000000f, 33406.4609375000f, 
33424.4882812500f, 33442.5156250000f, 33460.5507812500f, 33478.5820312500f, 
33496.6210937500f, 33514.6601562500f, 33532.6992187500f, 33550.7460937500f, 
33568.7929687500f, 33586.8398437500f, 33604.8906250000f, 33622.9453125000f, 
33641.0039062500f, 33659.0625000000f, 33677.1210937500f, 33695.1835937500f, 
33713.2500000000f, 33731.3203125000f, 33749.3906250000f, 33767.4648437500f, 
33785.5390625000f, 33803.6171875000f, 33821.6992187500f, 33839.7812500000f, 
33857.8671875000f, 33875.9531250000f, 33894.0429687500f, 33912.1367187500f, 
33930.2304687500f, 33948.3281250000f, 33966.4296875000f, 33984.5312500000f, 
34002.6328125000f, 34020.7421875000f, 34038.8515625000f, 34056.9609375000f, 
34075.0781250000f, 34093.1953125000f, 34111.3125000000f, 34129.4335937500f, 
34147.5585937500f, 34165.6835937500f, 34183.8125000000f, 34201.9453125000f, 
34220.0781250000f, 34238.2148437500f, 34256.3515625000f, 34274.4921875000f, 
34292.6367187500f, 34310.7812500000f, 34328.9296875000f, 34347.0781250000f, 
34365.2304687500f, 34383.3867187500f, 34401.5429687500f, 34419.7031250000f, 
34437.8671875000f, 34456.0312500000f, 34474.1992187500f, 34492.3671875000f, 
34510.5390625000f, 34528.7109375000f, 34546.8906250000f, 34565.0703125000f, 
34583.2500000000f, 34601.4335937500f, 34619.6210937500f, 34637.8085937500f, 
34656.0000000000f, 34674.1914062500f, 34692.3867187500f, 34710.5859375000f, 
34728.7851562500f, 34746.9882812500f, 34765.1953125000f, 34783.4023437500f, 
34801.6132812500f, 34819.8242187500f, 34838.0390625000f, 34856.2578125000f, 
34874.4765625000f, 34892.6992187500f, 34910.9218750000f, 34929.1484375000f, 
34947.3789062500f, 34965.6093750000f, 34983.8437500000f, 35002.0781250000f, 
35020.3164062500f, 35038.5585937500f, 35056.8007812500f, 35075.0468750000f, 
35093.2968750000f, 35111.5468750000f, 35129.8007812500f, 35148.0546875000f, 
35166.3125000000f, 35184.5703125000f, 35202.8359375000f, 35221.0976562500f, 
35239.3671875000f, 35257.6367187500f, 35275.9062500000f, 35294.1796875000f, 
35312.4570312500f, 35330.7382812500f, 35349.0195312500f, 35367.3007812500f, 
35385.5859375000f, 35403.8750000000f, 35422.1679687500f, 35440.4609375000f, 
35458.7539062500f, 35477.0507812500f, 35495.3515625000f, 35513.6562500000f, 
35531.9609375000f, 35550.2656250000f, 35568.5781250000f, 35586.8867187500f, 
35605.2031250000f, 35623.5195312500f, 35641.8398437500f, 35660.1601562500f, 
35678.4843750000f, 35696.8085937500f, 35715.1367187500f, 35733.4687500000f, 
35751.8007812500f, 35770.1367187500f, 35788.4765625000f, 35806.8164062500f, 
35825.1562500000f, 35843.5039062500f, 35861.8476562500f, 35880.1992187500f, 
35898.5507812500f, 35916.9062500000f, 35935.2617187500f, 35953.6210937500f, 
35971.9804687500f, 35990.3437500000f, 36008.7109375000f, 36027.0781250000f, 
36045.4492187500f, 36063.8242187500f, 36082.1992187500f, 36100.5781250000f, 
36118.9570312500f, 36137.3398437500f, 36155.7226562500f, 36174.1093750000f, 
36192.5000000000f, 36210.8906250000f, 36229.2851562500f, 36247.6796875000f, 
36266.0820312500f, 36284.4804687500f, 36302.8828125000f, 36321.2890625000f, 
36339.6992187500f, 36358.1093750000f, 36376.5195312500f, 36394.9375000000f, 
36413.3515625000f, 36431.7734375000f, 36450.1953125000f, 36468.6210937500f, 
36487.0468750000f, 36505.4765625000f, 36523.9062500000f, 36542.3398437500f, 
36560.7773437500f, 36579.2148437500f, 36597.6562500000f, 36616.0976562500f, 
36634.5429687500f, 36652.9921875000f, 36671.4414062500f, 36689.8906250000f, 
36708.3476562500f, 36726.8046875000f, 36745.2617187500f, 36763.7226562500f, 
36782.1875000000f, 36800.6562500000f, 36819.1210937500f, 36837.5937500000f, 
36856.0664062500f, 36874.5429687500f, 36893.0195312500f, 36911.5000000000f, 
36929.9804687500f, 36948.4648437500f, 36966.9531250000f, 36985.4414062500f, 
37003.9335937500f, 37022.4296875000f, 37040.9257812500f, 37059.4218750000f, 
37077.9218750000f, 37096.4257812500f, 37114.9335937500f, 37133.4414062500f, 
37151.9492187500f, 37170.4609375000f, 37188.9765625000f, 37207.4921875000f, 
37226.0117187500f, 37244.5351562500f, 37263.0585937500f, 37281.5859375000f, 
37300.1132812500f, 37318.6445312500f, 37337.1757812500f, 37355.7109375000f, 
37374.2500000000f, 37392.7890625000f, 37411.3320312500f, 37429.8789062500f, 
37448.4257812500f, 37466.9726562500f, 37485.5234375000f, 37504.0781250000f, 
37522.6328125000f, 37541.1914062500f, 37559.7539062500f, 37578.3164062500f, 
37596.8828125000f, 37615.4492187500f, 37634.0195312500f, 37652.5898437500f, 
37671.1640625000f, 37689.7421875000f, 37708.3203125000f, 37726.9023437500f, 
37745.4843750000f, 37764.0703125000f, 37782.6601562500f, 37801.2500000000f, 
37819.8437500000f, 37838.4375000000f, 37857.0351562500f, 37875.6328125000f, 
37894.2343750000f, 37912.8398437500f, 37931.4453125000f, 37950.0546875000f, 
37968.6679687500f, 37987.2812500000f, 38005.8945312500f, 38024.5117187500f, 
38043.1328125000f, 38061.7539062500f, 38080.3789062500f, 38099.0078125000f, 
38117.6367187500f, 38136.2656250000f, 38154.9023437500f, 38173.5390625000f, 
38192.1757812500f, 38210.8164062500f, 38229.4570312500f, 38248.1054687500f, 
38266.7500000000f, 38285.4023437500f, 38304.0546875000f, 38322.7070312500f, 
38341.3632812500f, 38360.0234375000f, 38378.6835937500f, 38397.3476562500f, 
38416.0117187500f, 38434.6796875000f, 38453.3515625000f, 38472.0234375000f, 
38490.6953125000f, 38509.3750000000f, 38528.0546875000f, 38546.7343750000f, 
38565.4179687500f, 38584.1054687500f, 38602.7929687500f, 38621.4843750000f, 
38640.1757812500f, 38658.8710937500f, 38677.5664062500f, 38696.2656250000f, 
38714.9687500000f, 38733.6718750000f, 38752.3789062500f, 38771.0859375000f, 
38789.7968750000f, 38808.5117187500f, 38827.2265625000f, 38845.9453125000f, 
38864.6640625000f, 38883.3867187500f, 38902.1093750000f, 38920.8359375000f, 
38939.5664062500f, 38958.2968750000f, 38977.0312500000f, 38995.7656250000f, 
39014.5039062500f, 39033.2421875000f, 39051.9843750000f, 39070.7304687500f, 
39089.4765625000f, 39108.2265625000f, 39126.9765625000f, 39145.7304687500f, 
39164.4882812500f, 39183.2460937500f, 39202.0039062500f, 39220.7695312500f, 
39239.5312500000f, 39258.3007812500f, 39277.0703125000f, 39295.8398437500f, 
39314.6132812500f, 39333.3906250000f, 39352.1679687500f, 39370.9492187500f, 
39389.7304687500f, 39408.5156250000f, 39427.3046875000f, 39446.0937500000f, 
39464.8867187500f, 39483.6796875000f, 39502.4765625000f, 39521.2734375000f, 
39540.0742187500f, 39558.8789062500f, 39577.6835937500f, 39596.4882812500f, 
39615.3007812500f, 39634.1093750000f, 39652.9257812500f, 39671.7421875000f, 
39690.5585937500f, 39709.3789062500f, 39728.2031250000f, 39747.0273437500f, 
39765.8554687500f, 39784.6875000000f, 39803.5195312500f, 39822.3515625000f, 
39841.1875000000f, 39860.0273437500f, 39878.8671875000f, 39897.7109375000f, 
39916.5546875000f, 39935.4023437500f, 39954.2539062500f, 39973.1054687500f, 
39991.9570312500f, 40010.8164062500f, 40029.6718750000f, 40048.5351562500f, 
40067.3984375000f, 40086.2617187500f, 40105.1289062500f, 40124.0000000000f, 
40142.8710937500f, 40161.7460937500f, 40180.6210937500f, 40199.5000000000f, 
40218.3828125000f, 40237.2656250000f, 40256.1484375000f, 40275.0390625000f, 
40293.9257812500f, 40312.8203125000f, 40331.7109375000f, 40350.6093750000f, 
40369.5078125000f, 40388.4062500000f, 40407.3125000000f, 40426.2148437500f, 
40445.1250000000f, 40464.0312500000f, 40482.9453125000f, 40501.8593750000f, 
40520.7734375000f, 40539.6914062500f, 40558.6132812500f, 40577.5351562500f, 
40596.4609375000f, 40615.3906250000f, 40634.3164062500f, 40653.2500000000f, 
40672.1835937500f, 40691.1210937500f, 40710.0585937500f, 40729.0000000000f, 
40747.9414062500f, 40766.8867187500f, 40785.8320312500f, 40804.7812500000f, 
40823.7343750000f, 40842.6875000000f, 40861.6445312500f, 40880.6015625000f, 
40899.5625000000f, 40918.5234375000f, 40937.4882812500f, 40956.4531250000f, 
40975.4257812500f, 40994.3945312500f, 41013.3671875000f, 41032.3437500000f, 
41051.3203125000f, 41070.3007812500f, 41089.2851562500f, 41108.2695312500f, 
41127.2539062500f, 41146.2421875000f, 41165.2343750000f, 41184.2265625000f, 
41203.2226562500f, 41222.2187500000f, 41241.2187500000f, 41260.2226562500f, 
41279.2265625000f, 41298.2343750000f, 41317.2421875000f, 41336.2500000000f, 
41355.2656250000f, 41374.2812500000f, 41393.2968750000f, 41412.3164062500f, 
41431.3359375000f, 41450.3593750000f, 41469.3867187500f, 41488.4140625000f, 
41507.4453125000f, 41526.4765625000f, 41545.5117187500f, 41564.5507812500f, 
41583.5898437500f, 41602.6289062500f, 41621.6718750000f, 41640.7187500000f, 
41659.7656250000f, 41678.8164062500f, 41697.8671875000f, 41716.9218750000f, 
41735.9804687500f, 41755.0390625000f, 41774.0976562500f, 41793.1601562500f, 
41812.2265625000f, 41831.2929687500f, 41850.3632812500f, 41869.4335937500f, 
41888.5078125000f, 41907.5859375000f, 41926.6640625000f, 41945.7421875000f, 
41964.8242187500f, 41983.9101562500f, 42002.9960937500f, 42022.0859375000f, 
42041.1757812500f, 42060.2695312500f, 42079.3671875000f, 42098.4648437500f, 
42117.5625000000f, 42136.6679687500f, 42155.7695312500f, 42174.8750000000f, 
42193.9843750000f, 42213.0976562500f, 42232.2070312500f, 42251.3242187500f, 
42270.4414062500f, 42289.5585937500f, 42308.6796875000f, 42327.8046875000f, 
42346.9296875000f, 42366.0585937500f, 42385.1875000000f, 42404.3203125000f, 
42423.4570312500f, 42442.5937500000f, 42461.7304687500f, 42480.8710937500f, 
42500.0156250000f, 42519.1601562500f, 42538.3085937500f, 42557.4570312500f, 
42576.6093750000f, 42595.7617187500f, 42614.9179687500f, 42634.0781250000f, 
42653.2382812500f, 42672.3984375000f, 42691.5664062500f, 42710.7304687500f, 
42729.8984375000f, 42749.0703125000f, 42768.2460937500f, 42787.4179687500f, 
42806.5976562500f, 42825.7773437500f, 42844.9570312500f, 42864.1445312500f, 
42883.3281250000f, 42902.5156250000f, 42921.7070312500f, 42940.8984375000f, 
42960.0937500000f, 42979.2929687500f, 42998.4921875000f, 43017.6914062500f, 
43036.8945312500f, 43056.1015625000f, 43075.3085937500f, 43094.5156250000f, 
43113.7304687500f, 43132.9414062500f, 43152.1601562500f, 43171.3789062500f, 
43190.5976562500f, 43209.8203125000f, 43229.0468750000f, 43248.2734375000f, 
43267.5000000000f, 43286.7304687500f, 43305.9648437500f, 43325.1992187500f, 
43344.4375000000f, 43363.6757812500f, 43382.9179687500f, 43402.1640625000f, 
43421.4101562500f, 43440.6562500000f, 43459.9062500000f, 43479.1601562500f, 
43498.4140625000f, 43517.6718750000f, 43536.9296875000f, 43556.1914062500f, 
43575.4531250000f, 43594.7187500000f, 43613.9882812500f, 43633.2578125000f, 
43652.5273437500f, 43671.8007812500f, 43691.0781250000f, 43710.3554687500f, 
43729.6367187500f, 43748.9179687500f, 43768.2031250000f, 43787.4882812500f, 
43806.7773437500f, 43826.0664062500f, 43845.3593750000f, 43864.6562500000f, 
43883.9531250000f, 43903.2500000000f, 43922.5546875000f, 43941.8554687500f, 
43961.1601562500f, 43980.4687500000f, 43999.7773437500f, 44019.0898437500f, 
44038.4062500000f, 44057.7226562500f, 44077.0390625000f, 44096.3593750000f, 
44115.6835937500f, 44135.0078125000f, 44154.3320312500f, 44173.6640625000f, 
44192.9921875000f, 44212.3281250000f, 44231.6601562500f, 44251.0000000000f, 
44270.3398437500f, 44289.6796875000f, 44309.0234375000f, 44328.3710937500f, 
44347.7187500000f, 44367.0664062500f, 44386.4179687500f, 44405.7734375000f, 
44425.1289062500f, 44444.4882812500f, 44463.8476562500f, 44483.2109375000f, 
44502.5781250000f, 44521.9414062500f, 44541.3125000000f, 44560.6835937500f, 
44580.0546875000f, 44599.4296875000f, 44618.8085937500f, 44638.1875000000f, 
44657.5703125000f, 44676.9531250000f, 44696.3398437500f, 44715.7265625000f, 
44735.1171875000f, 44754.5078125000f, 44773.9023437500f, 44793.3007812500f, 
44812.6992187500f, 44832.0976562500f, 44851.5000000000f, 44870.9062500000f, 
44890.3125000000f, 44909.7226562500f, 44929.1328125000f, 44948.5468750000f, 
44967.9609375000f, 44987.3789062500f, 45006.7968750000f, 45026.2187500000f, 
45045.6406250000f, 45065.0664062500f, 45084.4960937500f, 45103.9257812500f, 
45123.3593750000f, 45142.7929687500f, 45162.2265625000f, 45181.6640625000f, 
45201.1054687500f, 45220.5468750000f, 45239.9921875000f, 45259.4414062500f, 
45278.8867187500f, 45298.3398437500f, 45317.7929687500f, 45337.2460937500f, 
45356.7031250000f, 45376.1640625000f, 45395.6250000000f, 45415.0859375000f, 
45434.5507812500f, 45454.0195312500f, 45473.4882812500f, 45492.9609375000f, 
45512.4335937500f, 45531.9101562500f, 45551.3867187500f, 45570.8671875000f, 
45590.3515625000f, 45609.8359375000f, 45629.3203125000f, 45648.8085937500f, 
45668.3007812500f, 45687.7929687500f, 45707.2851562500f, 45726.7812500000f, 
45746.2812500000f, 45765.7812500000f, 45785.2851562500f, 45804.7890625000f, 
45824.2968750000f, 45843.8046875000f, 45863.3164062500f, 45882.8320312500f, 
45902.3437500000f, 45921.8632812500f, 45941.3828125000f, 45960.9023437500f, 
45980.4257812500f, 45999.9531250000f, 46019.4804687500f, 46039.0117187500f, 
46058.5429687500f, 46078.0781250000f, 46097.6132812500f, 46117.1484375000f, 
46136.6914062500f, 46156.2343750000f, 46175.7773437500f, 46195.3242187500f, 
46214.8710937500f, 46234.4218750000f, 46253.9765625000f, 46273.5273437500f, 
46293.0859375000f, 46312.6445312500f, 46332.2070312500f, 46351.7695312500f, 
46371.3320312500f, 46390.8984375000f, 46410.4687500000f, 46430.0390625000f, 
46449.6132812500f, 46469.1875000000f, 46488.7656250000f, 46508.3437500000f, 
46527.9257812500f, 46547.5117187500f, 46567.0976562500f, 46586.6835937500f, 
46606.2734375000f, 46625.8632812500f, 46645.4570312500f, 46665.0546875000f, 
46684.6523437500f, 46704.2539062500f, 46723.8554687500f, 46743.4570312500f, 
46763.0664062500f, 46782.6718750000f, 46802.2812500000f, 46821.8945312500f, 
46841.5117187500f, 46861.1250000000f, 46880.7460937500f, 46900.3632812500f, 
46919.9882812500f, 46939.6132812500f, 46959.2382812500f, 46978.8671875000f, 
46998.5000000000f, 47018.1328125000f, 47037.7656250000f, 47057.4023437500f, 
47077.0429687500f, 47096.6835937500f, 47116.3242187500f, 47135.9726562500f, 
47155.6171875000f, 47175.2656250000f, 47194.9179687500f, 47214.5703125000f, 
47234.2265625000f, 47253.8828125000f, 47273.5429687500f, 47293.2070312500f, 
47312.8710937500f, 47332.5351562500f, 47352.2031250000f, 47371.8710937500f, 
47391.5429687500f, 47411.2187500000f, 47430.8945312500f, 47450.5703125000f, 
47470.2500000000f, 47489.9335937500f, 47509.6171875000f, 47529.3007812500f, 
47548.9921875000f, 47568.6796875000f, 47588.3710937500f, 47608.0664062500f, 
47627.7617187500f, 47647.4609375000f, 47667.1601562500f, 47686.8632812500f, 
47706.5664062500f, 47726.2734375000f, 47745.9843750000f, 47765.6914062500f, 
47785.4062500000f, 47805.1210937500f, 47824.8359375000f, 47844.5546875000f, 
47864.2773437500f, 47884.0000000000f, 47903.7226562500f, 47923.4492187500f, 
47943.1796875000f, 47962.9101562500f, 47982.6406250000f, 48002.3750000000f, 
48022.1132812500f, 48041.8515625000f, 48061.5937500000f, 48081.3359375000f, 
48101.0820312500f, 48120.8281250000f, 48140.5781250000f, 48160.3281250000f, 
48180.0820312500f, 48199.8359375000f, 48219.5937500000f, 48239.3515625000f, 
48259.1132812500f, 48278.8750000000f, 48298.6406250000f, 48318.4101562500f, 
48338.1757812500f, 48357.9492187500f, 48377.7226562500f, 48397.4960937500f, 
48417.2734375000f, 48437.0546875000f, 48456.8359375000f, 48476.6171875000f, 
48496.4023437500f, 48516.1914062500f, 48535.9804687500f, 48555.7734375000f, 
48575.5664062500f, 48595.3593750000f, 48615.1601562500f, 48634.9570312500f, 
48654.7578125000f, 48674.5625000000f, 48694.3671875000f, 48714.1757812500f, 
48733.9843750000f, 48753.7968750000f, 48773.6093750000f, 48793.4257812500f, 
48813.2421875000f, 48833.0625000000f, 48852.8867187500f, 48872.7070312500f, 
48892.5351562500f, 48912.3632812500f, 48932.1914062500f, 48952.0234375000f, 
48971.8554687500f, 48991.6914062500f, 49011.5312500000f, 49031.3710937500f, 
49051.2109375000f, 49071.0546875000f, 49090.9023437500f, 49110.7500000000f, 
49130.5976562500f, 49150.4492187500f, 49170.3046875000f, 49190.1601562500f, 
49210.0195312500f, 49229.8789062500f, 49249.7382812500f, 49269.6015625000f, 
49289.4687500000f, 49309.3359375000f, 49329.2070312500f, 49349.0781250000f, 
49368.9531250000f, 49388.8281250000f, 49408.7070312500f, 49428.5859375000f, 
49448.4687500000f, 49468.3515625000f, 49488.2382812500f, 49508.1250000000f, 
49528.0156250000f, 49547.9062500000f, 49567.8007812500f, 49587.6953125000f, 
49607.5937500000f, 49627.4921875000f, 49647.3945312500f, 49667.2968750000f, 
49687.2031250000f, 49707.1132812500f, 49727.0234375000f, 49746.9335937500f, 
49766.8476562500f, 49786.7617187500f, 49806.6796875000f, 49826.6015625000f, 
49846.5234375000f, 49866.4453125000f, 49886.3710937500f, 49906.3007812500f, 
49926.2304687500f, 49946.1601562500f, 49966.0937500000f, 49986.0312500000f, 
50005.9687500000f, 50025.9062500000f, 50045.8476562500f, 50065.7929687500f, 
50085.7382812500f, 50105.6835937500f, 50125.6328125000f, 50145.5859375000f, 
50165.5390625000f, 50185.4960937500f, 50205.4531250000f, 50225.4101562500f, 
50245.3750000000f, 50265.3359375000f, 50285.3007812500f, 50305.2695312500f, 
50325.2382812500f, 50345.2109375000f, 50365.1835937500f, 50385.1601562500f, 
50405.1367187500f, 50425.1132812500f, 50445.0976562500f, 50465.0781250000f, 
50485.0664062500f, 50505.0507812500f, 50525.0390625000f, 50545.0312500000f, 
50565.0234375000f, 50585.0195312500f, 50605.0156250000f, 50625.0156250000f, 
50645.0156250000f, 50665.0195312500f, 50685.0234375000f, 50705.0312500000f, 
50725.0429687500f, 50745.0507812500f, 50765.0664062500f, 50785.0781250000f, 
50805.0976562500f, 50825.1132812500f, 50845.1367187500f, 50865.1601562500f, 
50885.1835937500f, 50905.2109375000f, 50925.2382812500f, 50945.2695312500f, 
50965.3007812500f, 50985.3359375000f, 51005.3710937500f, 51025.4101562500f, 
51045.4531250000f, 51065.4921875000f, 51085.5390625000f, 51105.5859375000f, 
51125.6328125000f, 51145.6835937500f, 51165.7343750000f, 51185.7890625000f, 
51205.8437500000f, 51225.9023437500f, 51245.9648437500f, 51266.0273437500f, 
51286.0898437500f, 51306.1562500000f, 51326.2226562500f, 51346.2929687500f, 
51366.3671875000f, 51386.4375000000f, 51406.5156250000f, 51426.5937500000f, 
51446.6718750000f, 51466.7539062500f, 51486.8359375000f, 51506.9218750000f, 
51527.0117187500f, 51547.1015625000f, 51567.1914062500f, 51587.2851562500f, 
51607.3789062500f, 51627.4765625000f, 51647.5781250000f, 51667.6796875000f, 
51687.7812500000f, 51707.8867187500f, 51727.9921875000f, 51748.1015625000f, 
51768.2148437500f, 51788.3281250000f, 51808.4414062500f, 51828.5585937500f, 
51848.6757812500f, 51868.7968750000f, 51888.9218750000f, 51909.0468750000f, 
51929.1718750000f, 51949.3007812500f, 51969.4296875000f, 51989.5625000000f, 
52009.6992187500f, 52029.8359375000f, 52049.9726562500f, 52070.1132812500f, 
52090.2539062500f, 52110.3984375000f, 52130.5468750000f, 52150.6914062500f, 
52170.8437500000f, 52190.9960937500f, 52211.1484375000f, 52231.3046875000f, 
52251.4609375000f, 52271.6210937500f, 52291.7851562500f, 52311.9492187500f, 
52332.1132812500f, 52352.2812500000f, 52372.4492187500f, 52392.6210937500f, 
52412.7929687500f, 52432.9687500000f, 52453.1484375000f, 52473.3281250000f, 
52493.5078125000f, 52513.6914062500f, 52533.8750000000f, 52554.0625000000f, 
52574.2500000000f, 52594.4414062500f, 52614.6328125000f, 52634.8281250000f, 
52655.0273437500f, 52675.2226562500f, 52695.4257812500f, 52715.6289062500f, 
52735.8320312500f, 52756.0390625000f, 52776.2460937500f, 52796.4570312500f, 
52816.6679687500f, 52836.8828125000f, 52857.0976562500f, 52877.3164062500f, 
52897.5351562500f, 52917.7578125000f, 52937.9804687500f, 52958.2070312500f, 
52978.4335937500f, 52998.6640625000f, 53018.8945312500f, 53039.1289062500f, 
53059.3632812500f, 53079.6015625000f, 53099.8398437500f, 53120.0820312500f, 
53140.3242187500f, 53160.5703125000f, 53180.8164062500f, 53201.0664062500f, 
53221.3164062500f, 53241.5664062500f, 53261.8242187500f, 53282.0781250000f, 
53302.3359375000f, 53322.5976562500f, 53342.8593750000f, 53363.1250000000f, 
53383.3906250000f, 53403.6562500000f, 53423.9296875000f, 53444.1992187500f, 
53464.4726562500f, 53484.7500000000f, 53505.0273437500f, 53525.3046875000f, 
53545.5898437500f, 53565.8710937500f, 53586.1562500000f, 53606.4453125000f, 
53626.7343750000f, 53647.0234375000f, 53667.3164062500f, 53687.6132812500f, 
53707.9101562500f, 53728.2070312500f, 53748.5078125000f, 53768.8125000000f, 
53789.1171875000f, 53809.4218750000f, 53829.7304687500f, 53850.0390625000f, 
53870.3515625000f, 53890.6679687500f, 53910.9804687500f, 53931.3007812500f, 
53951.6210937500f, 53971.9414062500f, 53992.2656250000f, 54012.5898437500f, 
54032.9179687500f, 54053.2460937500f, 54073.5781250000f, 54093.9140625000f, 
54114.2460937500f, 54134.5859375000f, 54154.9218750000f, 54175.2656250000f, 
54195.6054687500f, 54215.9531250000f, 54236.2968750000f, 54256.6484375000f, 
54276.9960937500f, 54297.3476562500f, 54317.7031250000f, 54338.0585937500f, 
54358.4179687500f, 54378.7773437500f, 54399.1406250000f, 54419.5039062500f, 
54439.8671875000f, 54460.2343750000f, 54480.6054687500f, 54500.9765625000f, 
54521.3515625000f, 54541.7265625000f, 54562.1015625000f, 54582.4804687500f, 
54602.8632812500f, 54623.2460937500f, 54643.6289062500f, 54664.0156250000f, 
54684.4062500000f, 54704.7968750000f, 54725.1875000000f, 54745.5820312500f, 
54765.9765625000f, 54786.3750000000f, 54806.7734375000f, 54827.1757812500f, 
54847.5820312500f, 54867.9843750000f, 54888.3945312500f, 54908.8046875000f, 
54929.2148437500f, 54949.6289062500f, 54970.0429687500f, 54990.4609375000f, 
55010.8789062500f, 55031.3007812500f, 55051.7226562500f, 55072.1445312500f, 
55092.5742187500f, 55113.0000000000f, 55133.4296875000f, 55153.8632812500f, 
55174.2968750000f, 55194.7343750000f, 55215.1718750000f, 55235.6093750000f, 
55256.0507812500f, 55276.4960937500f, 55296.9414062500f, 55317.3867187500f, 
55337.8359375000f, 55358.2890625000f, 55378.7421875000f, 55399.1953125000f, 
55419.6523437500f, 55440.1093750000f, 55460.5703125000f, 55481.0351562500f, 
55501.4960937500f, 55521.9648437500f, 55542.4335937500f, 55562.9023437500f, 
55583.3750000000f, 55603.8476562500f, 55624.3242187500f, 55644.8007812500f, 
55665.2812500000f, 55685.7617187500f, 55706.2421875000f, 55726.7304687500f, 
55747.2148437500f, 55767.7031250000f, 55788.1953125000f, 55808.6875000000f, 
55829.1796875000f, 55849.6796875000f, 55870.1757812500f, 55890.6757812500f, 
55911.1796875000f, 55931.6796875000f, 55952.1875000000f, 55972.6953125000f, 
55993.2031250000f, 56013.7148437500f, 56034.2265625000f, 56054.7421875000f, 
56075.2617187500f, 56095.7773437500f, 56116.3007812500f, 56136.8242187500f, 
56157.3476562500f, 56177.8750000000f, 56198.4023437500f, 56218.9296875000f, 
56239.4648437500f, 56259.9960937500f, 56280.5312500000f, 56301.0703125000f, 
56321.6093750000f, 56342.1523437500f, 56362.6953125000f, 56383.2382812500f, 
56403.7851562500f, 56424.3359375000f, 56444.8867187500f, 56465.4375000000f, 
56485.9921875000f, 56506.5468750000f, 56527.1054687500f, 56547.6679687500f, 
56568.2265625000f, 56588.7929687500f, 56609.3593750000f, 56629.9257812500f, 
56650.4960937500f, 56671.0664062500f, 56691.6406250000f, 56712.2148437500f, 
56732.7890625000f, 56753.3671875000f, 56773.9492187500f, 56794.5312500000f, 
56815.1171875000f, 56835.7031250000f, 56856.2890625000f, 56876.8789062500f, 
56897.4726562500f, 56918.0664062500f, 56938.6601562500f, 56959.2578125000f, 
56979.8593750000f, 57000.4570312500f, 57021.0625000000f, 57041.6679687500f, 
57062.2734375000f, 57082.8828125000f, 57103.4921875000f, 57124.1054687500f, 
57144.7187500000f, 57165.3320312500f, 57185.9531250000f, 57206.5703125000f, 
57227.1914062500f, 57247.8164062500f, 57268.4414062500f, 57289.0664062500f, 
57309.6953125000f, 57330.3281250000f, 57350.9609375000f, 57371.5937500000f, 
57392.2304687500f, 57412.8710937500f, 57433.5078125000f, 57454.1523437500f, 
57474.7968750000f, 57495.4414062500f, 57516.0898437500f, 57536.7382812500f, 
57557.3906250000f, 57578.0429687500f, 57598.6953125000f, 57619.3515625000f, 
57640.0117187500f, 57660.6718750000f, 57681.3359375000f, 57702.0000000000f, 
57722.6640625000f, 57743.3320312500f, 57764.0039062500f, 57784.6718750000f, 
57805.3476562500f, 57826.0234375000f, 57846.6992187500f, 57867.3789062500f, 
57888.0585937500f, 57908.7421875000f, 57929.4257812500f, 57950.1132812500f, 
57970.8007812500f, 57991.4921875000f, 58012.1835937500f, 58032.8750000000f, 
58053.5703125000f, 58074.2695312500f, 58094.9687500000f, 58115.6679687500f, 
58136.3710937500f, 58157.0781250000f, 58177.7851562500f, 58198.4921875000f, 
58219.2031250000f, 58239.9140625000f, 58260.6289062500f, 58281.3437500000f, 
58302.0625000000f, 58322.7812500000f, 58343.5039062500f, 58364.2265625000f, 
58384.9492187500f, 58405.6796875000f, 58426.4062500000f, 58447.1367187500f, 
58467.8710937500f, 58488.6054687500f, 58509.3398437500f, 58530.0781250000f, 
58550.8164062500f, 58571.5585937500f, 58592.3007812500f, 58613.0468750000f, 
58633.7929687500f, 58654.5429687500f, 58675.2929687500f, 58696.0468750000f, 
58716.8007812500f, 58737.5585937500f, 58758.3164062500f, 58779.0742187500f, 
58799.8359375000f, 58820.6015625000f, 58841.3671875000f, 58862.1328125000f, 
58882.9023437500f, 58903.6718750000f, 58924.4453125000f, 58945.2187500000f, 
58965.9960937500f, 58986.7734375000f, 59007.5546875000f, 59028.3359375000f, 
59049.1210937500f, 59069.9062500000f, 59090.6953125000f, 59111.4843750000f, 
59132.2734375000f, 59153.0664062500f, 59173.8632812500f, 59194.6562500000f, 
59215.4570312500f, 59236.2578125000f, 59257.0585937500f, 59277.8632812500f, 
59298.6679687500f, 59319.4765625000f, 59340.2851562500f, 59361.0976562500f, 
59381.9101562500f, 59402.7226562500f, 59423.5390625000f, 59444.3593750000f, 
59465.1796875000f, 59486.0000000000f, 59506.8242187500f, 59527.6484375000f, 
59548.4765625000f, 59569.3085937500f, 59590.1367187500f, 59610.9726562500f, 
59631.8046875000f, 59652.6445312500f, 59673.4804687500f, 59694.3203125000f, 
59715.1640625000f, 59736.0078125000f, 59756.8515625000f, 59777.6992187500f, 
59798.5507812500f, 59819.4023437500f, 59840.2539062500f, 59861.1093750000f, 
59881.9648437500f, 59902.8242187500f, 59923.6835937500f, 59944.5468750000f, 
59965.4101562500f, 59986.2773437500f, 60007.1445312500f, 60028.0117187500f, 
60048.8828125000f, 60069.7578125000f, 60090.6328125000f, 60111.5078125000f, 
60132.3867187500f, 60153.2656250000f, 60174.1484375000f, 60195.0312500000f, 
60215.9179687500f, 60236.8046875000f, 60257.6953125000f, 60278.5859375000f, 
60299.4804687500f, 60320.3750000000f, 60341.2695312500f, 60362.1679687500f, 
60383.0703125000f, 60403.9687500000f, 60424.8750000000f, 60445.7812500000f, 
60466.6875000000f, 60487.5976562500f, 60508.5078125000f, 60529.4218750000f, 
60550.3359375000f, 60571.2500000000f, 60592.1679687500f, 60613.0898437500f, 
60634.0117187500f, 60654.9335937500f, 60675.8593750000f, 60696.7890625000f, 
60717.7148437500f, 60738.6484375000f, 60759.5781250000f, 60780.5156250000f, 
60801.4492187500f, 60822.3867187500f, 60843.3281250000f, 60864.2695312500f, 
60885.2148437500f, 60906.1601562500f, 60927.1054687500f, 60948.0546875000f, 
60969.0039062500f, 60989.9570312500f, 61010.9101562500f, 61031.8671875000f, 
61052.8242187500f, 61073.7851562500f, 61094.7460937500f, 61115.7109375000f, 
61136.6757812500f, 61157.6406250000f, 61178.6093750000f, 61199.5820312500f, 
61220.5546875000f, 61241.5273437500f, 61262.5039062500f, 61283.4804687500f, 
61304.4609375000f, 61325.4414062500f, 61346.4257812500f, 61367.4101562500f, 
61388.3945312500f, 61409.3828125000f, 61430.3750000000f, 61451.3671875000f, 
61472.3593750000f, 61493.3554687500f, 61514.3515625000f, 61535.3515625000f, 
61556.3515625000f, 61577.3554687500f, 61598.3593750000f, 61619.3671875000f, 
61640.3750000000f, 61661.3828125000f, 61682.3945312500f, 61703.4101562500f, 
61724.4257812500f, 61745.4414062500f, 61766.4609375000f, 61787.4804687500f, 
61808.5039062500f, 61829.5273437500f, 61850.5546875000f, 61871.5820312500f, 
61892.6132812500f, 61913.6445312500f, 61934.6757812500f, 61955.7109375000f, 
61976.7460937500f, 61997.7851562500f, 62018.8281250000f, 62039.8671875000f, 
62060.9140625000f, 62081.9570312500f, 62103.0039062500f, 62124.0546875000f, 
62145.1054687500f, 62166.1562500000f, 62187.2109375000f, 62208.2695312500f, 
62229.3281250000f, 62250.3867187500f, 62271.4492187500f, 62292.5117187500f, 
62313.5781250000f, 62334.6445312500f, 62355.7148437500f, 62376.7851562500f, 
62397.8554687500f, 62418.9296875000f, 62440.0078125000f, 62461.0820312500f, 
62482.1640625000f, 62503.2460937500f, 62524.3281250000f, 62545.4140625000f, 
62566.5000000000f, 62587.5859375000f, 62608.6757812500f, 62629.7695312500f, 
62650.8632812500f, 62671.9570312500f, 62693.0546875000f, 62714.1562500000f, 
62735.2539062500f, 62756.3593750000f, 62777.4609375000f, 62798.5703125000f, 
62819.6757812500f, 62840.7851562500f, 62861.8984375000f, 62883.0117187500f, 
62904.1250000000f, 62925.2421875000f, 62946.3593750000f, 62967.4804687500f, 
62988.6015625000f, 63009.7265625000f, 63030.8515625000f, 63051.9804687500f, 
63073.1093750000f, 63094.2382812500f, 63115.3710937500f, 63136.5078125000f, 
63157.6445312500f, 63178.7812500000f, 63199.9218750000f, 63221.0625000000f, 
63242.2070312500f, 63263.3515625000f, 63284.4960937500f, 63305.6445312500f, 
63326.7968750000f, 63347.9492187500f, 63369.1015625000f, 63390.2578125000f, 
63411.4140625000f, 63432.5742187500f, 63453.7343750000f, 63474.8984375000f, 
63496.0625000000f, 63517.2304687500f, 63538.3984375000f, 63559.5664062500f, 
63580.7382812500f, 63601.9101562500f, 63623.0859375000f, 63644.2656250000f, 
63665.4414062500f, 63686.6210937500f, 63707.8046875000f, 63728.9882812500f, 
63750.1757812500f, 63771.3632812500f, 63792.5507812500f, 63813.7421875000f, 
63834.9335937500f, 63856.1289062500f, 63877.3242187500f, 63898.5234375000f, 
63919.7226562500f, 63940.9257812500f, 63962.1289062500f, 63983.3320312500f, 
64004.5390625000f, 64025.7460937500f, 64046.9570312500f, 64068.1679687500f, 
64089.3828125000f, 64110.5976562500f, 64131.8164062500f, 64153.0351562500f, 
64174.2539062500f, 64195.4765625000f, 64216.7031250000f, 64237.9296875000f, 
64259.1562500000f, 64280.3867187500f, 64301.6171875000f, 64322.8515625000f, 
64344.0859375000f, 64365.3203125000f, 64386.5585937500f, 64407.8007812500f, 
64429.0429687500f, 64450.2851562500f, 64471.5312500000f, 64492.7773437500f, 
64514.0273437500f, 64535.2773437500f, 64556.5312500000f, 64577.7851562500f, 
64599.0390625000f, 64620.2968750000f, 64641.5546875000f, 64662.8164062500f, 
64684.0781250000f, 64705.3437500000f, 64726.6093750000f, 64747.8789062500f, 
64769.1484375000f, 64790.4218750000f, 64811.6953125000f, 64832.9687500000f, 
64854.2460937500f, 64875.5234375000f, 64896.8046875000f, 64918.0859375000f, 
64939.3710937500f, 64960.6562500000f, 64981.9414062500f, 65003.2304687500f, 
65024.5234375000f, 65045.8164062500f, 65067.1093750000f, 65088.4062500000f, 
65109.7031250000f, 65131.0000000000f, 65152.3046875000f, 65173.6054687500f, 
65194.9101562500f, 65216.2187500000f, 65237.5234375000f, 65258.8359375000f, 
65280.1484375000f, 65301.4609375000f, 65322.7734375000f, 65344.0937500000f, 
65365.4101562500f, 65386.7304687500f, 65408.0546875000f, 65429.3750000000f, 
65450.7031250000f, 65472.0312500000f, 65493.3593750000f, 65514.6875000000f, 
65536.0234375000f, 65557.3593750000f, 65578.6953125000f, 65600.0312500000f, 
65621.3671875000f, 65642.7109375000f, 65664.0546875000f, 65685.3984375000f, 
65706.7421875000f, 65728.0937500000f, 65749.4453125000f, 65770.7968750000f, 
65792.1484375000f, 65813.5000000000f, 65834.8593750000f, 65856.2187500000f, 
65877.5781250000f, 65898.9375000000f, 65920.3046875000f, 65941.6718750000f, 
65963.0390625000f, 65984.4062500000f, 66005.7734375000f, 66027.1484375000f, 
66048.5234375000f, 66069.8984375000f, 66091.2734375000f, 66112.6562500000f, 
66134.0312500000f, 66155.4140625000f, 66176.8046875000f, 66198.1875000000f, 
66219.5781250000f, 66240.9687500000f, 66262.3593750000f, 66283.7500000000f, 
66305.1484375000f, 66326.5390625000f, 66347.9375000000f, 66369.3359375000f, 
66390.7421875000f, 66412.1484375000f, 66433.5468750000f, 66454.9531250000f, 
66476.3671875000f, 66497.7734375000f, 66519.1875000000f, 66540.6015625000f, 
66562.0156250000f, 66583.4375000000f, 66604.8515625000f, 66626.2734375000f, 
66647.6953125000f, 66669.1171875000f, 66690.5468750000f, 66711.9765625000f, 
66733.4062500000f, 66754.8359375000f, 66776.2656250000f, 66797.7031250000f, 
66819.1406250000f, 66840.5781250000f, 66862.0156250000f, 66883.4531250000f, 
66904.8984375000f, 66926.3437500000f, 66947.7890625000f, 66969.2343750000f, 
66990.6875000000f, 67012.1406250000f, 67033.5937500000f, 67055.0468750000f, 
67076.5078125000f, 67097.9609375000f, 67119.4218750000f, 67140.8828125000f, 
67162.3515625000f, 67183.8125000000f, 67205.2812500000f, 67226.7500000000f, 
67248.2187500000f, 67269.6953125000f, 67291.1640625000f, 67312.6406250000f, 
67334.1171875000f, 67355.6015625000f, 67377.0781250000f, 67398.5625000000f, 
67420.0468750000f, 67441.5312500000f, 67463.0234375000f, 67484.5078125000f, 
67506.0000000000f, 67527.4921875000f, 67548.9843750000f, 67570.4843750000f, 
67591.9843750000f, 67613.4843750000f, 67634.9843750000f, 67656.4843750000f, 
67677.9921875000f, 67699.5000000000f, 67721.0078125000f, 67742.5156250000f, 
67764.0234375000f, 67785.5390625000f, 67807.0546875000f, 67828.5703125000f, 
67850.0859375000f, 67871.6093750000f, 67893.1328125000f, 67914.6562500000f, 
67936.1796875000f, 67957.7031250000f, 67979.2343750000f, 68000.7656250000f, 
68022.2968750000f, 68043.8281250000f, 68065.3671875000f, 68086.9062500000f, 
68108.4453125000f, 68129.9843750000f, 68151.5234375000f, 68173.0703125000f, 
68194.6171875000f, 68216.1640625000f, 68237.7109375000f, 68259.2578125000f, 
68280.8125000000f, 68302.3671875000f, 68323.9218750000f, 68345.4843750000f, 
68367.0390625000f, 68388.6015625000f, 68410.1640625000f, 68431.7265625000f, 
68453.2968750000f, 68474.8593750000f, 68496.4296875000f, 68518.0000000000f, 
68539.5781250000f, 68561.1484375000f, 68582.7265625000f, 68604.3046875000f, 
68625.8828125000f, 68647.4687500000f, 68669.0468750000f, 68690.6328125000f, 
68712.2187500000f, 68733.8046875000f, 68755.3984375000f, 68776.9921875000f, 
68798.5859375000f, 68820.1796875000f, 68841.7734375000f, 68863.3750000000f, 
68884.9687500000f, 68906.5703125000f, 68928.1796875000f, 68949.7812500000f, 
68971.3906250000f, 68993.0000000000f, 69014.6093750000f, 69036.2187500000f, 
69057.8359375000f, 69079.4453125000f, 69101.0625000000f, 69122.6796875000f, 
69144.3046875000f, 69165.9218750000f, 69187.5468750000f, 69209.1718750000f, 
69230.8046875000f, 69252.4296875000f, 69274.0625000000f, 69295.6953125000f, 
69317.3281250000f, 69338.9609375000f, 69360.6015625000f, 69382.2343750000f, 
69403.8750000000f, 69425.5234375000f, 69447.1640625000f, 69468.8125000000f, 
69490.4609375000f, 69512.1093750000f, 69533.7578125000f, 69555.4062500000f, 
69577.0625000000f, 69598.7187500000f, 69620.3750000000f, 69642.0390625000f, 
69663.6953125000f, 69685.3593750000f, 69707.0234375000f, 69728.6875000000f, 
69750.3593750000f, 69772.0234375000f, 69793.6953125000f, 69815.3671875000f, 
69837.0390625000f, 69858.7187500000f, 69880.3984375000f, 69902.0781250000f, 
69923.7578125000f, 69945.4375000000f, 69967.1250000000f, 69988.8125000000f, 
70010.5000000000f, 70032.1875000000f, 70053.8750000000f, 70075.5703125000f, 
70097.2656250000f, 70118.9609375000f, 70140.6562500000f, 70162.3593750000f, 
70184.0625000000f, 70205.7578125000f, 70227.4687500000f, 70249.1718750000f, 
70270.8828125000f, 70292.5859375000f, 70314.2968750000f, 70336.0156250000f, 
70357.7265625000f, 70379.4453125000f, 70401.1640625000f, 70422.8828125000f, 
70444.6015625000f, 70466.3281250000f, 70488.0468750000f, 70509.7734375000f, 
70531.5078125000f, 70553.2343750000f, 70574.9687500000f, 70596.6953125000f, 
70618.4296875000f, 70640.1718750000f, 70661.9062500000f, 70683.6484375000f, 
70705.3906250000f, 70727.1328125000f, 70748.8750000000f, 70770.6250000000f, 
70792.3671875000f, 70814.1171875000f, 70835.8671875000f, 70857.6250000000f, 
70879.3750000000f, 70901.1328125000f, 70922.8906250000f, 70944.6484375000f, 
70966.4140625000f, 70988.1796875000f, 71009.9375000000f, 71031.7109375000f, 
71053.4765625000f, 71075.2421875000f, 71097.0156250000f, 71118.7890625000f, 
71140.5625000000f, 71162.3437500000f, 71184.1171875000f, 71205.8984375000f, 
71227.6796875000f, 71249.4609375000f, 71271.2500000000f, 71293.0312500000f, 
71314.8203125000f, 71336.6093750000f, 71358.4062500000f, 71380.1953125000f, 
71401.9921875000f, 71423.7890625000f, 71445.5859375000f, 71467.3828125000f, 
71489.1875000000f, 71510.9921875000f, 71532.7968750000f, 71554.6015625000f, 
71576.4062500000f, 71598.2187500000f, 71620.0312500000f, 71641.8437500000f, 
71663.6562500000f, 71685.4687500000f, 71707.2890625000f, 71729.1093750000f, 
71750.9296875000f, 71772.7500000000f, 71794.5781250000f, 71816.4062500000f, 
71838.2343750000f, 71860.0625000000f, 71881.8906250000f, 71903.7265625000f, 
71925.5625000000f, 71947.3984375000f, 71969.2343750000f, 71991.0703125000f, 
72012.9140625000f, 72034.7578125000f, 72056.6015625000f, 72078.4453125000f, 
72100.2968750000f, 72122.1484375000f, 72144.0000000000f, 72165.8515625000f, 
72187.7031250000f, 72209.5625000000f, 72231.4140625000f, 72253.2734375000f, 
72275.1406250000f, 72297.0000000000f, 72318.8671875000f, 72340.7265625000f, 
72362.6015625000f, 72384.4687500000f, 72406.3359375000f, 72428.2109375000f, 
72450.0859375000f, 72471.9609375000f, 72493.8359375000f, 72515.7187500000f, 
72537.6015625000f, 72559.4843750000f, 72581.3671875000f, 72603.2500000000f, 
72625.1406250000f, 72647.0234375000f, 72668.9140625000f, 72690.8125000000f, 
72712.7031250000f, 72734.6015625000f, 72756.5000000000f, 72778.3984375000f, 
72800.2968750000f, 72822.1953125000f, 72844.1015625000f, 72866.0078125000f, 
72887.9140625000f, 72909.8203125000f, 72931.7343750000f, 72953.6484375000f, 
72975.5625000000f, 72997.4765625000f, 73019.3906250000f, 73041.3125000000f, 
73063.2343750000f, 73085.1562500000f, 73107.0781250000f, 73129.0000000000f, 
73150.9296875000f, 73172.8593750000f, 73194.7890625000f, 73216.7187500000f, 
73238.6562500000f, 73260.5859375000f, 73282.5234375000f, 73304.4609375000f, 
73326.4062500000f, 73348.3437500000f, 73370.2890625000f, 73392.2343750000f, 
73414.1796875000f, 73436.1328125000f, 73458.0781250000f, 73480.0312500000f, 
73501.9843750000f, 73523.9375000000f, 73545.8984375000f, 73567.8515625000f, 
73589.8125000000f, 73611.7734375000f, 73633.7343750000f, 73655.7031250000f, 
73677.6718750000f, 73699.6328125000f, 73721.6093750000f, 73743.5781250000f, 
73765.5468750000f, 73787.5234375000f, 73809.5000000000f, 73831.4765625000f, 
73853.4609375000f, 73875.4375000000f, 73897.4218750000f, 73919.4062500000f, 
73941.3906250000f, 73963.3750000000f, 73985.3671875000f, 74007.3593750000f, 
74029.3515625000f, 74051.3437500000f, 74073.3437500000f, 74095.3359375000f, 
74117.3359375000f, 74139.3359375000f, 74161.3437500000f, 74183.3437500000f, 
74205.3515625000f, 74227.3593750000f, 74249.3671875000f, 74271.3750000000f, 
74293.3906250000f, 74315.3984375000f, 74337.4140625000f, 74359.4375000000f, 
74381.4531250000f, 74403.4687500000f, 74425.4921875000f, 74447.5156250000f, 
74469.5390625000f, 74491.5703125000f, 74513.6015625000f, 74535.6250000000f, 
74557.6562500000f, 74579.6953125000f, 74601.7265625000f, 74623.7656250000f, 
74645.8046875000f, 74667.8437500000f, 74689.8828125000f, 74711.9296875000f, 
74733.9687500000f, 74756.0156250000f, 74778.0625000000f, 74800.1171875000f, 
74822.1640625000f, 74844.2187500000f, 74866.2734375000f, 74888.3281250000f, 
74910.3828125000f, 74932.4453125000f, 74954.5078125000f, 74976.5703125000f, 
74998.6328125000f, 75020.6953125000f, 75042.7656250000f, 75064.8359375000f, 
75086.9062500000f, 75108.9765625000f, 75131.0546875000f, 75153.1250000000f, 
75175.2031250000f, 75197.2812500000f, 75219.3593750000f, 75241.4453125000f, 
75263.5312500000f, 75285.6171875000f, 75307.7031250000f, 75329.7890625000f, 
75351.8828125000f, 75373.9687500000f, 75396.0625000000f, 75418.1562500000f, 
75440.2578125000f, 75462.3515625000f, 75484.4531250000f, 75506.5546875000f, 
75528.6562500000f, 75550.7656250000f, 75572.8671875000f, 75594.9765625000f, 
75617.0859375000f, 75639.1953125000f, 75661.3125000000f, 75683.4218750000f, 
75705.5390625000f, 75727.6562500000f, 75749.7734375000f, 75771.8984375000f, 
75794.0156250000f, 75816.1406250000f, 75838.2656250000f, 75860.3984375000f, 
75882.5234375000f, 75904.6562500000f, 75926.7890625000f, 75948.9218750000f, 
75971.0546875000f, 75993.1953125000f, 76015.3281250000f, 76037.4687500000f, 
76059.6171875000f, 76081.7578125000f, 76103.8984375000f, 76126.0468750000f, 
76148.1953125000f, 76170.3437500000f, 76192.5000000000f, 76214.6484375000f, 
76236.8046875000f, 76258.9609375000f, 76281.1171875000f, 76303.2812500000f, 
76325.4375000000f, 76347.6015625000f, 76369.7656250000f, 76391.9296875000f, 
76414.1015625000f, 76436.2734375000f, 76458.4375000000f, 76480.6093750000f, 
76502.7890625000f, 76524.9609375000f, 76547.1406250000f, 76569.3203125000f, 
76591.5000000000f, 76613.6796875000f, 76635.8671875000f, 76658.0468750000f, 
76680.2343750000f, 76702.4218750000f, 76724.6171875000f, 76746.8046875000f, 
76769.0000000000f, 76791.1953125000f, 76813.3906250000f, 76835.5859375000f, 
76857.7890625000f, 76879.9921875000f, 76902.1953125000f, 76924.3984375000f, 
76946.6015625000f, 76968.8125000000f, 76991.0156250000f, 77013.2265625000f, 
77035.4453125000f, 77057.6562500000f, 77079.8750000000f, 77102.0859375000f, 
77124.3046875000f, 77146.5312500000f, 77168.7500000000f, 77190.9765625000f, 
77213.1953125000f, 77235.4296875000f, 77257.6562500000f, 77279.8828125000f, 
77302.1171875000f, 77324.3515625000f, 77346.5859375000f, 77368.8203125000f, 
77391.0546875000f, 77413.2968750000f, 77435.5390625000f, 77457.7812500000f, 
77480.0234375000f, 77502.2734375000f, 77524.5234375000f, 77546.7656250000f, 
77569.0234375000f, 77591.2734375000f, 77613.5234375000f, 77635.7812500000f, 
77658.0390625000f, 77680.2968750000f, 77702.5546875000f, 77724.8203125000f, 
77747.0859375000f, 77769.3515625000f, 77791.6171875000f, 77813.8828125000f, 
77836.1562500000f, 77858.4218750000f, 77880.6953125000f, 77902.9765625000f, 
77925.2500000000f, 77947.5312500000f, 77969.8046875000f, 77992.0859375000f, 
78014.3671875000f, 78036.6562500000f, 78058.9375000000f, 78081.2265625000f, 
78103.5156250000f, 78125.8046875000f, 78148.1015625000f, 78170.3906250000f, 
78192.6875000000f, 78214.9843750000f, 78237.2812500000f, 78259.5859375000f, 
78281.8828125000f, 78304.1875000000f, 78326.4921875000f, 78348.8046875000f, 
78371.1093750000f, 78393.4218750000f, 78415.7265625000f, 78438.0468750000f, 
78460.3593750000f, 78482.6718750000f, 78504.9921875000f, 78527.3125000000f, 
78549.6328125000f, 78571.9531250000f, 78594.2812500000f, 78616.6015625000f, 
78638.9296875000f, 78661.2578125000f, 78683.5859375000f, 78705.9218750000f, 
78728.2578125000f, 78750.5859375000f, 78772.9296875000f, 78795.2656250000f, 
78817.6015625000f, 78839.9453125000f, 78862.2890625000f, 78884.6328125000f, 
78906.9765625000f, 78929.3281250000f, 78951.6796875000f, 78974.0234375000f, 
78996.3828125000f, 79018.7343750000f, 79041.0859375000f, 79063.4453125000f, 
79085.8046875000f, 79108.1640625000f, 79130.5312500000f, 79152.8906250000f, 
79175.2578125000f, 79197.6250000000f, 79219.9921875000f, 79242.3593750000f, 
79264.7343750000f, 79287.1015625000f, 79309.4765625000f, 79331.8593750000f, 
79354.2343750000f, 79376.6093750000f, 79398.9921875000f, 79421.3750000000f, 
79443.7578125000f, 79466.1484375000f, 79488.5312500000f, 79510.9218750000f, 
79533.3125000000f, 79555.7031250000f, 79578.1015625000f, 79600.4921875000f, 
79622.8906250000f, 79645.2890625000f, 79667.6875000000f, 79690.0859375000f, 
79712.4921875000f, 79734.8984375000f, 79757.3046875000f, 79779.7109375000f, 
79802.1171875000f, 79824.5312500000f, 79846.9453125000f, 79869.3593750000f, 
79891.7734375000f, 79914.1875000000f, 79936.6093750000f, 79959.0312500000f, 
79981.4531250000f, 80003.8750000000f, 80026.2968750000f, 80048.7265625000f, 
80071.1562500000f, 80093.5859375000f, 80116.0156250000f, 80138.4453125000f, 
80160.8828125000f, 80183.3203125000f, 80205.7578125000f, 80228.1953125000f, 
80250.6328125000f, 80273.0781250000f, 80295.5234375000f, 80317.9687500000f, 
80340.4140625000f, 80362.8593750000f, 80385.3125000000f, 80407.7656250000f, 
80430.2187500000f, 80452.6718750000f, 80475.1250000000f, 80497.5859375000f, 
80520.0468750000f, 80542.5078125000f, 80564.9687500000f, 80587.4296875000f, 
80609.8984375000f, 80632.3671875000f, 80654.8359375000f, 80677.3046875000f, 
80699.7734375000f, 80722.2500000000f, 80744.7265625000f, 80767.2031250000f, 
80789.6796875000f, 80812.1640625000f, 80834.6406250000f, 80857.1250000000f, 
80879.6093750000f, 80902.0937500000f, 80924.5859375000f, 80947.0703125000f, 
80969.5625000000f, 80992.0546875000f, 81014.5468750000f, 81037.0468750000f, 
81059.5390625000f, 81082.0390625000f, 81104.5390625000f, 81127.0390625000f, 
81149.5468750000f, 81172.0468750000f, 81194.5546875000f, 81217.0625000000f, 
81239.5703125000f, 81262.0859375000f, 81284.6015625000f, 81307.1093750000f, 
81329.6250000000f, 81352.1484375000f, 81374.6640625000f, 81397.1875000000f, 
81419.7031250000f, 81442.2265625000f, 81464.7578125000f, 81487.2812500000f, 
81509.8125000000f, 81532.3359375000f, 81554.8671875000f, 81577.4062500000f, 
81599.9375000000f, 81622.4765625000f, 81645.0078125000f, 81667.5468750000f, 
81690.0937500000f, 81712.6328125000f, 81735.1796875000f, 81757.7187500000f, 
81780.2656250000f, 81802.8203125000f, 81825.3671875000f, 81847.9218750000f, 
81870.4687500000f, 81893.0234375000f, 81915.5859375000f, 81938.1406250000f, 
81960.7031250000f, 81983.2578125000f, 82005.8203125000f, 82028.3906250000f, 
82050.9531250000f, 82073.5234375000f, 82096.0859375000f, 82118.6562500000f, 
82141.2265625000f, 82163.8046875000f, 82186.3750000000f, 82208.9531250000f, 
82231.5312500000f, 82254.1093750000f, 82276.6953125000f, 82299.2734375000f, 
82321.8593750000f, 82344.4453125000f, 82367.0312500000f, 82389.6250000000f, 
82412.2109375000f, 82434.8046875000f, 82457.3984375000f, 82479.9921875000f, 
82502.5859375000f, 82525.1875000000f, 82547.7890625000f, 82570.3906250000f, 
82592.9921875000f, 82615.5937500000f, 82638.2031250000f, 82660.8046875000f, 
82683.4140625000f, 82706.0234375000f, 82728.6406250000f, 82751.2500000000f, 
82773.8671875000f, 82796.4843750000f, 82819.1015625000f, 82841.7187500000f, 
82864.3437500000f, 82886.9687500000f, 82909.5859375000f, 82932.2187500000f, 
82954.8437500000f, 82977.4687500000f, 83000.1015625000f, 83022.7343750000f, 
83045.3671875000f, 83068.0000000000f, 83090.6406250000f, 83113.2812500000f, 
83135.9140625000f, 83158.5546875000f, 83181.2031250000f, 83203.8437500000f, 
83226.4921875000f, 83249.1406250000f, 83271.7890625000f, 83294.4375000000f, 
83317.0937500000f, 83339.7421875000f, 83362.3984375000f, 83385.0546875000f, 
83407.7109375000f, 83430.3750000000f, 83453.0312500000f, 83475.6953125000f, 
83498.3593750000f, 83521.0312500000f, 83543.6953125000f, 83566.3671875000f, 
83589.0312500000f, 83611.7109375000f, 83634.3828125000f, 83657.0546875000f, 
83679.7343750000f, 83702.4140625000f, 83725.0937500000f, 83747.7734375000f, 
83770.4531250000f, 83793.1406250000f, 83815.8281250000f, 83838.5156250000f, 
83861.2031250000f, 83883.8906250000f, 83906.5859375000f, 83929.2734375000f, 
83951.9687500000f, 83974.6718750000f, 83997.3671875000f, 84020.0703125000f, 
84042.7656250000f, 84065.4687500000f, 84088.1718750000f, 84110.8828125000f, 
84133.5859375000f, 84156.2968750000f, 84179.0078125000f, 84201.7187500000f, 
84224.4296875000f, 84247.1484375000f, 84269.8671875000f, 84292.5859375000f, 
84315.3046875000f, 84338.0234375000f, 84360.7500000000f, 84383.4687500000f, 
84406.1953125000f, 84428.9218750000f, 84451.6484375000f, 84474.3828125000f, 
84497.1171875000f, 84519.8437500000f, 84542.5859375000f, 84565.3203125000f, 
84588.0546875000f, 84610.7968750000f, 84633.5390625000f, 84656.2812500000f, 
84679.0234375000f, 84701.7734375000f, 84724.5156250000f, 84747.2656250000f, 
84770.0156250000f, 84792.7656250000f, 84815.5234375000f, 84838.2734375000f, 
84861.0312500000f, 84883.7890625000f, 84906.5468750000f, 84929.3125000000f, 
84952.0703125000f, 84974.8359375000f, 84997.6015625000f, 85020.3671875000f, 
85043.1406250000f, 85065.9062500000f, 85088.6796875000f, 85111.4531250000f, 
85134.2265625000f, 85157.0000000000f, 85179.7812500000f, 85202.5625000000f, 
85225.3359375000f, 85248.1250000000f, 85270.9062500000f, 85293.6875000000f, 
85316.4765625000f, 85339.2656250000f, 85362.0546875000f, 85384.8437500000f, 
85407.6406250000f, 85430.4375000000f, 85453.2265625000f, 85476.0234375000f, 
85498.8281250000f, 85521.6250000000f, 85544.4296875000f, 85567.2343750000f, 
85590.0390625000f, 85612.8437500000f, 85635.6484375000f, 85658.4609375000f, 
85681.2734375000f, 85704.0859375000f, 85726.8984375000f, 85749.7109375000f, 
85772.5312500000f, 85795.3515625000f, 85818.1718750000f, 85840.9921875000f, 
85863.8125000000f, 85886.6406250000f, 85909.4687500000f, 85932.2968750000f, 
85955.1250000000f, 85977.9531250000f, 86000.7890625000f, 86023.6171875000f, 
86046.4531250000f, 86069.2890625000f, 86092.1328125000f, 86114.9687500000f, 
86137.8125000000f, 86160.6562500000f, 86183.5000000000f, 86206.3437500000f, 
86229.1953125000f, 86252.0390625000f, 86274.8906250000f, 86297.7421875000f, 
86320.6015625000f, 86343.4531250000f, 86366.3125000000f, 86389.1718750000f, 
86412.0312500000f, 86434.8906250000f, 86457.7500000000f, 86480.6171875000f, 
86503.4843750000f, 86526.3515625000f, 86549.2187500000f, 86572.0859375000f, 
86594.9609375000f, 86617.8281250000f, 86640.7031250000f, 86663.5859375000f, 
86686.4609375000f, 86709.3359375000f, 86732.2187500000f, 86755.1015625000f, 
86777.9843750000f, 86800.8750000000f, 86823.7578125000f, 86846.6484375000f, 
86869.5390625000f, 86892.4296875000f, 86915.3203125000f, 86938.2109375000f, 
86961.1093750000f, 86984.0078125000f, 87006.9062500000f, 87029.8046875000f, 
87052.7109375000f, 87075.6093750000f, 87098.5156250000f, 87121.4218750000f, 
87144.3281250000f, 87167.2421875000f, 87190.1484375000f, 87213.0625000000f, 
87235.9765625000f, 87258.8906250000f, 87281.8125000000f, 87304.7265625000f, 
87327.6484375000f, 87350.5703125000f, 87373.4921875000f, 87396.4140625000f, 
87419.3437500000f, 87442.2734375000f, 87465.1953125000f, 87488.1328125000f, 
87511.0625000000f, 87533.9921875000f, 87556.9296875000f, 87579.8671875000f, 
87602.8046875000f, 87625.7421875000f, 87648.6875000000f, 87671.6250000000f, 
87694.5703125000f, 87717.5156250000f, 87740.4609375000f, 87763.4140625000f, 
87786.3593750000f, 87809.3125000000f, 87832.2656250000f, 87855.2187500000f, 
87878.1796875000f, 87901.1328125000f, 87924.0937500000f, 87947.0546875000f, 
87970.0156250000f, 87992.9765625000f, 88015.9453125000f, 88038.9062500000f, 
88061.8750000000f, 88084.8437500000f, 88107.8203125000f, 88130.7890625000f, 
88153.7656250000f, 88176.7421875000f, 88199.7187500000f, 88222.6953125000f, 
88245.6718750000f, 88268.6562500000f, 88291.6406250000f, 88314.6250000000f, 
88337.6093750000f, 88360.5937500000f, 88383.5859375000f, 88406.5781250000f, 
88429.5703125000f, 88452.5625000000f, 88475.5546875000f, 88498.5546875000f, 
88521.5468750000f, 88544.5468750000f, 88567.5468750000f, 88590.5546875000f, 
88613.5546875000f, 88636.5625000000f, 88659.5703125000f, 88682.5781250000f, 
88705.5859375000f, 88728.6015625000f, 88751.6093750000f, 88774.6250000000f, 
88797.6406250000f, 88820.6562500000f, 88843.6796875000f, 88866.6953125000f, 
88889.7187500000f, 88912.7421875000f, 88935.7656250000f, 88958.7890625000f, 
88981.8203125000f, 89004.8515625000f, 89027.8828125000f, 89050.9140625000f, 
89073.9453125000f, 89096.9843750000f, 89120.0156250000f, 89143.0546875000f, 
89166.0937500000f, 89189.1328125000f, 89212.1796875000f, 89235.2265625000f, 
89258.2656250000f, 89281.3125000000f, 89304.3671875000f, 89327.4140625000f, 
89350.4687500000f, 89373.5156250000f, 89396.5703125000f, 89419.6328125000f, 
89442.6875000000f, 89465.7421875000f, 89488.8046875000f, 89511.8671875000f, 
89534.9296875000f, 89558.0000000000f, 89581.0625000000f, 89604.1328125000f, 
89627.2031250000f, 89650.2734375000f, 89673.3437500000f, 89696.4140625000f, 
89719.4921875000f, 89742.5703125000f, 89765.6484375000f, 89788.7265625000f, 
89811.8046875000f, 89834.8906250000f, 89857.9765625000f, 89881.0625000000f, 
89904.1484375000f, 89927.2343750000f, 89950.3281250000f, 89973.4140625000f, 
89996.5078125000f, 90019.6015625000f, 90042.7031250000f, 90065.7968750000f, 
90088.8984375000f, 90112.0000000000f, 90135.1015625000f, 90158.2031250000f, 
90181.3046875000f, 90204.4140625000f, 90227.5234375000f, 90250.6328125000f, 
90273.7421875000f, 90296.8515625000f, 90319.9687500000f, 90343.0859375000f, 
90366.2031250000f, 90389.3203125000f, 90412.4375000000f, 90435.5625000000f, 
90458.6796875000f, 90481.8046875000f, 90504.9296875000f, 90528.0625000000f, 
90551.1875000000f, 90574.3203125000f, 90597.4531250000f, 90620.5859375000f, 
90643.7187500000f, 90666.8515625000f, 90689.9921875000f, 90713.1328125000f, 
90736.2656250000f, 90759.4140625000f, 90782.5546875000f, 90805.7031250000f, 
90828.8437500000f, 90851.9921875000f, 90875.1406250000f, 90898.2968750000f, 
90921.4453125000f, 90944.6015625000f, 90967.7578125000f, 90990.9140625000f, 
91014.0703125000f, 91037.2265625000f, 91060.3906250000f, 91083.5546875000f, 
91106.7187500000f, 91129.8828125000f, 91153.0468750000f, 91176.2187500000f, 
91199.3828125000f, 91222.5546875000f, 91245.7343750000f, 91268.9062500000f, 
91292.0781250000f, 91315.2578125000f, 91338.4375000000f, 91361.6171875000f, 
91384.7968750000f, 91407.9843750000f, 91431.1640625000f, 91454.3515625000f, 
91477.5390625000f, 91500.7265625000f, 91523.9218750000f, 91547.1093750000f, 
91570.3046875000f, 91593.5000000000f, 91616.6953125000f, 91639.8906250000f, 
91663.0937500000f, 91686.2890625000f, 91709.4921875000f, 91732.6953125000f, 
91755.9062500000f, 91779.1093750000f, 91802.3203125000f, 91825.5312500000f, 
91848.7421875000f, 91871.9531250000f, 91895.1640625000f, 91918.3828125000f, 
91941.5937500000f, 91964.8125000000f, 91988.0390625000f, 92011.2578125000f, 
92034.4765625000f, 92057.7031250000f, 92080.9296875000f, 92104.1562500000f, 
92127.3828125000f, 92150.6171875000f, 92173.8437500000f, 92197.0781250000f, 
92220.3125000000f, 92243.5468750000f, 92266.7890625000f, 92290.0234375000f, 
92313.2656250000f, 92336.5078125000f, 92359.7500000000f, 92382.9921875000f, 
92406.2421875000f, 92429.4921875000f, 92452.7421875000f, 92475.9921875000f, 
92499.2421875000f, 92522.4921875000f, 92545.7500000000f, 92569.0078125000f, 
92592.2656250000f, 92615.5234375000f, 92638.7812500000f, 92662.0468750000f, 
92685.3125000000f, 92708.5781250000f, 92731.8437500000f, 92755.1093750000f, 
92778.3750000000f, 92801.6484375000f, 92824.9218750000f, 92848.1953125000f, 
92871.4687500000f, 92894.7500000000f, 92918.0234375000f, 92941.3046875000f, 
92964.5859375000f, 92987.8671875000f, 93011.1562500000f, 93034.4375000000f, 
93057.7265625000f, 93081.0156250000f, 93104.3046875000f, 93127.5937500000f, 
93150.8906250000f, 93174.1796875000f, 93197.4765625000f, 93220.7734375000f, 
93244.0781250000f, 93267.3750000000f, 93290.6796875000f, 93313.9765625000f, 
93337.2812500000f, 93360.5859375000f, 93383.8984375000f, 93407.2031250000f, 
93430.5156250000f, 93453.8281250000f, 93477.1406250000f, 93500.4531250000f, 
93523.7734375000f, 93547.0859375000f, 93570.4062500000f, 93593.7265625000f, 
93617.0468750000f, 93640.3750000000f, 93663.6953125000f, 93687.0234375000f, 
93710.3515625000f, 93733.6796875000f, 93757.0156250000f, 93780.3437500000f, 
93803.6796875000f, 93827.0156250000f, 93850.3515625000f, 93873.6875000000f, 
93897.0234375000f, 93920.3671875000f, 93943.7109375000f, 93967.0546875000f, 
93990.3984375000f, 94013.7421875000f, 94037.0937500000f, 94060.4453125000f, 
94083.7968750000f, 94107.1484375000f, 94130.5000000000f, 94153.8515625000f, 
94177.2109375000f, 94200.5703125000f, 94223.9296875000f, 94247.2890625000f, 
94270.6562500000f, 94294.0156250000f, 94317.3828125000f, 94340.7500000000f, 
94364.1171875000f, 94387.4843750000f, 94410.8593750000f, 94434.2343750000f, 
94457.6015625000f, 94480.9843750000f, 94504.3593750000f, 94527.7343750000f, 
94551.1171875000f, 94574.5000000000f, 94597.8828125000f, 94621.2656250000f, 
94644.6484375000f, 94668.0390625000f, 94691.4218750000f, 94714.8125000000f, 
94738.2109375000f, 94761.6015625000f, 94784.9921875000f, 94808.3906250000f, 
94831.7890625000f, 94855.1875000000f, 94878.5859375000f, 94901.9843750000f, 
94925.3906250000f, 94948.7968750000f, 94972.2031250000f, 94995.6093750000f, 
95019.0156250000f, 95042.4296875000f, 95065.8359375000f, 95089.2500000000f, 
95112.6640625000f, 95136.0859375000f, 95159.5000000000f, 95182.9218750000f, 
95206.3359375000f, 95229.7578125000f, 95253.1796875000f, 95276.6093750000f, 
95300.0312500000f, 95323.4609375000f, 95346.8906250000f, 95370.3203125000f, 
95393.7500000000f, 95417.1875000000f, 95440.6171875000f, 95464.0546875000f, 
95487.4921875000f, 95510.9296875000f, 95534.3750000000f, 95557.8125000000f, 
95581.2578125000f, 95604.7031250000f, 95628.1484375000f, 95651.5937500000f, 
95675.0468750000f, 95698.5000000000f, 95721.9453125000f, 95745.3984375000f, 
95768.8593750000f, 95792.3125000000f, 95815.7734375000f, 95839.2265625000f, 
95862.6875000000f, 95886.1562500000f, 95909.6171875000f, 95933.0781250000f, 
95956.5468750000f, 95980.0156250000f, 96003.4843750000f, 96026.9531250000f, 
96050.4296875000f, 96073.8984375000f, 96097.3750000000f, 96120.8515625000f, 
96144.3281250000f, 96167.8046875000f, 96191.2890625000f, 96214.7734375000f, 
96238.2578125000f, 96261.7421875000f, 96285.2265625000f, 96308.7109375000f, 
96332.2031250000f, 96355.6953125000f, 96379.1875000000f, 96402.6796875000f, 
96426.1718750000f, 96449.6718750000f, 96473.1640625000f, 96496.6640625000f, 
96520.1640625000f, 96543.6718750000f, 96567.1718750000f, 96590.6796875000f, 
96614.1875000000f, 96637.6953125000f, 96661.2031250000f, 96684.7109375000f, 
96708.2265625000f, 96731.7343750000f, 96755.2500000000f, 96778.7656250000f, 
96802.2890625000f, 96825.8046875000f, 96849.3281250000f, 96872.8515625000f, 
96896.3750000000f, 96919.8984375000f, 96943.4218750000f, 96966.9531250000f, 
96990.4843750000f, 97014.0156250000f, 97037.5468750000f, 97061.0781250000f, 
97084.6093750000f, 97108.1484375000f, 97131.6875000000f, 97155.2265625000f, 
97178.7656250000f, 97202.3125000000f, 97225.8515625000f, 97249.3984375000f, 
97272.9453125000f, 97296.4921875000f, 97320.0390625000f, 97343.5937500000f, 
97367.1406250000f, 97390.6953125000f, 97414.2500000000f, 97437.8046875000f, 
97461.3671875000f, 97484.9218750000f, 97508.4843750000f, 97532.0468750000f, 
97555.6093750000f, 97579.1718750000f, 97602.7421875000f, 97626.3125000000f, 
97649.8750000000f, 97673.4531250000f, 97697.0234375000f, 97720.5937500000f, 
97744.1718750000f, 97767.7421875000f, 97791.3203125000f, 97814.9062500000f, 
97838.4843750000f, 97862.0625000000f, 97885.6484375000f, 97909.2343750000f, 
97932.8203125000f, 97956.4062500000f, 97979.9921875000f, 98003.5859375000f, 
98027.1796875000f, 98050.7734375000f, 98074.3671875000f, 98097.9609375000f, 
98121.5625000000f, 98145.1562500000f, 98168.7578125000f, 98192.3593750000f, 
98215.9609375000f, 98239.5703125000f, 98263.1718750000f, 98286.7812500000f, 
98310.3906250000f, 98334.0000000000f, 98357.6171875000f, 98381.2265625000f, 
98404.8437500000f, 98428.4609375000f, 98452.0781250000f, 98475.6953125000f, 
98499.3125000000f, 98522.9375000000f, 98546.5546875000f, 98570.1796875000f, 
98593.8125000000f, 98617.4375000000f, 98641.0625000000f, 98664.6953125000f, 
98688.3281250000f, 98711.9609375000f, 98735.5937500000f, 98759.2265625000f, 
98782.8671875000f, 98806.5078125000f, 98830.1484375000f, 98853.7890625000f, 
98877.4296875000f, 98901.0703125000f, 98924.7187500000f, 98948.3671875000f, 
98972.0156250000f, 98995.6640625000f, 99019.3203125000f, 99042.9687500000f, 
99066.6250000000f, 99090.2812500000f, 99113.9375000000f, 99137.5937500000f, 
99161.2578125000f, 99184.9140625000f, 99208.5781250000f, 99232.2421875000f, 
99255.9062500000f, 99279.5781250000f, 99303.2421875000f, 99326.9140625000f, 
99350.5859375000f, 99374.2578125000f, 99397.9296875000f, 99421.6093750000f, 
99445.2812500000f, 99468.9609375000f, 99492.6406250000f, 99516.3203125000f, 
99540.0000000000f, 99563.6875000000f, 99587.3750000000f, 99611.0625000000f, 
99634.7500000000f, 99658.4375000000f, 99682.1250000000f, 99705.8203125000f, 
99729.5156250000f, 99753.2109375000f, 99776.9062500000f, 99800.6015625000f, 
99824.3046875000f, 99848.0000000000f, 99871.7031250000f, 99895.4062500000f, 
99919.1093750000f, 99942.8203125000f, 99966.5234375000f, 99990.2343750000f, 
100013.9453125000f, 100037.6562500000f, 100061.3750000000f, 100085.0859375000f, 
100108.8046875000f, 100132.5234375000f, 100156.2421875000f, 100179.9609375000f, 
100203.6796875000f, 100227.4062500000f, 100251.1328125000f, 100274.8515625000f, 
100298.5859375000f, 100322.3125000000f, 100346.0390625000f, 100369.7734375000f, 
100393.5078125000f, 100417.2421875000f, 100440.9765625000f, 100464.7109375000f, 
100488.4531250000f, 100512.1953125000f, 100535.9296875000f, 100559.6796875000f, 
100583.4218750000f, 100607.1640625000f, 100630.9140625000f, 100654.6640625000f, 
100678.4140625000f, 100702.1640625000f, 100725.9140625000f, 100749.6718750000f, 
100773.4218750000f, 100797.1796875000f, 100820.9375000000f, 100844.6953125000f, 
100868.4609375000f, 100892.2187500000f, 100915.9843750000f, 100939.7500000000f, 
100963.5156250000f, 100987.2890625000f, 101011.0546875000f, 101034.8281250000f, 
101058.6015625000f, 101082.3750000000f, 101106.1484375000f, 101129.9218750000f, 
101153.7031250000f, 101177.4843750000f, 101201.2578125000f, 101225.0468750000f, 
101248.8281250000f, 101272.6093750000f, 101296.3984375000f, 101320.1875000000f, 
101343.9765625000f, 101367.7656250000f, 101391.5546875000f, 101415.3515625000f, 
101439.1406250000f, 101462.9375000000f, 101486.7343750000f, 101510.5390625000f, 
101534.3359375000f, 101558.1406250000f, 101581.9375000000f, 101605.7421875000f, 
101629.5468750000f, 101653.3593750000f, 101677.1640625000f, 101700.9765625000f, 
101724.7890625000f, 101748.6015625000f, 101772.4140625000f, 101796.2265625000f, 
101820.0468750000f, 101843.8671875000f, 101867.6875000000f, 101891.5078125000f, 
101915.3281250000f, 101939.1484375000f, 101962.9765625000f, 101986.8046875000f, 
102010.6328125000f, 102034.4609375000f, 102058.2890625000f, 102082.1250000000f, 
102105.9609375000f, 102129.7890625000f, 102153.6250000000f, 102177.4687500000f, 
102201.3046875000f, 102225.1484375000f, 102248.9843750000f, 102272.8281250000f, 
102296.6718750000f, 102320.5234375000f, 102344.3671875000f, 102368.2187500000f, 
102392.0703125000f, 102415.9218750000f, 102439.7734375000f, 102463.6250000000f, 
102487.4843750000f, 102511.3437500000f, 102535.1953125000f, 102559.0625000000f, 
102582.9218750000f, 102606.7812500000f, 102630.6484375000f, 102654.5156250000f, 
102678.3828125000f, 102702.2500000000f, 102726.1171875000f, 102749.9921875000f, 
102773.8593750000f, 102797.7343750000f, 102821.6093750000f, 102845.4843750000f, 
102869.3671875000f, 102893.2421875000f, 102917.1250000000f, 102941.0078125000f, 
102964.8906250000f, 102988.7734375000f, 103012.6640625000f, 103036.5468750000f, 
103060.4375000000f, 103084.3281250000f, 103108.2187500000f, 103132.1171875000f, 
103156.0078125000f, 103179.9062500000f, 103203.8046875000f, 103227.7031250000f, 
103251.6015625000f, 103275.5000000000f, 103299.4062500000f, 103323.3125000000f, 
103347.2187500000f, 103371.1250000000f, 103395.0312500000f, 103418.9375000000f, 
103442.8515625000f, 103466.7656250000f, 103490.6796875000f, 103514.5937500000f, 
103538.5078125000f, 103562.4296875000f, 103586.3515625000f, 103610.2656250000f, 
103634.1875000000f, 103658.1171875000f, 103682.0390625000f, 103705.9687500000f, 
103729.8906250000f, 103753.8203125000f, 103777.7500000000f, 103801.6875000000f, 
103825.6171875000f, 103849.5546875000f, 103873.4921875000f, 103897.4296875000f, 
103921.3671875000f, 103945.3046875000f, 103969.2500000000f, 103993.1875000000f, 
104017.1328125000f, 104041.0781250000f, 104065.0312500000f, 104088.9765625000f, 
104112.9296875000f, 104136.8750000000f, 104160.8281250000f, 104184.7812500000f, 
104208.7421875000f, 104232.6953125000f, 104256.6562500000f, 104280.6171875000f, 
104304.5703125000f, 104328.5390625000f, 104352.5000000000f, 104376.4687500000f, 
104400.4296875000f, 104424.3984375000f, 104448.3671875000f, 104472.3359375000f, 
104496.3125000000f, 104520.2812500000f, 104544.2578125000f, 104568.2343750000f, 
104592.2109375000f, 104616.1875000000f, 104640.1718750000f, 104664.1484375000f, 
104688.1328125000f, 104712.1171875000f, 104736.1015625000f, 104760.0937500000f, 
104784.0781250000f, 104808.0703125000f, 104832.0625000000f, 104856.0546875000f, 
104880.0468750000f, 104904.0390625000f, 104928.0390625000f, 104952.0390625000f, 
104976.0390625000f, 105000.0390625000f, 105024.0390625000f, 105048.0390625000f, 
105072.0468750000f, 105096.0546875000f, 105120.0625000000f, 105144.0703125000f, 
105168.0781250000f, 105192.0937500000f, 105216.1015625000f, 105240.1171875000f, 
105264.1328125000f, 105288.1484375000f, 105312.1718750000f, 105336.1875000000f, 
105360.2109375000f, 105384.2343750000f, 105408.2578125000f, 105432.2812500000f, 
105456.3125000000f, 105480.3359375000f, 105504.3671875000f, 105528.3984375000f, 
105552.4296875000f, 105576.4609375000f, 105600.5000000000f, 105624.5390625000f, 
105648.5703125000f, 105672.6093750000f, 105696.6562500000f, 105720.6953125000f, 
105744.7343750000f, 105768.7812500000f, 105792.8281250000f, 105816.8750000000f, 
105840.9218750000f, 105864.9765625000f, 105889.0234375000f, 105913.0781250000f, 
105937.1328125000f, 105961.1875000000f, 105985.2421875000f, 106009.3046875000f, 
106033.3593750000f, 106057.4218750000f, 106081.4843750000f, 106105.5468750000f, 
106129.6171875000f, 106153.6796875000f, 106177.7500000000f, 106201.8203125000f, 
106225.8906250000f, 106249.9609375000f, 106274.0312500000f, 106298.1093750000f, 
106322.1796875000f, 106346.2578125000f, 106370.3359375000f, 106394.4218750000f, 
106418.5000000000f, 106442.5859375000f, 106466.6640625000f, 106490.7500000000f, 
106514.8359375000f, 106538.9296875000f, 106563.0156250000f, 106587.1093750000f, 
106611.2031250000f, 106635.2968750000f, 106659.3906250000f, 106683.4843750000f, 
106707.5859375000f, 106731.6796875000f, 106755.7812500000f, 106779.8828125000f, 
106803.9843750000f, 106828.0937500000f, 106852.1953125000f, 106876.3046875000f, 
106900.4140625000f, 106924.5234375000f, 106948.6328125000f, 106972.7500000000f, 
106996.8593750000f, 107020.9765625000f, 107045.0937500000f, 107069.2109375000f, 
107093.3281250000f, 107117.4531250000f, 107141.5703125000f, 107165.6953125000f, 
107189.8203125000f, 107213.9453125000f, 107238.0781250000f, 107262.2031250000f, 
107286.3359375000f, 107310.4687500000f, 107334.6015625000f, 107358.7343750000f, 
107382.8671875000f, 107407.0078125000f, 107431.1484375000f, 107455.2812500000f, 
107479.4296875000f, 107503.5703125000f, 107527.7109375000f, 107551.8593750000f, 
107576.0078125000f, 107600.1562500000f, 107624.3046875000f, 107648.4531250000f, 
107672.6015625000f, 107696.7578125000f, 107720.9140625000f, 107745.0703125000f, 
107769.2265625000f, 107793.3828125000f, 107817.5468750000f, 107841.7031250000f, 
107865.8671875000f, 107890.0312500000f, 107914.1953125000f, 107938.3671875000f, 
107962.5312500000f, 107986.7031250000f, 108010.8750000000f, 108035.0468750000f, 
108059.2187500000f, 108083.3984375000f, 108107.5703125000f, 108131.7500000000f, 
108155.9296875000f, 108180.1093750000f, 108204.2890625000f, 108228.4765625000f, 
108252.6562500000f, 108276.8437500000f, 108301.0312500000f, 108325.2187500000f, 
108349.4062500000f, 108373.6015625000f, 108397.7968750000f, 108421.9843750000f, 
108446.1796875000f, 108470.3828125000f, 108494.5781250000f, 108518.7734375000f, 
108542.9765625000f, 108567.1796875000f, 108591.3828125000f, 108615.5859375000f, 
108639.7968750000f, 108664.0000000000f, 108688.2109375000f, 108712.4218750000f, 
108736.6328125000f, 108760.8437500000f, 108785.0546875000f, 108809.2734375000f, 
108833.4921875000f, 108857.7109375000f, 108881.9296875000f, 108906.1484375000f, 
108930.3671875000f, 108954.5937500000f, 108978.8203125000f, 109003.0468750000f, 
109027.2734375000f, 109051.5000000000f, 109075.7343750000f, 109099.9609375000f, 
109124.1953125000f, 109148.4296875000f, 109172.6640625000f, 109196.9062500000f, 
109221.1406250000f, 109245.3828125000f, 109269.6250000000f, 109293.8671875000f, 
109318.1093750000f, 109342.3515625000f, 109366.6015625000f, 109390.8515625000f, 
109415.1015625000f, 109439.3515625000f, 109463.6015625000f, 109487.8515625000f, 
109512.1093750000f, 109536.3671875000f, 109560.6171875000f, 109584.8828125000f, 
109609.1406250000f, 109633.3984375000f, 109657.6640625000f, 109681.9296875000f, 
109706.1953125000f, 109730.4609375000f, 109754.7265625000f, 109778.9921875000f, 
109803.2656250000f, 109827.5390625000f, 109851.8125000000f, 109876.0859375000f, 
109900.3593750000f, 109924.6406250000f, 109948.9218750000f, 109973.1953125000f, 
109997.4765625000f, 110021.7656250000f, 110046.0468750000f, 110070.3281250000f, 
110094.6171875000f, 110118.9062500000f, 110143.1953125000f, 110167.4843750000f, 
110191.7812500000f, 110216.0703125000f, 110240.3671875000f, 110264.6640625000f, 
110288.9609375000f, 110313.2578125000f, 110337.5625000000f, 110361.8593750000f, 
110386.1640625000f, 110410.4687500000f, 110434.7734375000f, 110459.0781250000f, 
110483.3906250000f, 110507.6953125000f, 110532.0078125000f, 110556.3203125000f, 
110580.6328125000f, 110604.9453125000f, 110629.2656250000f, 110653.5781250000f, 
110677.8984375000f, 110702.2187500000f, 110726.5390625000f, 110750.8671875000f, 
110775.1875000000f, 110799.5156250000f, 110823.8437500000f, 110848.1718750000f, 
110872.5000000000f, 110896.8281250000f, 110921.1640625000f, 110945.4921875000f, 
110969.8281250000f, 110994.1640625000f, 111018.5078125000f, 111042.8437500000f, 
111067.1796875000f, 111091.5234375000f, 111115.8671875000f, 111140.2109375000f, 
111164.5546875000f, 111188.9062500000f, 111213.2500000000f, 111237.6015625000f, 
111261.9531250000f, 111286.3046875000f, 111310.6562500000f, 111335.0156250000f, 
111359.3671875000f, 111383.7265625000f, 111408.0859375000f, 111432.4453125000f, 
111456.8046875000f, 111481.1718750000f, 111505.5312500000f, 111529.8984375000f, 
111554.2656250000f, 111578.6328125000f, 111603.0000000000f, 111627.3750000000f, 
111651.7500000000f, 111676.1171875000f, 111700.4921875000f, 111724.8671875000f, 
111749.2500000000f, 111773.6250000000f, 111798.0078125000f, 111822.3906250000f, 
111846.7734375000f, 111871.1562500000f, 111895.5390625000f, 111919.9296875000f, 
111944.3125000000f, 111968.7031250000f, 111993.0937500000f, 112017.4843750000f, 
112041.8828125000f, 112066.2734375000f, 112090.6718750000f, 112115.0703125000f, 
112139.4687500000f, 112163.8671875000f, 112188.2734375000f, 112212.6718750000f, 
112237.0781250000f, 112261.4843750000f, 112285.8906250000f, 112310.2968750000f, 
112334.7031250000f, 112359.1171875000f, 112383.5312500000f, 112407.9453125000f, 
112432.3593750000f, 112456.7734375000f, 112481.1875000000f, 112505.6093750000f, 
112530.0312500000f, 112554.4453125000f, 112578.8750000000f, 112603.2968750000f, 
112627.7187500000f, 112652.1484375000f, 112676.5781250000f, 112701.0078125000f, 
112725.4375000000f, 112749.8671875000f, 112774.2968750000f, 112798.7343750000f, 
112823.1718750000f, 112847.6093750000f, 112872.0468750000f, 112896.4843750000f, 
112920.9296875000f, 112945.3671875000f, 112969.8125000000f, 112994.2578125000f, 
113018.7031250000f, 113043.1484375000f, 113067.6015625000f, 113092.0546875000f, 
113116.5000000000f, 113140.9531250000f, 113165.4140625000f, 113189.8671875000f, 
113214.3203125000f, 113238.7812500000f, 113263.2421875000f, 113287.7031250000f, 
113312.1640625000f, 113336.6250000000f, 113361.0937500000f, 113385.5625000000f, 
113410.0234375000f, 113434.4921875000f, 113458.9687500000f, 113483.4375000000f, 
113507.9062500000f, 113532.3828125000f, 113556.8593750000f, 113581.3359375000f, 
113605.8125000000f, 113630.2968750000f, 113654.7734375000f, 113679.2578125000f, 
113703.7421875000f, 113728.2265625000f, 113752.7109375000f, 113777.1953125000f, 
113801.6875000000f, 113826.1796875000f, 113850.6640625000f, 113875.1562500000f, 
113899.6562500000f, 113924.1484375000f, 113948.6484375000f, 113973.1406250000f, 
113997.6406250000f, 114022.1406250000f, 114046.6484375000f, 114071.1484375000f, 
114095.6484375000f, 114120.1562500000f, 114144.6640625000f, 114169.1718750000f, 
114193.6796875000f, 114218.1953125000f, 114242.7031250000f, 114267.2187500000f, 
114291.7343750000f, 114316.2500000000f, 114340.7656250000f, 114365.2890625000f, 
114389.8046875000f, 114414.3281250000f, 114438.8515625000f, 114463.3750000000f, 
114487.8984375000f, 114512.4296875000f, 114536.9531250000f, 114561.4843750000f, 
114586.0156250000f, 114610.5468750000f, 114635.0859375000f, 114659.6171875000f, 
114684.1562500000f, 114708.6875000000f, 114733.2265625000f, 114757.7656250000f, 
114782.3125000000f, 114806.8515625000f, 114831.3984375000f, 114855.9375000000f, 
114880.4843750000f, 114905.0390625000f, 114929.5859375000f, 114954.1328125000f, 
114978.6875000000f, 115003.2421875000f, 115027.7968750000f, 115052.3515625000f, 
115076.9062500000f, 115101.4609375000f, 115126.0234375000f, 115150.5859375000f, 
115175.1484375000f, 115199.7109375000f, 115224.2734375000f, 115248.8437500000f, 
115273.4062500000f, 115297.9765625000f, 115322.5468750000f, 115347.1171875000f, 
115371.6953125000f, 115396.2656250000f, 115420.8437500000f, 115445.4140625000f, 
115469.9921875000f, 115494.5781250000f, 115519.1562500000f, 115543.7343750000f, 
115568.3203125000f, 115592.9062500000f, 115617.4921875000f, 115642.0781250000f, 
115666.6640625000f, 115691.2578125000f, 115715.8437500000f, 115740.4375000000f, 
115765.0312500000f, 115789.6250000000f, 115814.2265625000f, 115838.8203125000f, 
115863.4218750000f, 115888.0234375000f, 115912.6250000000f, 115937.2265625000f, 
115961.8281250000f, 115986.4296875000f, 116011.0390625000f, 116035.6484375000f, 
116060.2578125000f, 116084.8671875000f, 116109.4765625000f, 116134.0937500000f, 
116158.7109375000f, 116183.3203125000f, 116207.9375000000f, 116232.5546875000f, 
116257.1796875000f, 116281.7968750000f, 116306.4218750000f, 116331.0468750000f, 
116355.6718750000f, 116380.2968750000f, 116404.9218750000f, 116429.5546875000f, 
116454.1796875000f, 116478.8125000000f, 116503.4453125000f, 116528.0781250000f, 
116552.7187500000f, 116577.3515625000f, 116601.9921875000f, 116626.6328125000f, 
116651.2734375000f, 116675.9140625000f, 116700.5546875000f, 116725.2031250000f, 
116749.8437500000f, 116774.4921875000f, 116799.1406250000f, 116823.7890625000f, 
116848.4375000000f, 116873.0937500000f, 116897.7500000000f, 116922.3984375000f, 
116947.0546875000f, 116971.7187500000f, 116996.3750000000f, 117021.0312500000f, 
117045.6953125000f, 117070.3593750000f, 117095.0234375000f, 117119.6875000000f, 
117144.3515625000f, 117169.0234375000f, 117193.6875000000f, 117218.3593750000f, 
117243.0312500000f, 117267.7031250000f, 117292.3828125000f, 117317.0546875000f, 
117341.7343750000f, 117366.4140625000f, 117391.0937500000f, 117415.7734375000f, 
117440.4531250000f, 117465.1328125000f, 117489.8203125000f, 117514.5078125000f, 
117539.1953125000f, 117563.8828125000f, 117588.5703125000f, 117613.2656250000f, 
117637.9531250000f, 117662.6484375000f, 117687.3437500000f, 117712.0390625000f, 
117736.7421875000f, 117761.4375000000f, 117786.1406250000f, 117810.8359375000f, 
117835.5390625000f, 117860.2500000000f, 117884.9531250000f, 117909.6562500000f, 
117934.3671875000f, 117959.0781250000f, 117983.7890625000f, 118008.5000000000f, 
118033.2109375000f, 118057.9296875000f, 118082.6406250000f, 118107.3593750000f, 
118132.0781250000f, 118156.7968750000f, 118181.5156250000f, 118206.2421875000f, 
118230.9609375000f, 118255.6875000000f, 118280.4140625000f, 118305.1406250000f, 
118329.8671875000f, 118354.6015625000f, 118379.3281250000f, 118404.0625000000f, 
118428.7968750000f, 118453.5312500000f, 118478.2734375000f, 118503.0078125000f, 
118527.7500000000f, 118552.4843750000f, 118577.2265625000f, 118601.9687500000f, 
118626.7187500000f, 118651.4609375000f, 118676.2109375000f, 118700.9531250000f, 
118725.7031250000f, 118750.4531250000f, 118775.2109375000f, 118799.9609375000f, 
118824.7187500000f, 118849.4687500000f, 118874.2265625000f, 118898.9843750000f, 
118923.7500000000f, 118948.5078125000f, 118973.2734375000f, 118998.0312500000f, 
119022.7968750000f, 119047.5625000000f, 119072.3281250000f, 119097.1015625000f, 
119121.8671875000f, 119146.6406250000f, 119171.4140625000f, 119196.1875000000f, 
119220.9609375000f, 119245.7421875000f, 119270.5156250000f, 119295.2968750000f, 
119320.0781250000f, 119344.8593750000f, 119369.6406250000f, 119394.4218750000f, 
119419.2109375000f, 119444.0000000000f, 119468.7812500000f, 119493.5703125000f, 
119518.3671875000f, 119543.1562500000f, 119567.9531250000f, 119592.7421875000f, 
119617.5390625000f, 119642.3359375000f, 119667.1328125000f, 119691.9375000000f, 
119716.7343750000f, 119741.5390625000f, 119766.3437500000f, 119791.1484375000f, 
119815.9531250000f, 119840.7578125000f, 119865.5703125000f, 119890.3750000000f, 
119915.1875000000f, 119940.0000000000f, 119964.8125000000f, 119989.6328125000f, 
120014.4453125000f, 120039.2656250000f, 120064.0781250000f, 120088.8984375000f, 
120113.7265625000f, 120138.5468750000f, 120163.3671875000f, 120188.1953125000f, 
120213.0234375000f, 120237.8515625000f, 120262.6796875000f, 120287.5078125000f, 
120312.3437500000f, 120337.1718750000f, 120362.0078125000f, 120386.8437500000f, 
120411.6796875000f, 120436.5156250000f, 120461.3593750000f, 120486.1953125000f, 
120511.0390625000f, 120535.8828125000f, 120560.7265625000f, 120585.5703125000f, 
120610.4218750000f, 120635.2656250000f, 120660.1171875000f, 120684.9687500000f, 
120709.8203125000f, 120734.6718750000f, 120759.5312500000f, 120784.3828125000f, 
120809.2421875000f, 120834.1015625000f, 120858.9609375000f, 120883.8203125000f, 
120908.6875000000f, 120933.5468750000f, 120958.4140625000f, 120983.2812500000f, 
121008.1484375000f, 121033.0156250000f, 121057.8828125000f, 121082.7578125000f, 
121107.6250000000f, 121132.5000000000f, 121157.3750000000f, 121182.2500000000f, 
121207.1328125000f, 121232.0078125000f, 121256.8906250000f, 121281.7734375000f, 
121306.6562500000f, 121331.5390625000f, 121356.4218750000f, 121381.3125000000f, 
121406.1953125000f, 121431.0859375000f, 121455.9765625000f, 121480.8671875000f, 
121505.7656250000f, 121530.6562500000f, 121555.5546875000f, 121580.4531250000f, 
121605.3515625000f, 121630.2500000000f, 121655.1484375000f, 121680.0468750000f, 
121704.9531250000f, 121729.8593750000f, 121754.7656250000f, 121779.6718750000f, 
121804.5781250000f, 121829.4843750000f, 121854.3984375000f, 121879.3125000000f, 
121904.2265625000f, 121929.1406250000f, 121954.0546875000f, 121978.9687500000f, 
122003.8906250000f, 122028.8125000000f, 122053.7343750000f, 122078.6562500000f, 
122103.5781250000f, 122128.5000000000f, 122153.4296875000f, 122178.3515625000f, 
122203.2812500000f, 122228.2109375000f, 122253.1484375000f, 122278.0781250000f, 
122303.0078125000f, 122327.9453125000f, 122352.8828125000f, 122377.8203125000f, 
122402.7578125000f, 122427.6953125000f, 122452.6406250000f, 122477.5859375000f, 
122502.5234375000f, 122527.4687500000f, 122552.4218750000f, 122577.3671875000f, 
122602.3125000000f, 122627.2656250000f, 122652.2187500000f, 122677.1718750000f, 
122702.1250000000f, 122727.0781250000f, 122752.0390625000f, 122776.9921875000f, 
122801.9531250000f, 122826.9140625000f, 122851.8750000000f, 122876.8359375000f, 
122901.8046875000f, 122926.7656250000f, 122951.7343750000f, 122976.7031250000f, 
123001.6718750000f, 123026.6406250000f, 123051.6093750000f, 123076.5859375000f, 
123101.5625000000f, 123126.5390625000f, 123151.5156250000f, 123176.4921875000f, 
123201.4687500000f, 123226.4531250000f, 123251.4296875000f, 123276.4140625000f, 
123301.3984375000f, 123326.3828125000f, 123351.3750000000f, 123376.3593750000f, 
123401.3515625000f, 123426.3437500000f, 123451.3359375000f, 123476.3281250000f, 
123501.3203125000f, 123526.3203125000f, 123551.3125000000f, 123576.3125000000f, 
123601.3125000000f, 123626.3125000000f, 123651.3125000000f, 123676.3203125000f, 
123701.3203125000f, 123726.3281250000f, 123751.3359375000f, 123776.3437500000f, 
123801.3515625000f, 123826.3671875000f, 123851.3750000000f, 123876.3906250000f, 
123901.4062500000f, 123926.4218750000f, 123951.4375000000f, 123976.4609375000f, 
124001.4765625000f, 124026.5000000000f, 124051.5234375000f, 124076.5468750000f, 
124101.5703125000f, 124126.5937500000f, 124151.6250000000f, 124176.6562500000f, 
124201.6796875000f, 124226.7109375000f, 124251.7500000000f, 124276.7812500000f, 
124301.8125000000f, 124326.8515625000f, 124351.8906250000f, 124376.9296875000f, 
124401.9687500000f, 124427.0078125000f, 124452.0546875000f, 124477.0937500000f, 
124502.1406250000f, 124527.1875000000f, 124552.2343750000f, 124577.2812500000f, 
124602.3359375000f, 124627.3828125000f, 124652.4375000000f, 124677.4921875000f, 
124702.5468750000f, 124727.6015625000f, 124752.6640625000f, 124777.7187500000f, 
124802.7812500000f, 124827.8437500000f, 124852.9062500000f, 124877.9687500000f, 
124903.0312500000f, 124928.1015625000f, 124953.1640625000f, 124978.2343750000f, 
125003.3046875000f, 125028.3750000000f, 125053.4531250000f, 125078.5234375000f, 
125103.6015625000f, 125128.6796875000f, 125153.7578125000f, 125178.8359375000f, 
125203.9140625000f, 125228.9921875000f, 125254.0781250000f, 125279.1640625000f, 
125304.2500000000f, 125329.3359375000f, 125354.4218750000f, 125379.5078125000f, 
125404.6015625000f, 125429.6953125000f, 125454.7890625000f, 125479.8828125000f, 
125504.9765625000f, 125530.0703125000f, 125555.1718750000f, 125580.2656250000f, 
125605.3671875000f, 125630.4687500000f, 125655.5703125000f, 125680.6796875000f, 
125705.7812500000f, 125730.8906250000f, 125756.0000000000f, 125781.1093750000f, 
125806.2187500000f, 125831.3281250000f, 125856.4375000000f, 125881.5546875000f, 
125906.6718750000f, 125931.7890625000f, 125956.9062500000f, 125982.0234375000f, 
126007.1484375000f, 126032.2656250000f, 126057.3906250000f, 126082.5156250000f, 
126107.6406250000f, 126132.7656250000f, 126157.8906250000f, 126183.0234375000f, 
126208.1562500000f, 126233.2812500000f, 126258.4140625000f, 126283.5546875000f, 
126308.6875000000f, 126333.8203125000f, 126358.9609375000f, 126384.1015625000f, 
126409.2421875000f, 126434.3828125000f, 126459.5234375000f, 126484.6718750000f, 
126509.8125000000f, 126534.9609375000f, 126560.1093750000f, 126585.2578125000f, 
126610.4140625000f, 126635.5625000000f, 126660.7109375000f, 126685.8671875000f, 
126711.0234375000f, 126736.1796875000f, 126761.3359375000f, 126786.5000000000f, 
126811.6562500000f, 126836.8203125000f, 126861.9843750000f, 126887.1484375000f, 
126912.3125000000f, 126937.4765625000f, 126962.6484375000f, 126987.8203125000f, 
127012.9843750000f, 127038.1562500000f, 127063.3281250000f, 127088.5078125000f, 
127113.6796875000f, 127138.8593750000f, 127164.0390625000f, 127189.2109375000f, 
127214.3984375000f, 127239.5781250000f, 127264.7578125000f, 127289.9453125000f, 
127315.1328125000f, 127340.3125000000f, 127365.5000000000f, 127390.6953125000f, 
127415.8828125000f, 127441.0781250000f, 127466.2656250000f, 127491.4609375000f, 
127516.6562500000f, 127541.8515625000f, 127567.0546875000f, 127592.2500000000f, 
127617.4531250000f, 127642.6484375000f, 127667.8515625000f, 127693.0625000000f, 
127718.2656250000f, 127743.4687500000f, 127768.6796875000f, 127793.8906250000f, 
127819.0937500000f, 127844.3125000000f, 127869.5234375000f, 127894.7343750000f, 
127919.9531250000f, 127945.1640625000f, 127970.3828125000f, 127995.6015625000f, 
128020.8203125000f, 128046.0468750000f, 128071.2656250000f, 128096.4921875000f, 
128121.7187500000f, 128146.9453125000f, 128172.1718750000f, 128197.3984375000f, 
128222.6328125000f, 128247.8593750000f, 128273.0937500000f, 128298.3281250000f, 
128323.5625000000f, 128348.7968750000f, 128374.0390625000f, 128399.2734375000f, 
128424.5156250000f, 128449.7578125000f, 128475.0000000000f, 128500.2421875000f, 
128525.4921875000f, 128550.7343750000f, 128575.9843750000f, 128601.2343750000f, 
128626.4843750000f, 128651.7343750000f, 128676.9843750000f, 128702.2421875000f, 
128727.4921875000f, 128752.7500000000f, 128778.0078125000f, 128803.2656250000f, 
128828.5234375000f, 128853.7890625000f, 128879.0468750000f, 128904.3125000000f, 
128929.5781250000f, 128954.8437500000f, 128980.1093750000f, 129005.3828125000f, 
129030.6484375000f, 129055.9218750000f, 129081.1953125000f, 129106.4687500000f, 
129131.7421875000f, 129157.0156250000f, 129182.2968750000f, 129207.5703125000f, 
129232.8515625000f, 129258.1328125000f, 129283.4140625000f, 129308.6953125000f, 
129333.9843750000f, 129359.2656250000f, 129384.5546875000f, 129409.8437500000f, 
129435.1328125000f, 129460.4218750000f, 129485.7187500000f, 129511.0078125000f, 
129536.3046875000f, 129561.6015625000f, 129586.8984375000f, 129612.1953125000f, 
129637.4921875000f, 129662.7968750000f, 129688.0937500000f, 129713.3984375000f, 
129738.7031250000f, 129764.0078125000f, 129789.3203125000f, 129814.6250000000f, 
129839.9375000000f, 129865.2421875000f, 129890.5546875000f, 129915.8671875000f, 
129941.1875000000f, 129966.5000000000f, 129991.8125000000f, 130017.1328125000f, 
130042.4531250000f, 130067.7734375000f, 130093.0937500000f, 130118.4218750000f, 
130143.7421875000f, 130169.0703125000f, 130194.3906250000f, 130219.7187500000f, 
130245.0546875000f, 130270.3828125000f, 130295.7109375000f, 130321.0468750000f, 
130346.3828125000f, 130371.7187500000f, 130397.0546875000f, 130422.3906250000f, 
130447.7265625000f, 130473.0703125000f, 130498.4062500000f, 130523.7500000000f, 
130549.0937500000f, 130574.4375000000f, 130599.7890625000f, 130625.1328125000f, 
130650.4843750000f, 130675.8359375000f, 130701.1875000000f, 130726.5390625000f, 
130751.8906250000f, 130777.2421875000f, 130802.6015625000f, 130827.9609375000f, 
130853.3203125000f, 130878.6796875000f, 130904.0390625000f, 130929.3984375000f, 
130954.7656250000f, 130980.1250000000f, 131005.4921875000f, 131030.8593750000f, 
131056.2265625000f, 131081.5937500000f, 131106.9687500000f, 131132.3437500000f, 
131157.7187500000f, 131183.0937500000f, 131208.4687500000f, 131233.8437500000f, 
131259.2187500000f, 131284.5937500000f, 131309.9843750000f, 131335.3593750000f, 
131360.7500000000f, 131386.1250000000f, 131411.5156250000f, 131436.9062500000f, 
131462.2968750000f, 131487.6875000000f, 131513.0781250000f, 131538.4687500000f, 
131563.8593750000f, 131589.2500000000f, 131614.6406250000f, 131640.0468750000f, 
131665.4375000000f, 131690.8437500000f, 131716.2343750000f, 131741.6406250000f, 
131767.0468750000f, 131792.4531250000f, 131817.8593750000f, 131843.2656250000f, 
131868.6718750000f, 131894.0781250000f, 131919.4843750000f, 131944.8906250000f, 
131970.3125000000f, 131995.7187500000f, 132021.1406250000f, 132046.5468750000f, 
132071.9687500000f, 132097.3906250000f, 132122.8125000000f, 132148.2343750000f, 
132173.6562500000f, 132199.0781250000f, 132224.5000000000f, 132249.9218750000f, 
132275.3593750000f, 132300.7812500000f, 132326.2187500000f, 132351.6406250000f, 
132377.0781250000f, 132402.5156250000f, 132427.9375000000f, 132453.3750000000f, 
132478.8125000000f, 132504.2500000000f, 132529.6875000000f, 132555.1406250000f, 
132580.5781250000f, 132606.0156250000f, 132631.4687500000f, 132656.9062500000f, 
132682.3593750000f, 132707.7968750000f, 132733.2500000000f, 132758.7031250000f, 
132784.1562500000f, 132809.6093750000f, 132835.0625000000f, 132860.5156250000f, 
132885.9687500000f, 132911.4375000000f, 132936.8906250000f, 132962.3437500000f, 
132987.8125000000f, 133013.2656250000f, 133038.7343750000f, 133064.2031250000f, 
133089.6718750000f, 133115.1406250000f, 133140.6093750000f, 133166.0781250000f, 
133191.5468750000f, 133217.0156250000f, 133242.4843750000f, 133267.9687500000f, 
133293.4375000000f, 133318.9218750000f, 133344.3906250000f, 133369.8750000000f, 
133395.3593750000f, 133420.8437500000f, 133446.3281250000f, 133471.8125000000f, 
133497.2968750000f, 133522.7812500000f, 133548.2656250000f, 133573.7500000000f, 
133599.2500000000f, 133624.7343750000f, 133650.2343750000f, 133675.7343750000f, 
133701.2187500000f, 133726.7187500000f, 133752.2187500000f, 133777.7187500000f, 
133803.2187500000f, 133828.7187500000f, 133854.2187500000f, 133879.7187500000f, 
133905.2343750000f, 133930.7343750000f, 133956.2500000000f, 133981.7500000000f, 
134007.2656250000f, 134032.7812500000f, 134058.2812500000f, 134083.7968750000f, 
134109.3125000000f, 134134.8281250000f, 134160.3437500000f, 134185.8593750000f, 
134211.3906250000f, 134236.9062500000f, 134262.4218750000f, 134287.9531250000f, 
134313.4843750000f, 134339.0000000000f, 134364.5312500000f, 134390.0625000000f, 
134415.5937500000f, 134441.1250000000f, 134466.6562500000f, 134492.1875000000f, 
134517.7187500000f, 134543.2500000000f, 134568.7968750000f, 134594.3281250000f, 
134619.8593750000f, 134645.4062500000f, 134670.9531250000f, 134696.4843750000f, 
134722.0312500000f, 134747.5781250000f, 134773.1250000000f, 134798.6718750000f, 
134824.2187500000f, 134849.7656250000f, 134875.3281250000f, 134900.8750000000f, 
134926.4375000000f, 134951.9843750000f, 134977.5468750000f, 135003.0937500000f, 
135028.6562500000f, 135054.2187500000f, 135079.7812500000f, 135105.3437500000f, 
135130.9062500000f, 135156.4687500000f, 135182.0312500000f, 135207.5937500000f, 
135233.1718750000f, 135258.7343750000f, 135284.3125000000f, 135309.8750000000f, 
135335.4531250000f, 135361.0312500000f, 135386.6093750000f, 135412.1718750000f, 
135437.7500000000f, 135463.3437500000f, 135488.9218750000f, 135514.5000000000f, 
135540.0781250000f, 135565.6718750000f, 135591.2500000000f, 135616.8437500000f, 
135642.4218750000f, 135668.0156250000f, 135693.6093750000f, 135719.1875000000f, 
135744.7812500000f, 135770.3750000000f, 135795.9687500000f, 135821.5625000000f, 
135847.1718750000f, 135872.7656250000f, 135898.3593750000f, 135923.9687500000f, 
135949.5625000000f, 135975.1718750000f, 136000.7812500000f, 136026.3750000000f, 
136051.9843750000f, 136077.5937500000f, 136103.2031250000f, 136128.8125000000f, 
136154.4218750000f, 136180.0468750000f, 136205.6562500000f, 136231.2656250000f, 
136256.8906250000f, 136282.5000000000f, 136308.1250000000f, 136333.7343750000f, 
136359.3593750000f, 136384.9843750000f, 136410.6093750000f, 136436.2343750000f, 
136461.8593750000f, 136487.4843750000f, 136513.1093750000f, 136538.7500000000f, 
136564.3750000000f, 136590.0000000000f, 136615.6406250000f, 136641.2812500000f, 
136666.9062500000f, 136692.5468750000f, 136718.1875000000f, 136743.8281250000f, 
136769.4687500000f, 136795.1093750000f, 136820.7500000000f, 136846.3906250000f, 
136872.0468750000f, 136897.6875000000f, 136923.3281250000f, 136948.9843750000f, 
136974.6250000000f, 137000.2812500000f, 137025.9375000000f, 137051.5937500000f, 
137077.2500000000f, 137102.9062500000f, 137128.5625000000f, 137154.2187500000f, 
137179.8750000000f, 137205.5312500000f, 137231.2031250000f, 137256.8593750000f, 
137282.5312500000f, 137308.1875000000f, 137333.8593750000f, 137359.5312500000f, 
137385.2031250000f, 137410.8750000000f, 137436.5468750000f, 137462.2187500000f, 
137487.8906250000f, 137513.5625000000f, 137539.2343750000f, 137564.9218750000f, 
137590.5937500000f, 137616.2812500000f, 137641.9531250000f, 137667.6406250000f, 
137693.3281250000f, 137719.0000000000f, 137744.6875000000f, 137770.3750000000f, 
137796.0625000000f, 137821.7656250000f, 137847.4531250000f, 137873.1406250000f, 
137898.8281250000f, 137924.5312500000f, 137950.2187500000f, 137975.9218750000f, 
138001.6250000000f, 138027.3125000000f, 138053.0156250000f, 138078.7187500000f, 
138104.4218750000f, 138130.1250000000f, 138155.8281250000f, 138181.5468750000f, 
138207.2500000000f, 138232.9531250000f, 138258.6718750000f, 138284.3750000000f, 
138310.0937500000f, 138335.7968750000f, 138361.5156250000f, 138387.2343750000f, 
138412.9531250000f, 138438.6718750000f, 138464.3906250000f, 138490.1093750000f, 
138515.8281250000f, 138541.5468750000f, 138567.2812500000f, 138593.0000000000f, 
138618.7343750000f, 138644.4531250000f, 138670.1875000000f, 138695.9218750000f, 
138721.6562500000f, 138747.3750000000f, 138773.1093750000f, 138798.8437500000f, 
138824.5937500000f, 138850.3281250000f, 138876.0625000000f, 138901.7968750000f, 
138927.5468750000f, 138953.2812500000f, 138979.0312500000f, 139004.7812500000f, 
139030.5156250000f, 139056.2656250000f, 139082.0156250000f, 139107.7656250000f, 
139133.5156250000f, 139159.2656250000f, 139185.0156250000f, 139210.7812500000f, 
139236.5312500000f, 139262.2812500000f, 139288.0468750000f, 139313.7968750000f, 
139339.5625000000f, 139365.3281250000f, 139391.0937500000f, 139416.8437500000f, 
139442.6093750000f, 139468.3750000000f, 139494.1562500000f, 139519.9218750000f, 
139545.6875000000f, 139571.4531250000f, 139597.2343750000f, 139623.0000000000f, 
139648.7812500000f, 139674.5468750000f, 139700.3281250000f, 139726.1093750000f, 
139751.8906250000f, 139777.6718750000f, 139803.4531250000f, 139829.2343750000f, 
139855.0156250000f, 139880.7968750000f, 139906.5937500000f, 139932.3750000000f, 
139958.1562500000f, 139983.9531250000f, 140009.7500000000f, 140035.5312500000f, 
140061.3281250000f, 140087.1250000000f, 140112.9218750000f, 140138.7187500000f, 
140164.5156250000f, 140190.3125000000f, 140216.1093750000f, 140241.9218750000f, 
140267.7187500000f, 140293.5156250000f, 140319.3281250000f, 140345.1406250000f, 
140370.9375000000f, 140396.7500000000f, 140422.5625000000f, 140448.3750000000f, 
140474.1875000000f, 140500.0000000000f, 140525.8125000000f, 140551.6250000000f, 
140577.4531250000f, 140603.2656250000f, 140629.0781250000f, 140654.9062500000f, 
140680.7187500000f, 140706.5468750000f, 140732.3750000000f, 140758.2031250000f, 
140784.0312500000f, 140809.8593750000f, 140835.6875000000f, 140861.5156250000f, 
140887.3437500000f, 140913.1718750000f, 140939.0156250000f, 140964.8437500000f, 
140990.6875000000f, 141016.5156250000f, 141042.3593750000f, 141068.2031250000f, 
141094.0312500000f, 141119.8750000000f, 141145.7187500000f, 141171.5625000000f, 
141197.4062500000f, 141223.2656250000f, 141249.1093750000f, 141274.9531250000f, 
141300.8125000000f, 141326.6562500000f, 141352.5156250000f, 141378.3593750000f, 
141404.2187500000f, 141430.0781250000f, 141455.9375000000f, 141481.7968750000f, 
141507.6562500000f, 141533.5156250000f, 141559.3750000000f, 141585.2343750000f, 
141611.0937500000f, 141636.9687500000f, 141662.8281250000f, 141688.7031250000f, 
141714.5781250000f, 141740.4375000000f, 141766.3125000000f, 141792.1875000000f, 
141818.0625000000f, 141843.9375000000f, 141869.8125000000f, 141895.6875000000f, 
141921.5625000000f, 141947.4531250000f, 141973.3281250000f, 141999.2031250000f, 
142025.0937500000f, 142050.9843750000f, 142076.8593750000f, 142102.7500000000f, 
142128.6406250000f, 142154.5312500000f, 142180.4218750000f, 142206.3125000000f, 
142232.2031250000f, 142258.0937500000f, 142283.9843750000f, 142309.8906250000f, 
142335.7812500000f, 142361.6875000000f, 142387.5781250000f, 142413.4843750000f, 
142439.3906250000f, 142465.2968750000f, 142491.1875000000f, 142517.0937500000f, 
142543.0000000000f, 142568.9218750000f, 142594.8281250000f, 142620.7343750000f, 
142646.6406250000f, 142672.5625000000f, 142698.4687500000f, 142724.3906250000f, 
142750.3125000000f, 142776.2187500000f, 142802.1406250000f, 142828.0625000000f, 
142853.9843750000f, 142879.9062500000f, 142905.8281250000f, 142931.7500000000f, 
142957.6718750000f, 142983.6093750000f, 143009.5312500000f, 143035.4687500000f, 
143061.3906250000f, 143087.3281250000f, 143113.2500000000f, 143139.1875000000f, 
143165.1250000000f, 143191.0625000000f, 143217.0000000000f, 143242.9375000000f, 
143268.8750000000f, 143294.8125000000f, 143320.7656250000f, 143346.7031250000f, 
143372.6562500000f, 143398.5937500000f, 143424.5468750000f, 143450.4843750000f, 
143476.4375000000f, 143502.3906250000f, 143528.3437500000f, 143554.2968750000f, 
143580.2500000000f, 143606.2031250000f, 143632.1562500000f, 143658.1093750000f, 
143684.0781250000f, 143710.0312500000f, 143736.0000000000f, 143761.9531250000f, 
143787.9218750000f, 143813.8906250000f, 143839.8437500000f, 143865.8125000000f, 
143891.7812500000f, 143917.7500000000f, 143943.7187500000f, 143969.7031250000f, 
143995.6718750000f, 144021.6406250000f, 144047.6250000000f, 144073.5937500000f, 
144099.5781250000f, 144125.5468750000f, 144151.5312500000f, 144177.5156250000f, 
144203.5000000000f, 144229.4687500000f, 144255.4531250000f, 144281.4531250000f, 
144307.4375000000f, 144333.4218750000f, 144359.4062500000f, 144385.4062500000f, 
144411.3906250000f, 144437.3906250000f, 144463.3750000000f, 144489.3750000000f, 
144515.3750000000f, 144541.3593750000f, 144567.3593750000f, 144593.3593750000f, 
144619.3593750000f, 144645.3593750000f, 144671.3750000000f, 144697.3750000000f, 
144723.3750000000f, 144749.3906250000f, 144775.3906250000f, 144801.4062500000f, 
144827.4062500000f, 144853.4218750000f, 144879.4375000000f, 144905.4531250000f, 
144931.4687500000f, 144957.4843750000f, 144983.5000000000f, 145009.5156250000f, 
145035.5312500000f, 145061.5625000000f, 145087.5781250000f, 145113.5937500000f, 
145139.6250000000f, 145165.6562500000f, 145191.6718750000f, 145217.7031250000f, 
145243.7343750000f, 145269.7656250000f, 145295.7968750000f, 145321.8281250000f, 
145347.8593750000f, 145373.8906250000f, 145399.9218750000f, 145425.9687500000f, 
145452.0000000000f, 145478.0468750000f, 145504.0781250000f, 145530.1250000000f, 
145556.1718750000f, 145582.2187500000f, 145608.2500000000f, 145634.2968750000f, 
145660.3437500000f, 145686.4062500000f, 145712.4531250000f, 145738.5000000000f, 
145764.5468750000f, 145790.6093750000f, 145816.6562500000f, 145842.7187500000f, 
145868.7656250000f, 145894.8281250000f, 145920.8906250000f, 145946.9531250000f, 
145973.0156250000f, 145999.0781250000f, 146025.1406250000f, 146051.2031250000f, 
146077.2656250000f, 146103.3281250000f, 146129.4062500000f, 146155.4687500000f, 
146181.5468750000f, 146207.6093750000f, 146233.6875000000f, 146259.7656250000f, 
146285.8437500000f, 146311.9218750000f, 146338.0000000000f, 146364.0781250000f, 
146390.1562500000f, 146416.2343750000f, 146442.3125000000f, 146468.3906250000f, 
146494.4843750000f, 146520.5625000000f, 146546.6562500000f, 146572.7500000000f, 
146598.8281250000f, 146624.9218750000f, 146651.0156250000f, 146677.1093750000f, 
146703.2031250000f, 146729.2968750000f, 146755.3906250000f, 146781.4843750000f, 
146807.5937500000f, 146833.6875000000f, 146859.7968750000f, 146885.8906250000f, 
146912.0000000000f, 146938.0937500000f, 146964.2031250000f, 146990.3125000000f, 
147016.4218750000f, 147042.5312500000f, 147068.6406250000f, 147094.7500000000f, 
147120.8593750000f, 147146.9687500000f, 147173.0937500000f, 147199.2031250000f, 
147225.3281250000f, 147251.4375000000f, 147277.5625000000f, 147303.6875000000f, 
147329.7968750000f, 147355.9218750000f, 147382.0468750000f, 147408.1718750000f, 
147434.2968750000f, 147460.4218750000f, 147486.5625000000f, 147512.6875000000f, 
147538.8125000000f, 147564.9531250000f, 147591.0781250000f, 147617.2187500000f, 
147643.3593750000f, 147669.4843750000f, 147695.6250000000f, 147721.7656250000f, 
147747.9062500000f, 147774.0468750000f, 147800.1875000000f, 147826.3281250000f, 
147852.4843750000f, 147878.6250000000f, 147904.7812500000f, 147930.9218750000f, 
147957.0781250000f, 147983.2187500000f, 148009.3750000000f, 148035.5312500000f, 
148061.6875000000f, 148087.8437500000f, 148114.0000000000f, 148140.1562500000f, 
148166.3125000000f, 148192.4687500000f, 148218.6250000000f, 148244.7968750000f, 
148270.9531250000f, 148297.1250000000f, 148323.2812500000f, 148349.4531250000f, 
148375.6250000000f, 148401.7968750000f, 148427.9531250000f, 148454.1250000000f, 
148480.2968750000f, 148506.4843750000f, 148532.6562500000f, 148558.8281250000f, 
148585.0000000000f, 148611.1875000000f, 148637.3593750000f, 148663.5468750000f, 
148689.7187500000f, 148715.9062500000f, 148742.0937500000f, 148768.2812500000f, 
148794.4687500000f, 148820.6562500000f, 148846.8437500000f, 148873.0312500000f, 
148899.2187500000f, 148925.4218750000f, 148951.6093750000f, 148977.7968750000f, 
149004.0000000000f, 149030.1875000000f, 149056.3906250000f, 149082.5937500000f, 
149108.7968750000f, 149135.0000000000f, 149161.2031250000f, 149187.4062500000f, 
149213.6093750000f, 149239.8125000000f, 149266.0156250000f, 149292.2187500000f, 
149318.4375000000f, 149344.6406250000f, 149370.8593750000f, 149397.0625000000f, 
149423.2812500000f, 149449.5000000000f, 149475.7187500000f, 149501.9375000000f, 
149528.1562500000f, 149554.3750000000f, 149580.5937500000f, 149606.8125000000f, 
149633.0312500000f, 149659.2656250000f, 149685.4843750000f, 149711.7187500000f, 
149737.9375000000f, 149764.1718750000f, 149790.4062500000f, 149816.6250000000f, 
149842.8593750000f, 149869.0937500000f, 149895.3281250000f, 149921.5625000000f, 
149947.8125000000f, 149974.0468750000f, 150000.2812500000f, 150026.5312500000f, 
150052.7656250000f, 150079.0156250000f, 150105.2500000000f, 150131.5000000000f, 
150157.7500000000f, 150183.9843750000f, 150210.2343750000f, 150236.4843750000f, 
150262.7343750000f, 150288.9843750000f, 150315.2500000000f, 150341.5000000000f, 
150367.7500000000f, 150394.0156250000f, 150420.2656250000f, 150446.5312500000f, 
150472.7812500000f, 150499.0468750000f, 150525.3125000000f, 150551.5781250000f, 
150577.8437500000f, 150604.1093750000f, 150630.3750000000f, 150656.6406250000f, 
150682.9062500000f, 150709.1718750000f, 150735.4531250000f, 150761.7187500000f, 
150788.0000000000f, 150814.2656250000f, 150840.5468750000f, 150866.8281250000f, 
150893.1093750000f, 150919.3750000000f, 150945.6562500000f, 150971.9375000000f, 
150998.2343750000f, 151024.5156250000f, 151050.7968750000f, 151077.0781250000f, 
151103.3750000000f, 151129.6562500000f, 151155.9531250000f, 151182.2343750000f, 
151208.5312500000f, 151234.8281250000f, 151261.1250000000f, 151287.4062500000f, 
151313.7031250000f, 151340.0000000000f, 151366.3125000000f, 151392.6093750000f, 
151418.9062500000f, 151445.2031250000f, 151471.5156250000f, 151497.8125000000f, 
151524.1250000000f, 151550.4375000000f, 151576.7343750000f, 151603.0468750000f, 
151629.3593750000f, 151655.6718750000f, 151681.9843750000f, 151708.2968750000f, 
151734.6093750000f, 151760.9218750000f, 151787.2500000000f, 151813.5625000000f, 
151839.8750000000f, 151866.2031250000f, 151892.5156250000f, 151918.8437500000f, 
151945.1718750000f, 151971.5000000000f, 151997.8281250000f, 152024.1562500000f, 
152050.4843750000f, 152076.8125000000f, 152103.1406250000f, 152129.4687500000f, 
152155.7968750000f, 152182.1406250000f, 152208.4687500000f, 152234.8125000000f, 
152261.1406250000f, 152287.4843750000f, 152313.8281250000f, 152340.1718750000f, 
152366.5156250000f, 152392.8593750000f, 152419.2031250000f, 152445.5468750000f, 
152471.8906250000f, 152498.2343750000f, 152524.5781250000f, 152550.9375000000f, 
152577.2812500000f, 152603.6406250000f, 152630.0000000000f, 152656.3437500000f, 
152682.7031250000f, 152709.0625000000f, 152735.4218750000f, 152761.7812500000f, 
152788.1406250000f, 152814.5000000000f, 152840.8593750000f, 152867.2187500000f, 
152893.5937500000f, 152919.9531250000f, 152946.3281250000f, 152972.6875000000f, 
152999.0625000000f, 153025.4375000000f, 153051.8125000000f, 153078.1718750000f, 
153104.5468750000f, 153130.9218750000f, 153157.2968750000f, 153183.6875000000f, 
153210.0625000000f, 153236.4375000000f, 153262.8125000000f, 153289.2031250000f, 
153315.5781250000f, 153341.9687500000f, 153368.3593750000f, 153394.7343750000f, 
153421.1250000000f, 153447.5156250000f, 153473.9062500000f, 153500.2968750000f, 
153526.6875000000f, 153553.0781250000f, 153579.4843750000f, 153605.8750000000f, 
153632.2656250000f, 153658.6718750000f, 153685.0625000000f, 153711.4687500000f, 
153737.8750000000f, 153764.2656250000f, 153790.6718750000f, 153817.0781250000f, 
153843.4843750000f, 153869.8906250000f, 153896.2968750000f, 153922.7031250000f, 
153949.1250000000f, 153975.5312500000f, 154001.9375000000f, 154028.3593750000f, 
154054.7656250000f, 154081.1875000000f, 154107.6093750000f, 154134.0312500000f, 
154160.4375000000f, 154186.8593750000f, 154213.2812500000f, 154239.7031250000f, 
154266.1250000000f, 154292.5625000000f, 154318.9843750000f, 154345.4062500000f, 
154371.8437500000f, 154398.2656250000f, 154424.7031250000f, 154451.1250000000f, 
154477.5625000000f, 154504.0000000000f, 154530.4375000000f, 154556.8750000000f, 
154583.3125000000f, 154609.7500000000f, 154636.1875000000f, 154662.6250000000f, 
154689.0625000000f, 154715.5156250000f, 154741.9531250000f, 154768.4062500000f, 
154794.8437500000f, 154821.2968750000f, 154847.7500000000f, 154874.1875000000f, 
154900.6406250000f, 154927.0937500000f, 154953.5468750000f, 154980.0000000000f, 
155006.4687500000f, 155032.9218750000f, 155059.3750000000f, 155085.8281250000f, 
155112.2968750000f, 155138.7500000000f, 155165.2187500000f, 155191.6875000000f, 
155218.1406250000f, 155244.6093750000f, 155271.0781250000f, 155297.5468750000f, 
155324.0156250000f, 155350.4843750000f, 155376.9531250000f, 155403.4375000000f, 
155429.9062500000f, 155456.3750000000f, 155482.8593750000f, 155509.3281250000f, 
155535.8125000000f, 155562.2812500000f, 155588.7656250000f, 155615.2500000000f, 
155641.7343750000f, 155668.2187500000f, 155694.7031250000f, 155721.1875000000f, 
155747.6718750000f, 155774.1562500000f, 155800.6562500000f, 155827.1406250000f, 
155853.6406250000f, 155880.1250000000f, 155906.6250000000f, 155933.1093750000f, 
155959.6093750000f, 155986.1093750000f, 156012.6093750000f, 156039.1093750000f, 
156065.6093750000f, 156092.1093750000f, 156118.6093750000f, 156145.1093750000f, 
156171.6250000000f, 156198.1250000000f, 156224.6406250000f, 156251.1406250000f, 
156277.6562500000f, 156304.1718750000f, 156330.6718750000f, 156357.1875000000f, 
156383.7031250000f, 156410.2187500000f, 156436.7343750000f, 156463.2500000000f, 
156489.7656250000f, 156516.2968750000f, 156542.8125000000f, 156569.3281250000f, 
156595.8593750000f, 156622.3750000000f, 156648.9062500000f, 156675.4375000000f, 
156701.9687500000f, 156728.4843750000f, 156755.0156250000f, 156781.5468750000f, 
156808.0781250000f, 156834.6093750000f, 156861.1562500000f, 156887.6875000000f, 
156914.2187500000f, 156940.7656250000f, 156967.2968750000f, 156993.8437500000f, 
157020.3750000000f, 157046.9218750000f, 157073.4687500000f, 157100.0156250000f, 
157126.5625000000f, 157153.1093750000f, 157179.6562500000f, 157206.2031250000f, 
157232.7500000000f, 157259.2968750000f, 157285.8593750000f, 157312.4062500000f, 
157338.9531250000f, 157365.5156250000f, 157392.0781250000f, 157418.6250000000f, 
157445.1875000000f, 157471.7500000000f, 157498.3125000000f, 157524.8750000000f, 
157551.4375000000f, 157578.0000000000f, 157604.5625000000f, 157631.1406250000f, 
157657.7031250000f, 157684.2656250000f, 157710.8437500000f, 157737.4062500000f, 
157763.9843750000f, 157790.5625000000f, 157817.1406250000f, 157843.7031250000f, 
157870.2812500000f, 157896.8593750000f, 157923.4375000000f, 157950.0312500000f, 
157976.6093750000f, 158003.1875000000f, 158029.7656250000f, 158056.3593750000f, 
158082.9375000000f, 158109.5312500000f, 158136.1250000000f, 158162.7031250000f, 
158189.2968750000f, 158215.8906250000f, 158242.4843750000f, 158269.0781250000f, 
158295.6718750000f, 158322.2656250000f, 158348.8593750000f, 158375.4531250000f, 
158402.0625000000f, 158428.6562500000f, 158455.2656250000f, 158481.8593750000f, 
158508.4687500000f, 158535.0781250000f, 158561.6718750000f, 158588.2812500000f, 
158614.8906250000f, 158641.5000000000f, 158668.1093750000f, 158694.7187500000f, 
158721.3437500000f, 158747.9531250000f, 158774.5625000000f, 158801.1875000000f, 
158827.7968750000f, 158854.4218750000f, 158881.0312500000f, 158907.6562500000f, 
158934.2812500000f, 158960.9062500000f, 158987.5312500000f, 159014.1562500000f, 
159040.7812500000f, 159067.4062500000f, 159094.0312500000f, 159120.6562500000f, 
159147.2968750000f, 159173.9218750000f, 159200.5625000000f, 159227.1875000000f, 
159253.8281250000f, 159280.4687500000f, 159307.0937500000f, 159333.7343750000f, 
159360.3750000000f, 159387.0156250000f, 159413.6562500000f, 159440.2968750000f, 
159466.9531250000f, 159493.5937500000f, 159520.2343750000f, 159546.8906250000f, 
159573.5312500000f, 159600.1875000000f, 159626.8281250000f, 159653.4843750000f, 
159680.1406250000f, 159706.7968750000f, 159733.4531250000f, 159760.1093750000f, 
159786.7656250000f, 159813.4218750000f, 159840.0781250000f, 159866.7343750000f, 
159893.4062500000f, 159920.0625000000f, 159946.7187500000f, 159973.3906250000f, 
160000.0625000000f, 160026.7187500000f, 160053.3906250000f, 160080.0625000000f, 
160106.7343750000f, 160133.4062500000f, 160160.0781250000f, 160186.7500000000f, 
160213.4218750000f, 160240.1093750000f, 160266.7812500000f, 160293.4531250000f, 
160320.1406250000f, 160346.8125000000f, 160373.5000000000f, 160400.1875000000f, 
160426.8593750000f, 160453.5468750000f, 160480.2343750000f, 160506.9218750000f, 
160533.6093750000f, 160560.2968750000f, 160587.0000000000f, 160613.6875000000f, 
160640.3750000000f, 160667.0781250000f, 160693.7656250000f, 160720.4687500000f, 
160747.1562500000f, 160773.8593750000f, 160800.5625000000f, 160827.2500000000f, 
160853.9531250000f, 160880.6562500000f, 160907.3593750000f, 160934.0781250000f, 
160960.7812500000f, 160987.4843750000f, 161014.1875000000f, 161040.9062500000f, 
161067.6093750000f, 161094.3281250000f, 161121.0312500000f, 161147.7500000000f, 
161174.4687500000f, 161201.1875000000f, 161227.8906250000f, 161254.6093750000f, 
161281.3281250000f, 161308.0625000000f, 161334.7812500000f, 161361.5000000000f, 
161388.2187500000f, 161414.9531250000f, 161441.6718750000f, 161468.4062500000f, 
161495.1250000000f, 161521.8593750000f, 161548.5937500000f, 161575.3281250000f, 
161602.0468750000f, 161628.7812500000f, 161655.5156250000f, 161682.2656250000f, 
161709.0000000000f, 161735.7343750000f, 161762.4687500000f, 161789.2187500000f, 
161815.9531250000f, 161842.7031250000f, 161869.4375000000f, 161896.1875000000f, 
161922.9375000000f, 161949.6718750000f, 161976.4218750000f, 162003.1718750000f, 
162029.9218750000f, 162056.6718750000f, 162083.4375000000f, 162110.1875000000f, 
162136.9375000000f, 162163.6875000000f, 162190.4531250000f, 162217.2031250000f, 
162243.9687500000f, 162270.7343750000f, 162297.4843750000f, 162324.2500000000f, 
162351.0156250000f, 162377.7812500000f, 162404.5468750000f, 162431.3125000000f, 
162458.0781250000f, 162484.8437500000f, 162511.6250000000f, 162538.3906250000f, 
162565.1718750000f, 162591.9375000000f, 162618.7187500000f, 162645.4843750000f, 
162672.2656250000f, 162699.0468750000f, 162725.8281250000f, 162752.6093750000f, 
162779.3906250000f, 162806.1718750000f, 162832.9531250000f, 162859.7343750000f, 
162886.5156250000f, 162913.3125000000f, 162940.0937500000f, 162966.8750000000f, 
162993.6718750000f, 163020.4687500000f, 163047.2500000000f, 163074.0468750000f, 
163100.8437500000f, 163127.6406250000f, 163154.4375000000f, 163181.2343750000f, 
163208.0312500000f, 163234.8281250000f, 163261.6250000000f, 163288.4375000000f, 
163315.2343750000f, 163342.0468750000f, 163368.8437500000f, 163395.6562500000f, 
163422.4687500000f, 163449.2656250000f, 163476.0781250000f, 163502.8906250000f, 
163529.7031250000f, 163556.5156250000f, 163583.3281250000f, 163610.1406250000f, 
163636.9687500000f, 163663.7812500000f, 163690.5937500000f, 163717.4218750000f, 
163744.2343750000f, 163771.0625000000f, 163797.8906250000f, 163824.7031250000f, 
163851.5312500000f, 163878.3593750000f, 163905.1875000000f, 163932.0156250000f, 
163958.8437500000f, 163985.6718750000f, 164012.5000000000f, 164039.3437500000f, 
164066.1718750000f, 164093.0156250000f, 164119.8437500000f, 164146.6875000000f, 
164173.5156250000f, 164200.3593750000f, 164227.2031250000f, 164254.0468750000f, 
164280.8906250000f, 164307.7343750000f, 164334.5781250000f, 164361.4218750000f, 
164388.2656250000f, 164415.1093750000f, 164441.9687500000f, 164468.8125000000f, 
164495.6718750000f, 164522.5156250000f, 164549.3750000000f, 164576.2343750000f, 
164603.0781250000f, 164629.9375000000f, 164656.7968750000f, 164683.6562500000f, 
164710.5156250000f, 164737.3750000000f, 164764.2343750000f, 164791.1093750000f, 
164817.9687500000f, 164844.8281250000f, 164871.7031250000f, 164898.5781250000f, 
164925.4375000000f, 164952.3125000000f, 164979.1875000000f, 165006.0468750000f, 
165032.9218750000f, 165059.7968750000f, 165086.6718750000f, 165113.5468750000f, 
165140.4375000000f, 165167.3125000000f, 165194.1875000000f, 165221.0781250000f, 
165247.9531250000f, 165274.8437500000f, 165301.7187500000f, 165328.6093750000f, 
165355.5000000000f, 165382.3750000000f, 165409.2656250000f, 165436.1562500000f, 
165463.0468750000f, 165489.9375000000f, 165516.8281250000f
};

/* pre-calculated cos() in steps of PI/72, PI/18 and PI/24 for MDCT calcs. 
 * The Octave formula to generate each table is given in the comment above it */
/* 0.5 ./ cos ((1:12) .* pi/24) */
static const gfloat cos24_table[] = {
  5.043144802900764167574720886477734893560409545898437500000000e-01f,
  5.176380902050414789528076653368771076202392578125000000000000e-01f,
  5.411961001461970122150546558259520679712295532226562500000000e-01f,
  5.773502691896257310588680411456152796745300292968750000000000e-01f,
  6.302362070051322762154200063378084450960159301757812500000000e-01f,
  7.071067811865474617150084668537601828575134277343750000000000e-01f,
  8.213398158522907666068135768000502139329910278320312500000000e-01f,
  9.999999999999997779553950749686919152736663818359375000000000e-01f,
  1.306562964876376353728915091778617352247238159179687500000000e+00f,
  1.931851652578136846472034449107013642787933349609375000000000e+00f,
  3.830648787770190910606515899416990578174591064453125000000000e+00f,
  8.165889364191922000000000000000000000000000000000000000000000e+15f
};

/* cos ((0:8) .* pi/18) */
static const gfloat cos18_table[] = {
  1.000000000000000000000000000000000000000000000000000000000000e+00f,
  9.848077530122080203156542665965389460325241088867187500000000e-01f,
  9.396926207859084279050421173451468348503112792968750000000000e-01f,
  8.660254037844387076106045242340769618749618530273437500000000e-01f,
  7.660444431189780134516809084743726998567581176757812500000000e-01f,
  6.427876096865393629187224178167525678873062133789062500000000e-01f,
  5.000000000000001110223024625156540423631668090820312500000000e-01f,
  3.420201433256688239303855425532674416899681091308593750000000e-01f,
  1.736481776669304144533612088707741349935531616210937500000000e-01f
};

/* 0.5 ./ cos ((1:35) .* pi/72)) */
static const gfloat icos72_table[] = {
  5.004763425816599609063928255636710673570632934570312500000000e-01f,
  5.019099187716736798492433990759309381246566772460937500000000e-01f,
  5.043144802900764167574720886477734893560409545898437500000000e-01f,
  5.077133059428725614381505693017970770597457885742187500000000e-01f,
  5.121397571572545714957414020318537950515747070312500000000000e-01f,
  5.176380902050414789528076653368771076202392578125000000000000e-01f,
  5.242645625704053236049162478593643754720687866210937500000000e-01f,
  5.320888862379560269033618169487453997135162353515625000000000e-01f,
  5.411961001461970122150546558259520679712295532226562500000000e-01f,
  5.516889594812458552652856269560288637876510620117187500000000e-01f,
  5.636909734331712051869089918909594416618347167968750000000000e-01f,
  5.773502691896257310588680411456152796745300292968750000000000e-01f,
  5.928445237170802961657045671017840504646301269531250000000000e-01f,
  6.103872943807280293526673631276935338973999023437500000000000e-01f,
  6.302362070051321651931175438221544027328491210937500000000000e-01f,
  6.527036446661392821155800447741057723760604858398437500000000e-01f,
  6.781708524546284921896699415810871869325637817382812500000000e-01f,
  7.071067811865474617150084668537601828575134277343750000000000e-01f,
  7.400936164611303658134033867099788039922714233398437500000000e-01f,
  7.778619134302061643992942663317080587148666381835937500000000e-01f,
  8.213398158522907666068135768000502139329910278320312500000000e-01f,
  8.717233978105488612087015098950359970331192016601562500000000e-01f,
  9.305794983517888807611484480730723589658737182617187500000000e-01f,
  9.999999999999997779553950749686919152736663818359375000000000e-01f,
  1.082840285100100219395358180918265134096145629882812500000000e+00f,
  1.183100791576249255498964885191526263952255249023437500000000e+00f,
  1.306562964876376353728915091778617352247238159179687500000000e+00f,
  1.461902200081543146126250576344318687915802001953125000000000e+00f,
  1.662754761711521034328598034335300326347351074218750000000000e+00f,
  1.931851652578135070115195048856548964977264404296875000000000e+00f,
  2.310113157672649020213384574162773787975311279296875000000000e+00f,
  2.879385241571815523542454684502445161342620849609375000000000e+00f,
  3.830648787770197127855453800293616950511932373046875000000000e+00f,
  5.736856622834929808618653623852878808975219726562500000000000e+00f,
  1.146279281302667207853573927422985434532165527343750000000000e+01f
};

static const gfloat mdct_swin[4][36] = {
  {
    0.0436193869f, 0.1305261850f, 0.2164396197f, 0.3007057905f, 0.3826834261f, 0.4617486000f,
    0.5372996330f, 0.6087614298f, 0.6755902171f, 0.7372773290f, 0.7933533192f, 0.8433914185f,
    0.8870108128f, 0.9238795042f, 0.9537169337f, 0.9762960076f, 0.9914448857f, 0.9990482330f,
    0.9990482330f, 0.9914448857f, 0.9762960076f, 0.9537169337f, 0.9238795042f, 0.8870108128f,
    0.8433914185f, 0.7933533192f, 0.7372773290f, 0.6755902171f, 0.6087614298f, 0.5372996330f,
    0.4617486000f, 0.3826834261f, 0.3007057905f, 0.2164396197f, 0.1305261850f, 0.0436193869f,
  }, {
    0.0436193869f, 0.1305261850f, 0.2164396197f, 0.3007057905f, 0.3826834261f, 0.4617486000f,
    0.5372996330f, 0.6087614298f, 0.6755902171f, 0.7372773290f, 0.7933533192f, 0.8433914185f,
    0.8870108128f, 0.9238795042f, 0.9537169337f, 0.9762960076f, 0.9914448857f, 0.9990482330f,
    1.0000000000f, 1.0000000000f, 1.0000000000f, 1.0000000000f, 1.0000000000f, 1.0000000000f,
    0.9914448857f, 0.9238795042f, 0.7933533192f, 0.6087614298f, 0.3826834261f, 0.1305261850f,
    0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f,
  }, {
    0.1305261850f, 0.3826834261f, 0.6087614298f, 0.7933533192f, 0.9238795042f, 0.9914448857f,
    0.9914448857f, 0.9238795042f, 0.7933533192f, 0.6087614298f, 0.3826834261f, 0.1305261850f,
    0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f,
    0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f,
    0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f,
    0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f,
  }, {
    0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f, 0.0000000000f,
    0.1305261850f, 0.3826834261f, 0.6087614298f, 0.7933533192f, 0.9238795042f, 0.9914448857f,
    1.0000000000f, 1.0000000000f, 1.0000000000f, 1.0000000000f, 1.0000000000f, 1.0000000000f,
    0.9990482330f, 0.9914448857f, 0.9762960076f, 0.9537169337f, 0.9238795042f, 0.8870108128f,
    0.8433914185f, 0.7933533192f, 0.7372773290f, 0.6755902171f, 0.6087614298f, 0.5372996330f,
    0.4617486000f, 0.3826834261f, 0.3007057905f, 0.2164396197f, 0.1305261850f, 0.0436193869f,
  }
};

/* pre-calculated table for (1.0 / (2.0 * cos ((2*i+1) * (M_PI / (64)))))
 * for i 0:31 */
/* 0.5 ./ cos (((2 .* 0:30)+1) .* pi/64) */
/* NOTE: The table is already offset by pi/64 at index 0, by the +1 term 
 ie, index x yields 0.5 / cos ((2*x+1) * pi/64) */
static const gfloat synth_cos64_table[] = {
  5.0060299823519627260e-01f, 5.0241928618815567820e-01f,
  5.0547095989754364798e-01f, 5.0979557910415917998e-01f,
  5.1544730992262455249e-01f, 5.2249861493968885462e-01f,
  5.3104259108978413284e-01f, 5.4119610014619701222e-01f,
  5.5310389603444454210e-01f, 5.6694403481635768927e-01f,
  5.8293496820613388554e-01f, 6.0134488693504528634e-01f,
  6.2250412303566482475e-01f, 6.4682178335999007679e-01f,
  6.7480834145500567800e-01f, 7.0710678118654746172e-01f,
  7.4453627100229857749e-01f, 7.8815462345125020249e-01f,
  8.3934964541552681272e-01f, 8.9997622313641556513e-01f,
  9.7256823786196078263e-01f, 1.0606776859903470633e+00f,
  1.1694399334328846596e+00f, 1.3065629648763763537e+00f,
  1.4841646163141661852e+00f, 1.7224470982383341955e+00f,
  2.0577810099534108446e+00f, 2.5629154477415054814e+00f,
  3.4076084184687189804e+00f, 5.1011486186891552563e+00f,
  1.0190008123548032870e+01f
};

static __CACHE_LINE_DECL_ALIGN(const gfloat dewindow[512]) = {
  0.000000000f, -0.000015259f, -0.000015259f, -0.000015259f,
  -0.000015259f, -0.000015259f, -0.000015259f, -0.000030518f,
  -0.000030518f, -0.000030518f, -0.000030518f, -0.000045776f,
  -0.000045776f, -0.000061035f, -0.000061035f, -0.000076294f,
  -0.000076294f, -0.000091553f, -0.000106812f, -0.000106812f,
  -0.000122070f, -0.000137329f, -0.000152588f, -0.000167847f,
  -0.000198364f, -0.000213623f, -0.000244141f, -0.000259399f,
  -0.000289917f, -0.000320435f, -0.000366211f, -0.000396729f,
  -0.000442505f, -0.000473022f, -0.000534058f, -0.000579834f,
  -0.000625610f, -0.000686646f, -0.000747681f, -0.000808716f,
  -0.000885010f, -0.000961304f, -0.001037598f, -0.001113892f,
  -0.001205444f, -0.001296997f, -0.001388550f, -0.001480103f,
  -0.001586914f, -0.001693726f, -0.001785278f, -0.001907349f,
  -0.002014160f, -0.002120972f, -0.002243042f, -0.002349854f,
  -0.002456665f, -0.002578735f, -0.002685547f, -0.002792358f,
  -0.002899170f, -0.002990723f, -0.003082275f, -0.003173828f,
  0.003250122f, 0.003326416f, 0.003387451f, 0.003433228f,
  0.003463745f, 0.003479004f, 0.003479004f, 0.003463745f,
  0.003417969f, 0.003372192f, 0.003280640f, 0.003173828f,
  0.003051758f, 0.002883911f, 0.002700806f, 0.002487183f,
  0.002227783f, 0.001937866f, 0.001617432f, 0.001266479f,
  0.000869751f, 0.000442505f, -0.000030518f, -0.000549316f,
  -0.001098633f, -0.001693726f, -0.002334595f, -0.003005981f,
  -0.003723145f, -0.004486084f, -0.005294800f, -0.006118774f,
  -0.007003784f, -0.007919312f, -0.008865356f, -0.009841919f,
  -0.010848999f, -0.011886597f, -0.012939453f, -0.014022827f,
  -0.015121460f, -0.016235352f, -0.017349243f, -0.018463135f,
  -0.019577026f, -0.020690918f, -0.021789551f, -0.022857666f,
  -0.023910522f, -0.024932861f, -0.025909424f, -0.026840210f,
  -0.027725220f, -0.028533936f, -0.029281616f, -0.029937744f,
  -0.030532837f, -0.031005859f, -0.031387329f, -0.031661987f,
  -0.031814575f, -0.031845093f, -0.031738281f, -0.031478882f,
  0.031082153f, 0.030517578f, 0.029785156f, 0.028884888f,
  0.027801514f, 0.026535034f, 0.025085449f, 0.023422241f,
  0.021575928f, 0.019531250f, 0.017257690f, 0.014801025f,
  0.012115479f, 0.009231567f, 0.006134033f, 0.002822876f,
  -0.000686646f, -0.004394531f, -0.008316040f, -0.012420654f,
  -0.016708374f, -0.021179199f, -0.025817871f, -0.030609131f,
  -0.035552979f, -0.040634155f, -0.045837402f, -0.051132202f,
  -0.056533813f, -0.061996460f, -0.067520142f, -0.073059082f,
  -0.078628540f, -0.084182739f, -0.089706421f, -0.095169067f,
  -0.100540161f, -0.105819702f, -0.110946655f, -0.115921021f,
  -0.120697021f, -0.125259399f, -0.129562378f, -0.133590698f,
  -0.137298584f, -0.140670776f, -0.143676758f, -0.146255493f,
  -0.148422241f, -0.150115967f, -0.151306152f, -0.151962280f,
  -0.152069092f, -0.151596069f, -0.150497437f, -0.148773193f,
  -0.146362305f, -0.143264771f, -0.139450073f, -0.134887695f,
  -0.129577637f, -0.123474121f, -0.116577148f, -0.108856201f,
  0.100311279f, 0.090927124f, 0.080688477f, 0.069595337f,
  0.057617187f, 0.044784546f, 0.031082153f, 0.016510010f,
  0.001068115f, -0.015228271f, -0.032379150f, -0.050354004f,
  -0.069168091f, -0.088775635f, -0.109161377f, -0.130310059f,
  -0.152206421f, -0.174789429f, -0.198059082f, -0.221984863f,
  -0.246505737f, -0.271591187f, -0.297210693f, -0.323318481f,
  -0.349868774f, -0.376800537f, -0.404083252f, -0.431655884f,
  -0.459472656f, -0.487472534f, -0.515609741f, -0.543823242f,
  -0.572036743f, -0.600219727f, -0.628295898f, -0.656219482f,
  -0.683914185f, -0.711318970f, -0.738372803f, -0.765029907f,
  -0.791213989f, -0.816864014f, -0.841949463f, -0.866363525f,
  -0.890090942f, -0.913055420f, -0.935195923f, -0.956481934f,
  -0.976852417f, -0.996246338f, -1.014617920f, -1.031936646f,
  -1.048156738f, -1.063217163f, -1.077117920f, -1.089782715f,
  -1.101211548f, -1.111373901f, -1.120223999f, -1.127746582f,
  -1.133926392f, -1.138763428f, -1.142211914f, -1.144287109f,
  1.144989014f, 1.144287109f, 1.142211914f, 1.138763428f,
  1.133926392f, 1.127746582f, 1.120223999f, 1.111373901f,
  1.101211548f, 1.089782715f, 1.077117920f, 1.063217163f,
  1.048156738f, 1.031936646f, 1.014617920f, 0.996246338f,
  0.976852417f, 0.956481934f, 0.935195923f, 0.913055420f,
  0.890090942f, 0.866363525f, 0.841949463f, 0.816864014f,
  0.791213989f, 0.765029907f, 0.738372803f, 0.711318970f,
  0.683914185f, 0.656219482f, 0.628295898f, 0.600219727f,
  0.572036743f, 0.543823242f, 0.515609741f, 0.487472534f,
  0.459472656f, 0.431655884f, 0.404083252f, 0.376800537f,
  0.349868774f, 0.323318481f, 0.297210693f, 0.271591187f,
  0.246505737f, 0.221984863f, 0.198059082f, 0.174789429f,
  0.152206421f, 0.130310059f, 0.109161377f, 0.088775635f,
  0.069168091f, 0.050354004f, 0.032379150f, 0.015228271f,
  -0.001068115f, -0.016510010f, -0.031082153f, -0.044784546f,
  -0.057617187f, -0.069595337f, -0.080688477f, -0.090927124f,
  0.100311279f, 0.108856201f, 0.116577148f, 0.123474121f,
  0.129577637f, 0.134887695f, 0.139450073f, 0.143264771f,
  0.146362305f, 0.148773193f, 0.150497437f, 0.151596069f,
  0.152069092f, 0.151962280f, 0.151306152f, 0.150115967f,
  0.148422241f, 0.146255493f, 0.143676758f, 0.140670776f,
  0.137298584f, 0.133590698f, 0.129562378f, 0.125259399f,
  0.120697021f, 0.115921021f, 0.110946655f, 0.105819702f,
  0.100540161f, 0.095169067f, 0.089706421f, 0.084182739f,
  0.078628540f, 0.073059082f, 0.067520142f, 0.061996460f,
  0.056533813f, 0.051132202f, 0.045837402f, 0.040634155f,
  0.035552979f, 0.030609131f, 0.025817871f, 0.021179199f,
  0.016708374f, 0.012420654f, 0.008316040f, 0.004394531f,
  0.000686646f, -0.002822876f, -0.006134033f, -0.009231567f,
  -0.012115479f, -0.014801025f, -0.017257690f, -0.019531250f,
  -0.021575928f, -0.023422241f, -0.025085449f, -0.026535034f,
  -0.027801514f, -0.028884888f, -0.029785156f, -0.030517578f,
  0.031082153f, 0.031478882f, 0.031738281f, 0.031845093f,
  0.031814575f, 0.031661987f, 0.031387329f, 0.031005859f,
  0.030532837f, 0.029937744f, 0.029281616f, 0.028533936f,
  0.027725220f, 0.026840210f, 0.025909424f, 0.024932861f,
  0.023910522f, 0.022857666f, 0.021789551f, 0.020690918f,
  0.019577026f, 0.018463135f, 0.017349243f, 0.016235352f,
  0.015121460f, 0.014022827f, 0.012939453f, 0.011886597f,
  0.010848999f, 0.009841919f, 0.008865356f, 0.007919312f,
  0.007003784f, 0.006118774f, 0.005294800f, 0.004486084f,
  0.003723145f, 0.003005981f, 0.002334595f, 0.001693726f,
  0.001098633f, 0.000549316f, 0.000030518f, -0.000442505f,
  -0.000869751f, -0.001266479f, -0.001617432f, -0.001937866f,
  -0.002227783f, -0.002487183f, -0.002700806f, -0.002883911f,
  -0.003051758f, -0.003173828f, -0.003280640f, -0.003372192f,
  -0.003417969f, -0.003463745f, -0.003479004f, -0.003479004f,
  -0.003463745f, -0.003433228f, -0.003387451f, -0.003326416f,
  0.003250122f, 0.003173828f, 0.003082275f, 0.002990723f,
  0.002899170f, 0.002792358f, 0.002685547f, 0.002578735f,
  0.002456665f, 0.002349854f, 0.002243042f, 0.002120972f,
  0.002014160f, 0.001907349f, 0.001785278f, 0.001693726f,
  0.001586914f, 0.001480103f, 0.001388550f, 0.001296997f,
  0.001205444f, 0.001113892f, 0.001037598f, 0.000961304f,
  0.000885010f, 0.000808716f, 0.000747681f, 0.000686646f,
  0.000625610f, 0.000579834f, 0.000534058f, 0.000473022f,
  0.000442505f, 0.000396729f, 0.000366211f, 0.000320435f,
  0.000289917f, 0.000259399f, 0.000244141f, 0.000213623f,
  0.000198364f, 0.000167847f, 0.000152588f, 0.000137329f,
  0.000122070f, 0.000106812f, 0.000106812f, 0.000091553f,
  0.000076294f, 0.000076294f, 0.000061035f, 0.000061035f,
  0.000045776f, 0.000045776f, 0.000030518f, 0.000030518f,
  0.000030518f, 0.000030518f, 0.000015259f, 0.000015259f,
  0.000015259f, 0.000015259f, 0.000015259f, 0.000015259f
};

/***********************************************************************
 * Use the header information to select the subband allocation table
 **********************************************************************/
static void
II_pick_table (frame_params * fr_ps)
{
  int table, ver, lay, bsp, br_per_ch, sfrq;

  ver = fr_ps->header.version;
  lay = fr_ps->header.layer - 1;
  bsp = fr_ps->header.bitrate_idx;

  /* decision rules refer to per-channel bitrates (kbits/sec/chan) */
  if (ver == MPEG_VERSION_1) {
    br_per_ch = bitrates_v1[lay][bsp] / fr_ps->stereo;

    sfrq = s_rates[ver][fr_ps->header.srate_idx];

    /* MPEG-1 */
    if ((sfrq == 48000 && br_per_ch >= 56) ||
        (br_per_ch >= 56 && br_per_ch <= 80))
      table = 0;
    else if (sfrq != 48000 && br_per_ch >= 96)
      table = 1;
    else if (sfrq != 32000 && br_per_ch <= 48)
      table = 2;
    else
      table = 3;

  } else {
    /* br_per_ch = bitrates_v2[lay][bsp] / fr_ps->stereo; */

    /* MPEG-2 LSF */
    table = 4;
  }

  fr_ps->sblimit = ba_tables[table].sub_bands;
  fr_ps->alloc = &ba_tables[table].alloc;
}

static int
js_bound (gint lay, gint m_ext)
{
  /* layer + mode_ext -> jsbound */
  static const int jsb_table[3][4] = {
    {4, 8, 12, 16}, {4, 8, 12, 16}, {0, 4, 8, 16}
  };

  if (lay < 1 || lay > 3 || m_ext < 0 || m_ext > 3) {
    GST_WARNING ("js_bound bad layer/modext (%d/%d)\n", lay, m_ext);
    return 0;
  }
  return (jsb_table[lay - 1][m_ext]);
}

static void
hdr_to_frps (frame_params * fr_ps)
{
  fr_header *hdr = &fr_ps->header;

  fr_ps->actual_mode = hdr->mode;
  fr_ps->stereo = (hdr->mode == MPG_MD_MONO) ? 1 : 2;
  fr_ps->sblimit = SBLIMIT;

  if (hdr->mode == MPG_MD_JOINT_STEREO)
    fr_ps->jsbound = js_bound (hdr->layer, hdr->mode_ext);
  else
    fr_ps->jsbound = fr_ps->sblimit;
}

/*****************************************************************************
*
*  CRC error protection package
*
*****************************************************************************/
static void
update_CRC (const guint data, const guint length, guint * crc)
{
  unsigned int masking, carry;

  masking = 1 << length;

  while ((masking >>= 1)) {
    carry = *crc & 0x8000;
    *crc <<= 1;
    if (!carry ^ !(data & masking))
      *crc ^= CRC16_POLYNOMIAL;
  }
  *crc &= 0xffff;
}

static void
I_CRC_calc (const frame_params * fr_ps, guint bit_alloc[2][SBLIMIT],
    guint * crc)
{
  gint i, k;
  const fr_header *hdr = &fr_ps->header;
  const gint stereo = fr_ps->stereo;
  const gint jsbound = fr_ps->jsbound;

  *crc = 0xffff;                /* changed from '0' 92-08-11 shn */
  update_CRC (hdr->bitrate_idx, 4, crc);
  update_CRC (hdr->srate_idx, 2, crc);
  update_CRC (hdr->padding, 1, crc);
  update_CRC (hdr->extension, 1, crc);
  update_CRC (hdr->mode, 2, crc);
  update_CRC (hdr->mode_ext, 2, crc);
  update_CRC (hdr->copyright, 1, crc);
  update_CRC (hdr->original, 1, crc);
  update_CRC (hdr->emphasis, 2, crc);

  for (i = 0; i < SBLIMIT; i++)
    for (k = 0; k < ((i < jsbound) ? stereo : 1); k++)
      update_CRC (bit_alloc[k][i], 4, crc);
}

static void
II_CRC_calc (const frame_params * fr_ps, guint bit_alloc[2][SBLIMIT],
    guint scfsi[2][SBLIMIT], guint * crc)
{
  gint i, k;
  const fr_header *hdr = &fr_ps->header;
  const gint stereo = fr_ps->stereo;
  const gint sblimit = fr_ps->sblimit;
  const gint jsbound = fr_ps->jsbound;
  const al_table *alloc = fr_ps->alloc;

  *crc = 0xffff;                /* changed from '0' 92-08-11 shn */

  update_CRC (hdr->bitrate_idx, 4, crc);
  update_CRC (hdr->srate_idx, 2, crc);
  update_CRC (hdr->padding, 1, crc);
  update_CRC (hdr->extension, 1, crc);
  update_CRC (hdr->mode, 2, crc);
  update_CRC (hdr->mode_ext, 2, crc);
  update_CRC (hdr->copyright, 1, crc);
  update_CRC (hdr->original, 1, crc);
  update_CRC (hdr->emphasis, 2, crc);

  for (i = 0; i < sblimit; i++)
    for (k = 0; k < ((i < jsbound) ? stereo : 1); k++)
      update_CRC (bit_alloc[k][i], (*alloc)[i][0].bits, crc);

  for (i = 0; i < sblimit; i++)
    for (k = 0; k < stereo; k++)
      if (bit_alloc[k][i])
        update_CRC (scfsi[k][i], 2, crc);
}

/* 
 * 2 Bitstream buffer implementations. 1 reading from a provided
 * data pointer, the other from a fixed size ring buffer.
 */

/* Create and initialise a new bitstream reader */
Bit_stream_struc *
bs_new ()
{
  Bit_stream_struc *bs;

  bs = (Bit_stream_struc *) calloc(1, sizeof(Bit_stream_struc));
  g_return_val_if_fail (bs != NULL, NULL);

  bs->master.cur_bit = 8;
  bs->master.size = 0;
  bs->master.cur_used = 0;
  bs->read.cur_bit = 8;
  bs->read.size = 0;
  bs->read.cur_used = 0;
  return bs;
}

/* Release a bitstream reader */
void
bs_free (Bit_stream_struc * bs)
{
  g_return_if_fail (bs != NULL);

  free (bs);
}

/* Set data as the stream for processing */
gboolean
bs_set_data (Bit_stream_struc * bs, const guint8 * data, gsize size)
{
  g_return_val_if_fail (bs != NULL, FALSE);
  g_return_val_if_fail (data != NULL, FALSE);
  g_return_val_if_fail (size != 0, FALSE);
  
  bs->master.data = data;
  bs->master.cur_byte = (guint8 *) data;
  bs->master.size = size;
  bs->master.bitpos = 0;
  bs->master.cur_used = 0;
  bs_reset (bs);
  return TRUE;
}

/* Advance N bits on the indicated BSreader */
static inline void
bs_eat (Bit_stream_struc * bs, BSReader * read, guint32 Nbits)
{
  while (Nbits > 0) {
    gint k;

    /* Check for the data limit */
    if (read->cur_used >= read->size) {
      return;
    }

    if (Nbits < 8 || read->cur_bit != 8) {
      /* Take as many bits as we can from the current byte */
      k = MIN (Nbits, read->cur_bit);

      /* Adjust our tracking vars */
      read->cur_bit -= k;
      Nbits -= k;
      read->bitpos += k;

      /* Move to the next byte if we consumed the current one */
      if (read->cur_bit == 0) {
        read->cur_bit = 8;
        read->cur_used++;
        read->cur_byte++;
      }
    } else {
      /* Take as many bytes as we can from current buffer */
      k = MIN (Nbits / 8, (guint32)(read->size - read->cur_used));

      read->cur_used += k;
      read->cur_byte += k;

      /* convert to bits */
      k *= 8;
      read->bitpos += k;
      Nbits -= k;
    }
  }
}

/* Advance the master position by Nbits */
void
bs_consume (Bit_stream_struc * bs, guint32 Nbits)
{
#if 0
  static gint n = 0;
  GST_DEBUG ("%d Consumed %d bits to end at %" G_GUINT64_FORMAT,
      n++, Nbits, bs_pos (bs) + Nbits);
#endif
  bs_eat (bs, &bs->master, Nbits);
}

/* Advance the read position by Nbits */
void
bs_skipbits (Bit_stream_struc * bs, guint32 Nbits)
{
  bs_eat (bs, &bs->read, Nbits);
}

/* Advances the read position to the first bit of next frame or
 * last byte in the buffer when the sync code is not found */
gboolean
bs_seek_sync (Bit_stream_struc * bs)
{
  gboolean res = FALSE;
  guint8 last_byte;
  guint8 *start_pos;

  /* Align to the start of the next byte */
  if (bs->read.cur_bit != BS_BYTE_SIZE) {
    bs->read.bitpos += (BS_BYTE_SIZE - bs->read.cur_bit);
    bs->read.cur_bit = BS_BYTE_SIZE;
    bs->read.cur_used++;
    bs->read.cur_byte++;
  }

  /* Ensure there's still some data to read */
  if (G_UNLIKELY (bs->read.cur_used >= bs->read.size)) {
    return FALSE;
  }

  start_pos = bs->read.cur_byte;
  while (bs->read.cur_used < bs->read.size - 1) {
    last_byte = bs->read.cur_byte[0];
    bs->read.cur_byte++;
    bs->read.cur_used++;

    if (last_byte == 0xff && bs->read.cur_byte[0] >= 0xe0) {
      /* Found a sync word */
      res = TRUE;
      break;
    }
  }
  /* Update the tracked position in the reader */
  bs->read.bitpos += BS_BYTE_SIZE * (bs->read.cur_byte - start_pos);
  
  if (res) {
    /* Move past the first 3 bits of 2nd sync byte */
    bs->read.cur_bit = 5;
    bs->read.bitpos += 3;
  }

  return res;
}

/* Extract N bytes from the bitstream into the out array. */
void
bs_getbytes (Bit_stream_struc * bs, guint8 * out, guint32 N)
{
  gint j = N;
  gint to_take;

  while (j > 0) {
    /* Move to the next byte if we consumed any bits of the current one */
    if (bs->read.cur_bit != 8) {
      bs->read.cur_bit = 8;
      bs->read.cur_used++;
      bs->read.cur_byte++;
    }

    /* Check for the data limit */
    if (bs->read.cur_used >= bs->read.size) {
      GST_WARNING ("Attempted to read beyond buffer");
      return;
    }

    /* Take as many bytes as we can from the current buffer */
    to_take = MIN (j, (gint) (bs->read.size - bs->read.cur_used));
    memcpy (out, bs->read.cur_byte, to_take);

    out += to_take;
    bs->read.cur_byte += to_take;
    bs->read.cur_used += to_take;
    j -= to_take;
    bs->read.bitpos += (to_take * 8);
  }
}

static void
h_setbuf (huffdec_bitbuf * bb, guint8 * buf, guint size)
{
  bb->avail = size;
  bb->buf_byte_idx = 0;
  bb->buf_bit_idx = 8;
  bb->buf = buf;
#if ENABLE_OPT_BS
  if (buf) {
    /* First load of the accumulator, assumes that size >= 4 */
    bb->buf_bit_idx = 32;
    bb->remaining = bb->avail - 4;

    /* we need reverse the byte order */
    bb->accumulator = (guint) buf[3];
    bb->accumulator |= (guint) (buf[2]) << 8;
    bb->accumulator |= (guint) (buf[1]) << 16;
    bb->accumulator |= (guint) (buf[0]) << 24;

    bb->buf_byte_idx += 4;
  } else {
    bb->remaining = 0;
    bb->accumulator = 0;
  }
#endif
}

static void
h_reset (huffdec_bitbuf * bb)
{
  h_setbuf (bb, NULL, 0);
}

#if ENABLE_OPT_BS
static void
h_rewindNbits (huffdec_bitbuf * bb, guint N)
{
  guint bits = 0;
  guint bytes = 0;
  if (N <= (BS_ACUM_SIZE - bb->buf_bit_idx))
    bb->buf_bit_idx += N;
  else {
    N -= (BS_ACUM_SIZE - bb->buf_bit_idx);
    bb->buf_bit_idx = 0;
    bits = 8 - (N % 8);
    bytes = (N + 8 + BS_ACUM_SIZE) >> 3;
    if (bb->buf_byte_idx >= bytes)
      bb->buf_byte_idx -= bytes;
    else 
      bb->buf_byte_idx = 0;
    bb->remaining += bytes;
    h_getbits (bb, bits);
  }
}
#else
void
static h_rewindNbits (huffdec_bitbuf * bb, guint N)
{
  guint32 byte_off;

  byte_off = (bb->buf_bit_idx + N) / 8;

  g_return_if_fail (bb->buf_byte_idx >= byte_off);

  bb->buf_bit_idx += N;

  if (bb->buf_bit_idx >= 8) {
    bb->buf_bit_idx -= 8 * byte_off;
    bb->buf_byte_idx -= byte_off;
  }
}
#endif

/* Constant declarations */
static const gfloat multiple[64] = {
  2.00000000000000f, 1.58740105196820f, 1.25992104989487f,
  1.00000000000000f, 0.79370052598410f, 0.62996052494744f, 0.50000000000000f,
  0.39685026299205f, 0.31498026247372f, 0.25000000000000f, 0.19842513149602f,
  0.15749013123686f, 0.12500000000000f, 0.09921256574801f, 0.07874506561843f,
  0.06250000000000f, 0.04960628287401f, 0.03937253280921f, 0.03125000000000f,
  0.02480314143700f, 0.01968626640461f, 0.01562500000000f, 0.01240157071850f,
  0.00984313320230f, 0.00781250000000f, 0.00620078535925f, 0.00492156660115f,
  0.00390625000000f, 0.00310039267963f, 0.00246078330058f, 0.00195312500000f,
  0.00155019633981f, 0.00123039165029f, 0.00097656250000f, 0.00077509816991f,
  0.00061519582514f, 0.00048828125000f, 0.00038754908495f, 0.00030759791257f,
  0.00024414062500f, 0.00019377454248f, 0.00015379895629f, 0.00012207031250f,
  0.00009688727124f, 0.00007689947814f, 0.00006103515625f, 0.00004844363562f,
  0.00003844973907f, 0.00003051757813f, 0.00002422181781f, 0.00001922486954f,
  0.00001525878906f, 0.00001211090890f, 0.00000961243477f, 0.00000762939453f,
  0.00000605545445f, 0.00000480621738f, 0.00000381469727f, 0.00000302772723f,
  0.00000240310869f, 0.00000190734863f, 0.00000151386361f, 0.00000120155435f,
  1e-20f
};

/*************************************************************
 *
 * This module parses the starting 21 bits of the header
 *
 * **********************************************************/

static gboolean
read_main_header (Bit_stream_struc * bs, fr_header * hdr)
{
  if (bs_bits_avail (bs) < HEADER_LNGTH) {
    return FALSE;
  }

  /* Read 2 bits as version, since we're doing the MPEG2.5 thing
   * of an 11 bit sync word and 2 bit version */
  hdr->version = bs_getbits (bs, 2);
  hdr->layer = 4 - bs_getbits (bs, 2);

  /* error_protection TRUE indicates there is a CRC */
  hdr->error_protection = !bs_get1bit (bs);
  hdr->bitrate_idx = bs_getbits (bs, 4);
  hdr->srate_idx = bs_getbits (bs, 2);
  hdr->padding = bs_get1bit (bs);
  hdr->extension = bs_get1bit (bs);
  hdr->mode = bs_getbits (bs, 2);
  hdr->mode_ext = bs_getbits (bs, 2);

  hdr->copyright = bs_get1bit (bs);
  hdr->original = bs_get1bit (bs);
  hdr->emphasis = bs_getbits (bs, 2);

  return TRUE;
}

/***************************************************************
 *
 * This module contains the core of the decoder ie all the
 * computational routines. (Layer I and II only)
 * Functions are common to both layer unless
 * otherwise specified.
 *
 ***************************************************************/

/************ Layer I, Layer II & Layer III ******************/
static gboolean
read_header (mp3tl * tl, fr_header * hdr)
{
  Bit_stream_struc *bs = tl->bs;

  if (!read_main_header (bs, hdr))
    return FALSE;

  switch (hdr->layer) {
    case 1:
      hdr->bits_per_slot = 32;
      hdr->frame_samples = 384;
      break;
    case 2:
      hdr->bits_per_slot = 8;
      hdr->frame_samples = 1152;
      break;
    case 3:
      hdr->bits_per_slot = 8;
      switch (hdr->version) {
        case MPEG_VERSION_1:
          hdr->frame_samples = 1152;
          break;
        case MPEG_VERSION_2:
        case MPEG_VERSION_2_5:
          hdr->frame_samples = 576;
          break;
        default:
          return FALSE;
      }
      break;
    default:
      /* Layer must be 1, 2 or 3 */
      return FALSE;
  }

  /* Sample rate index cannot be 0x03 (reserved value) */
  /* Bitrate index cannot be 0x0f (forbidden) */
  if (hdr->srate_idx == 0x03 || hdr->bitrate_idx == 0x0f) {
    return FALSE;
  }

  hdr->channels = (hdr->mode == MPG_MD_MONO) ? 1 : 2;
  hdr->sample_rate = s_rates[hdr->version][hdr->srate_idx];
  hdr->bitrate = 0;
  /*Free format as bitrate index is 0 */
  if (hdr->bitrate_idx == 0) {
    /*Calculate Only for the first free format frame since the stream
     * is of constant bitrate */
    if (tl->free_first) {
      Bit_stream_struc org_bs;
      fr_header hdr1;
      guint N;
      /*copy the orignal bitsream structure */
      memcpy (&org_bs, bs, sizeof (Bit_stream_struc));

      /*Seek to next mp3 sync word and loop till there is data in the bitstream buffer */
      while (bs_seek_sync (bs)) {
        if (!read_main_header (bs, &hdr1))
          return FALSE;

        /*Checks if the original and forwarded frames header details are same
         *if yes then calculate free format bitrate else seek to next frame*/
        if (hdr->version == hdr1.version &&
            hdr->layer == hdr1.layer &&
            hdr->error_protection == hdr1.error_protection &&
            hdr->bitrate_idx == hdr1.bitrate_idx &&
            hdr->srate_idx == hdr1.srate_idx) {
          /*Calculates distance between 2 valid frames */
          N = (guint)(bs->read.cur_used - org_bs.read.cur_used);
          /*Copies back the original bitsream to main bs structure */
          memcpy (bs, &org_bs, sizeof (Bit_stream_struc));

          /*Free format bitrate in kbps that will be used for future reference */
          tl->free_bitrate =
              (hdr->sample_rate * (N - hdr->padding + 1) * 8 / hdr->frame_samples) / 1000;
          hdr->bitrate = tl->free_bitrate * 1000;
          tl->free_first = FALSE;
          break;
        }
      }
    } else
      /*for all frames copy the same free format bitrate as the stream is cbr */
      hdr->bitrate = tl->free_bitrate * 1000;
  } else if (hdr->version == MPEG_VERSION_1)
    hdr->bitrate = bitrates_v1[hdr->layer - 1][hdr->bitrate_idx] * 1000;
  else
    hdr->bitrate = bitrates_v2[hdr->layer - 1][hdr->bitrate_idx] * 1000;

  if (hdr->sample_rate == 0 || hdr->bitrate == 0) {
    return FALSE;
  }

  /* Magic formula for calculating the size of a frame based on
   * the duration of the frame and the bitrate */
  hdr->frame_slots = (hdr->frame_samples / hdr->bits_per_slot)
      * hdr->bitrate / hdr->sample_rate + hdr->padding;

  /* Number of bits we need for decode is frame_slots * slot_size */
  hdr->frame_bits = hdr->frame_slots * hdr->bits_per_slot;
  if (hdr->frame_bits <= 32) {
    return FALSE;               /* Invalid header */
  }

  return TRUE;
}

#define MPEG1_STEREO_SI_SLOTS 32
#define MPEG1_MONO_SI_SLOTS 17
#define MPEG2_LSF_STEREO_SI_SLOTS 17
#define MPEG2_LSF_MONO_SI_SLOTS 9

#define SAMPLE_RATES 3
#define BIT_RATES 15

/* For layer 3 only - the number of slots for main data of
 * current frame. In Layer 3, 1 slot = 1 byte */
static gboolean
set_hdr_data_slots (fr_header * hdr)
{
  int nSlots;

  if (hdr->layer != 3) {
    hdr->side_info_slots = 0;
    hdr->main_slots = 0;
    return TRUE;
  }

  nSlots = hdr->frame_slots - hdr->padding;

#if 0
  if (hdr->version == MPEG_VERSION_1) {
    static const gint MPEG1_slot_table[SAMPLE_RATES][BIT_RATES] = {
      {0, 104, 130, 156, 182, 208, 261, 313, 365, 417, 522, 626, 731, 835, 1044},
      {0, 96, 120, 144, 168, 192, 240, 288, 336, 384, 480, 576, 672, 768, 960},
      {0, 144, 180, 216, 252, 288, 360, 432, 504, 576, 720, 864, 1008, 1152, 1440}
    };
    g_print ("Calced %d main slots, table says %d\n", nSlots,
        MPEG1_slot_table[hdr->srate_idx][hdr->bitrate_idx]);
  } else {
    static const gint MPEG2_LSF_slot_table[SAMPLE_RATES][BIT_RATES] = {
      {0, 26, 52, 78, 104, 130, 156, 182, 208, 261, 313, 365, 417, 470, 522},
      {0, 24, 48, 72, 96, 120, 144, 168, 192, 240, 288, 336, 384, 432, 480},
      {0, 36, 72, 108, 144, 180, 216, 252, 288, 360, 432, 504, 576, 648, 720}
    };
    g_print ("Calced %d main slots, table says %d\n", nSlots,
        MPEG2_LSF_slot_table[hdr->srate_idx][hdr->bitrate_idx]);
  }
#endif

  if (hdr->version == MPEG_VERSION_1) {
    if (hdr->channels == 1)
      hdr->side_info_slots = MPEG1_MONO_SI_SLOTS;
    else
      hdr->side_info_slots = MPEG1_STEREO_SI_SLOTS;
  } else {
    if (hdr->channels == 1)
      hdr->side_info_slots = MPEG2_LSF_MONO_SI_SLOTS;
    else
      hdr->side_info_slots = MPEG2_LSF_STEREO_SI_SLOTS;
  }
  nSlots -= hdr->side_info_slots;

  if (hdr->padding)
    nSlots++;

  nSlots -= 4;
  if (hdr->error_protection)
    nSlots -= 2;

  if (nSlots < 0)
    return FALSE;

  hdr->main_slots = nSlots;

  return TRUE;
}

/*******************************************************************
 *
 * The bit allocation information is decoded. Layer I
 * has 4 bit per subband whereas Layer II is Ws and bit rate
 * dependent.
 *
 ********************************************************************/

/**************************** Layer II *************/
static void
II_decode_bitalloc (Bit_stream_struc * bs, guint32 bit_alloc[2][SBLIMIT],
    frame_params * fr_ps)
{
  int sb, ch;
  int stereo = fr_ps->stereo;
  int sblimit = fr_ps->sblimit;
  int jsbound = fr_ps->jsbound;
  const al_table *alloc = fr_ps->alloc;

  for (sb = 0; sb < jsbound; sb++)
    for (ch = 0; ch < stereo; ch++) {
      bit_alloc[ch][sb] = (char) bs_getbits (bs, (*alloc)[sb][0].bits);
    }

  for (sb = jsbound; sb < sblimit; sb++) {
    /* expand to 2 channels */
    bit_alloc[0][sb] = bit_alloc[1][sb] = bs_getbits (bs, (*alloc)[sb][0].bits);
  }

  /* Zero the rest of the array */
  for (sb = sblimit; sb < SBLIMIT; sb++)
    for (ch = 0; ch < stereo; ch++)
      bit_alloc[ch][sb] = 0;
}

/**************************** Layer I *************/

static void
I_decode_bitalloc (Bit_stream_struc * bs, guint32 bit_alloc[2][SBLIMIT],
    frame_params * fr_ps)
{
  int i, j;
  int stereo = fr_ps->stereo;

//  int sblimit = fr_ps->sblimit;
  int jsbound = fr_ps->jsbound;

  for (i = 0; i < jsbound; i++)
    for (j = 0; j < stereo; j++) {
      bit_alloc[j][i] = bs_getbits (bs, 4);
    }

  for (i = jsbound; i < SBLIMIT; i++) {
    guint32 b = bs_getbits (bs, 4);

    for (j = 0; j < stereo; j++)
      bit_alloc[j][i] = b;
  }
}

/*****************************************************************
 *
 * The following two functions implement the layer I and II
 * format of scale factor extraction. Layer I involves reading
 * 6 bit per subband as scale factor. Layer II requires reading
 * first the scfsi which in turn indicate the number of scale factors
 * transmitted.
 *    Layer I : I_decode_scale
 *   Layer II : II_decode_scale
 *
 ****************************************************************/

/************************** Layer I stuff ************************/
static void
I_decode_scale (Bit_stream_struc * bs, guint32 bit_alloc[2][SBLIMIT],
    guint32 scale_index[2][3][SBLIMIT], frame_params * fr_ps)
{
  int i, j;
  int stereo = fr_ps->stereo;

//  int sblimit = fr_ps->sblimit;

  for (i = 0; i < SBLIMIT; i++)
    for (j = 0; j < stereo; j++) {
      if (!bit_alloc[j][i])
        scale_index[j][0][i] = SCALE_RANGE - 1;
      else {
        /* 6 bit per scale factor */
        scale_index[j][0][i] = bs_getbits (bs, 6);
      }
    }
}

/*************************** Layer II stuff ***************************/

static void
II_decode_scale (Bit_stream_struc * bs,
    guint scfsi[2][SBLIMIT],
    guint bit_alloc[2][SBLIMIT],
    guint scale_index[2][3][SBLIMIT], frame_params * fr_ps)
{
  int sb, ch;
  int stereo = fr_ps->stereo;
  int sblimit = fr_ps->sblimit;

  for (sb = 0; sb < sblimit; sb++)
    for (ch = 0; ch < stereo; ch++)     /* 2 bit scfsi */
      if (bit_alloc[ch][sb]) {
        scfsi[ch][sb] = bs_getbits (bs, 2);
      }

  for (sb = sblimit; sb < SBLIMIT; sb++)
    for (ch = 0; ch < stereo; ch++)
      scfsi[ch][sb] = 0;

  for (sb = 0; sb < sblimit; sb++) {
    for (ch = 0; ch < stereo; ch++) {
      if (bit_alloc[ch][sb]) {
        switch (scfsi[ch][sb]) {
            /* all three scale factors transmitted */
          case 0:
            scale_index[ch][0][sb] = bs_getbits (bs, 6);
            scale_index[ch][1][sb] = bs_getbits (bs, 6);
            scale_index[ch][2][sb] = bs_getbits (bs, 6);
            break;
            /* scale factor 1 & 3 transmitted */
          case 1:
            scale_index[ch][0][sb] =
                scale_index[ch][1][sb] = bs_getbits (bs, 6);
            scale_index[ch][2][sb] = bs_getbits (bs, 6);
            break;
            /* scale factor 1 & 2 transmitted */
          case 3:
            scale_index[ch][0][sb] = bs_getbits (bs, 6);
            scale_index[ch][1][sb] =
                scale_index[ch][2][sb] = bs_getbits (bs, 6);
            break;
            /* only one scale factor transmitted */
          case 2:
            scale_index[ch][0][sb] =
                scale_index[ch][1][sb] =
                scale_index[ch][2][sb] = bs_getbits (bs, 6);
            break;
          default:
            break;
        }
      } else {
        scale_index[ch][0][sb] =
            scale_index[ch][1][sb] = scale_index[ch][2][sb] = SCALE_RANGE - 1;
      }
    }
  }
  for (sb = sblimit; sb < SBLIMIT; sb++) {
    for (ch = 0; ch < stereo; ch++) {
      scale_index[ch][0][sb] =
          scale_index[ch][1][sb] = scale_index[ch][2][sb] = SCALE_RANGE - 1;
    }
  }
}

/**************************************************************
 *
 *   The following two routines take care of reading the
 * compressed sample from the bit stream for both layer 1 and
 * layer 2. For layer 1, read the number of bits as indicated
 * by the bit_alloc information. For layer 2, if grouping is
 * indicated for a particular subband, then the sample size has
 * to be read from the bits_group and the merged samples has
 * to be decompose into the three distinct samples. Otherwise,
 * it is the same for as layer one.
 *
 **************************************************************/

/******************************* Layer I stuff ******************/

static void
I_buffer_sample (Bit_stream_struc * bs,
    guint sample[2][3][SBLIMIT],
    guint bit_alloc[2][SBLIMIT], frame_params * fr_ps)
{
  int i, j, k;
  int stereo = fr_ps->stereo;

//  int sblimit = fr_ps->sblimit;
  int jsbound = fr_ps->jsbound;
  unsigned int s;

  for (i = 0; i < jsbound; i++) {
    for (j = 0; j < stereo; j++) {
      k = bit_alloc[j][i];
      if (k == 0)
        sample[j][0][i] = 0;
      else
        sample[j][0][i] = bs_getbits (bs, k + 1);
    }
  }
  for (i = jsbound; i < SBLIMIT; i++) {
    k = bit_alloc[0][i];
    if (k == 0)
      s = 0;
    else
      s = bs_getbits (bs, k + 1);

    for (j = 0; j < stereo; j++)
      sample[j][0][i] = s;
  }
}

/*************************** Layer II stuff ************************/

static void
II_buffer_sample (Bit_stream_struc * bs, guint sample[2][3][SBLIMIT],
    guint bit_alloc[2][SBLIMIT], frame_params * fr_ps)
{
  int sb, ch, k;
  int stereo = fr_ps->stereo;
  int sblimit = fr_ps->sblimit;
  int jsbound = fr_ps->jsbound;
  const al_table *alloc = fr_ps->alloc;

  for (sb = 0; sb < sblimit; sb++) {
    for (ch = 0; ch < ((sb < jsbound) ? stereo : 1); ch++) {
      guint allocation = bit_alloc[ch][sb];
      if (allocation) {
        /* check for grouping in subband */
        if (alloc[0][sb][allocation].group == 3) {
          k = alloc[0][sb][allocation].bits;
          sample[ch][0][sb] = bs_getbits (bs, k);
          sample[ch][1][sb] = bs_getbits (bs, k);
          sample[ch][2][sb] = bs_getbits (bs, k);
        } else {                /* bit_alloc = 3, 5, 9 */
          unsigned int nlevels, c = 0;

          nlevels = alloc[0][sb][allocation].steps;
          k = alloc[0][sb][allocation].bits;
          c = bs_getbits (bs, k);
          for (k = 0; k < 3; k++) {
            sample[ch][k][sb] = c % nlevels;
            c /= nlevels;
          }
        }
      } else {                  /* for no sample transmitted */
        sample[ch][0][sb] = 0;
        sample[ch][1][sb] = 0;
        sample[ch][2][sb] = 0;
      }
      if (stereo == 2 && sb >= jsbound) {       /* joint stereo : copy L to R */
        sample[1][0][sb] = sample[0][0][sb];
        sample[1][1][sb] = sample[0][1][sb];
        sample[1][2][sb] = sample[0][2][sb];
      }
    }
  }
  for (sb = sblimit; sb < SBLIMIT; sb++)
    for (ch = 0; ch < stereo; ch++) {
      sample[ch][0][sb] = 0;
      sample[ch][1][sb] = 0;
      sample[ch][2][sb] = 0;
    }
}

/**************************************************************
 *
 *   Restore the compressed sample to a factional number.
 *   first complement the MSB of the sample
 *    for layer I :
 *    Use s = (s' + 2^(-nb+1) ) * 2^nb / (2^nb-1)
 *   for Layer II :
 *   Use the formula s = s' * c + d
 *
 **************************************************************/
static const gfloat c_table[17] = {
  1.33333333333f, 1.60000000000f, 1.14285714286f,
  1.77777777777f, 1.06666666666f, 1.03225806452f,
  1.01587301587f, 1.00787401575f, 1.00392156863f,
  1.00195694716f, 1.00097751711f, 1.00048851979f,
  1.00024420024f, 1.00012208522f, 1.00006103888f,
  1.00003051851f, 1.00001525902f
};

static const gfloat d_table[17] = {
  0.500000000f, 0.500000000f, 0.250000000f, 0.500000000f,
  0.125000000f, 0.062500000f, 0.031250000f, 0.015625000f,
  0.007812500f, 0.003906250f, 0.001953125f, 0.0009765625f,
  0.00048828125f, 0.00024414063f, 0.00012207031f,
  0.00006103516f, 0.00003051758f
};

/************************** Layer II stuff ************************/

static void
II_dequant_and_scale_sample (guint sample[2][3][SBLIMIT],
    guint bit_alloc[2][SBLIMIT], float fraction[2][3][SBLIMIT],
    guint scale_index[2][3][SBLIMIT], int scale_block, frame_params * fr_ps)
{
  int sb, gr, ch, x;
  int stereo = fr_ps->stereo;
  int sblimit = fr_ps->sblimit;
  const al_table *alloc = fr_ps->alloc;

  for (sb = 0; sb < sblimit; sb++) {
    for (ch = 0; ch < stereo; ch++) {
      guint allocation = bit_alloc[ch][sb];

      if (allocation != 0) {
        gfloat scale_val, val;
        gfloat c_quant, d_quant;

        c_quant = c_table[alloc[0][sb][allocation].quant];
        d_quant = d_table[alloc[0][sb][allocation].quant];
        scale_val = multiple[scale_index[ch][scale_block][sb]];

        for (gr = 0; gr < 3; gr++) {
          /* locate MSB in the sample */
          x = 0;
          while ((1UL << x) < (*alloc)[sb][allocation].steps)
            x++;

          /* MSB inversion */
          if (((sample[ch][gr][sb] >> (x - 1)) & 1) == 1)
            val = 0.0f;
          else
            val = -1.0f;

          /* Form a 2's complement sample */
          val += (gfloat) ((double) (sample[ch][gr][sb] & ((1 << (x - 1)) - 1))
              / (double) (1L << (x - 1)));

          /* Dequantize the sample */
          val += d_quant;
          val *= c_quant;

          /* And scale */
          val *= scale_val;
          fraction[ch][gr][sb] = val;
        }
      } else {
        fraction[ch][0][sb] = 0.0f;
        fraction[ch][1][sb] = 0.0f;
        fraction[ch][2][sb] = 0.0f;
      }
    }
  }

  for (sb = sblimit; sb < SBLIMIT; sb++)
    for (ch = 0; ch < stereo; ch++) {
      fraction[ch][0][sb] = 0.0f;
      fraction[ch][1][sb] = 0.0f;
      fraction[ch][2][sb] = 0.0f;
    }
}

/***************************** Layer I stuff ***********************/

static void
I_dequant_and_scale_sample (guint sample[2][3][SBLIMIT],
    float fraction[2][3][SBLIMIT],
    guint bit_alloc[2][SBLIMIT],
    guint scale_index[2][3][SBLIMIT], frame_params * fr_ps)
{
  int sb, ch;
  guint nb;
  int stereo = fr_ps->stereo;

  for (sb = 0; sb < SBLIMIT; sb++) {
    for (ch = 0; ch < stereo; ch++) {
      guint allocation = bit_alloc[ch][sb];

      if (allocation != 0) {
        double val;

        nb = allocation + 1;

        if (((sample[ch][0][sb] >> allocation) & 1) != 0)
          val = 0.0;
        else
          val = -1.0;

        val += (double) (sample[ch][0][sb] & ((1 << allocation) - 1)) /
            (double) (1L << (nb - 1));

        val =
            (double) (val + 1.0 / (1L << allocation)) *
            (double) (1L << nb) / ((1L << nb) - 1);

        val *= (double) multiple[scale_index[ch][0][sb]];

        fraction[ch][0][sb] = (gfloat) val;
      } else
        fraction[ch][0][sb] = 0.0f;
    }
  }
}

/*****************************************************************
 *
 * The following are the subband synthesis routines. They apply
 * to both layer I and layer II stereo or mono. The user has to
 * decide what parameters are to be passed to the routines.
 *
 ***************************************************************/

/* Write output samples into the outBuf, incrementing psamples for each
 * sample, wrapping at bufSize */
static inline void
out_fifo (short pcm_sample[2][SSLIMIT][SBLIMIT], int num,
    frame_params * fr_ps, gint16 * outBuf, guint32 * psamples, guint32 bufSize)
{
  int i, j, k, l;
  int stereo = fr_ps->stereo;
  k = *psamples;
  if (stereo == 2) {
    for (i = 0; i < num; i++) {
      for (j = 0; j < SBLIMIT; j++) {
        outBuf[k] = pcm_sample[0][i][j];
        outBuf[k+1] = pcm_sample[1][i][j];
        k += 2;
        k %= bufSize;
      }
    }
  } else if (stereo == 1) {
    for (i = 0; i < num; i++) {
      for (j = 0; j < SBLIMIT; j++) {
        outBuf[k] = pcm_sample[0][i][j];
        k++;
        k %= bufSize;
      }
    }
  } else {
    for (i = 0; i < num; i++) {
      for (j = 0; j < SBLIMIT; j++) {
        for (l = 0; l < stereo; l++) {
          outBuf[k] = pcm_sample[l][i][j];
          k++;
          k %= bufSize;
        }
      }
    }
  }
  *psamples = k;
}

/*************************************************************
 *
 *   Pass the subband sample through the synthesis window
 *
 **************************************************************/

/* create in synthesis filter */
static void
init_syn_filter (frame_params * fr_ps)
{
  int i, k;
  gfloat (*filter)[32];

  filter = fr_ps->filter;

  for (i = 0; i < 64; i++)
    for (k = 0; k < 32; k++) {
      if ((filter[i][k] = 1e9f * cosf ((float)((PI64 * i + PI4) * (2 * k + 1)))) >= 0.0f)
        modff (filter[i][k] + 0.5f, &filter[i][k]);
      else
        modff (filter[i][k] - 0.5f, &filter[i][k]);
      filter[i][k] *= 1e-9f;
    }

  for (i = 0; i < 2; i++)
    fr_ps->bufOffset[i] = 64;
}

/***************************************************************
 *
 *   Window the restored sample
 *
 ***************************************************************/

#define INV_SQRT_2 (7.071067811865474617150084668537e-01f)

#if defined(USE_ARM_NEON)
static const __CACHE_LINE_DECL_ALIGN(float dct8_k[8]) = {
  INV_SQRT_2, 0.0f, 5.4119610014619701222e-01f, 1.3065629648763763537e+00f,
  5.0979557910415917998e-01f, 6.0134488693504528634e-01f,
  8.9997622313641556513e-01f, 2.5629154477415054814e+00f 
};

STATIC_INLINE void
MPG_DCT_8 (gfloat in[8], gfloat out[8])
{
  __asm__ volatile (
      "vld1.64       {q0-q1}, [%[dct8_k],:128]       \n\t" /* read dct8_k */
      "vld1.64       {q2-q3}, [%[in],:128]           \n\t" /* read in */
      "vrev64.f32    q3, q3                          \n\t"
      "vswp          d6, d7                          \n\t"
      "vadd.f32      q4, q2, q3                      \n\t" /* ei0, ei2, ei3, ei1 */
      "vadd.f32      s28, s16, s19                   \n\t" /* t0 = ei0 + ei1 */
      "vadd.f32      s29, s17, s18                   \n\t" /* t1 = ei2 + ei3 */             
      "vsub.f32      s30, s16, s19                   \n\t" /* t2 = ei0 - ei1 */
      "vsub.f32      s31, s17, s18                   \n\t" /* t3 = ei2 - ei3 */
      "vmul.f32      d15, d15, d1                    \n\t"
      "vsub.f32      q5, q2, q3                      \n\t" /* oi0', oi1', oi2', oi3' */
      "vsub.f32      s27, s30, s31                   \n\t" /* t4' = t2 - t3 */
      "vadd.f32      s24, s28, s29                   \n\t" /* out0 = t0 + t1 */
      "vmul.f32      q5, q5, q1                      \n\t" /* oi0, oi1, oi2, oi3 */
      "vmul.f32      s27, s27, s0                    \n\t" /* out6 = t4 */
      "vadd.f32      s25, s30, s31                   \n\t" /* out2 = t2 + t3 */
      "vadd.f32      s25, s25, s27                   \n\t" /* out2 = t2 + t3 + t4 */
      "vsub.f32      s26, s28, s29                   \n\t" /* out4' = t0 - t1 */
      "vrev64.f32    d11, d11                        \n\t"
      "vmul.f32      s26, s26, s0                    \n\t" /* out4 = (t0 -t1) * INV_SQRT_2 */
      "vadd.f32      d4, d10, d11                    \n\t" /* t0,t1 = oi0 + oi3, oi1 + oi2 */
      "vsub.f32      d5, d10, d11                    \n\t" /* t2',t3' = oi0 - oi3, oi1 - oi3 */
      "vmul.f32      d5, d5, d1                      \n\t" /* t2, t3 */
      "vadd.f32      s12, s10, s11                   \n\t" /* t4 = t2 + t3 */
      "vsub.f32      s13, s10, s11                   \n\t" /* t5' = t2 - t3 */
      "vmul.f32      s31, s13, s0                    \n\t" /* out7 = oo3 = t5 */
      "vadd.f32      s14, s8, s9                     \n\t" /* oo0 = t0 + t1 */
      "vadd.f32      s15, s12, s31                   \n\t" /* oo1 = t4 + t5 */
      "vsub.f32      s16, s8, s9                     \n\t" /* oo2' = t0 - t1 */
      "vmul.f32      s16, s16, s0                    \n\t" /* oo2 */
      "vadd.f32      s28, s14, s15                   \n\t" /* out1 = oo0 + oo1 */
      "vadd.f32      s29, s15, s16                   \n\t" /* out3 = oo1 + oo2 */
      "vadd.f32      s30, s16, s31                   \n\t" /* out5 = oo2 + oo3 */
      "vst2.32       {q6, q7}, [%[out],:128]         \n\t"
    : [in] "+&r" (in),
      [out] "+&r" (out)
    : [dct8_k] "r" (dct8_k)
    : "memory", "cc",
      "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7" 
  );
}
#else
STATIC_INLINE void
MPG_DCT_8 (gfloat in[8], gfloat out[8])
{
  gfloat even_in[4];
  gfloat odd_in[4], odd_out[4];
  gfloat tmp[6];

  /* Even indices */
  even_in[0] = in[0] + in[7];
  even_in[1] = in[3] + in[4];
  even_in[2] = in[1] + in[6];
  even_in[3] = in[2] + in[5];

  tmp[0] = even_in[0] + even_in[1];
  tmp[1] = even_in[2] + even_in[3];
  tmp[2] = (even_in[0] - even_in[1]) * synth_cos64_table[7];
  tmp[3] = (even_in[2] - even_in[3]) * synth_cos64_table[23];
  tmp[4] = (gfloat) ((tmp[2] - tmp[3]) * INV_SQRT_2);

  out[0] = tmp[0] + tmp[1];
  out[2] = tmp[2] + tmp[3] + tmp[4];
  out[4] = (gfloat) ((tmp[0] - tmp[1]) * INV_SQRT_2);
  out[6] = tmp[4];

  /* Odd indices */
  odd_in[0] = (in[0] - in[7]) * synth_cos64_table[3];
  odd_in[1] = (in[1] - in[6]) * synth_cos64_table[11];
  odd_in[2] = (in[2] - in[5]) * synth_cos64_table[19];
  odd_in[3] = (in[3] - in[4]) * synth_cos64_table[27];

  tmp[0] = odd_in[0] + odd_in[3];
  tmp[1] = odd_in[1] + odd_in[2];
  tmp[2] = (odd_in[0] - odd_in[3]) * synth_cos64_table[7];
  tmp[3] = (odd_in[1] - odd_in[2]) * synth_cos64_table[23];
  tmp[4] = tmp[2] + tmp[3];
  tmp[5] = (gfloat) ((tmp[2] - tmp[3]) * INV_SQRT_2);

  odd_out[0] = tmp[0] + tmp[1];
  odd_out[1] = tmp[4] + tmp[5];
  odd_out[2] = (gfloat) ((tmp[0] - tmp[1]) * INV_SQRT_2);
  odd_out[3] = tmp[5];

  out[1] = odd_out[0] + odd_out[1];
  out[3] = odd_out[1] + odd_out[2];
  out[5] = odd_out[2] + odd_out[3];
  out[7] = odd_out[3];
}
#endif

#if defined(USE_ARM_NEON)

static const __CACHE_LINE_DECL_ALIGN(float dct16_k[8]) = {
  5.0241928618815567820e-01f, 5.2249861493968885462e-01f,
  5.6694403481635768927e-01f, 6.4682178335999007679e-01f,
  6.4682178335999007679e-01f, 1.0606776859903470633e+00f,
  1.7224470982383341955e+00f, 5.1011486186891552563e+00f
};

STATIC_INLINE void
MPG_DCT_16 (gfloat in[16], gfloat out[16])
{
  __CACHE_LINE_DECL_ALIGN(gfloat even_in[8]);
  __CACHE_LINE_DECL_ALIGN(gfloat even_out[8]);
  __CACHE_LINE_DECL_ALIGN(gfloat odd_in[8]);
  __CACHE_LINE_DECL_ALIGN(gfloat odd_out[8]);

  __asm__ volatile (
      "vld1.64       {q0-q1}, [%[dct16_k],:128]     \n\t" /* read dct16_k */
      "vld1.64       {q2-q3}, [%[in],:128]!         \n\t" /* read in */
      "vld1.64       {q4-q5}, [%[in],:128]          \n\t" /* read in */
      "vrev64.f32    q4, q4                         \n\t"
      "vrev64.f32    q5, q5                         \n\t"
      "vswp          d8, d9                         \n\t"
      "vswp          d10, d11                       \n\t"
      "vadd.f32      q6, q2, q4                     \n\t"
      "vadd.f32      q7, q3, q5                     \n\t"
      "vst1.64       {q6-q7},  [%[even_in],:128]    \n\t"
      "vsub.f32      q6, q2, q4                     \n\t"
      "vsub.f32      q7, q3, q5                     \n\t"
      "vmul.f32      q6, q6, q0                     \n\t"
      "vmul.f32      q7, q7, q1                     \n\t"
      "vst1.64       {q6-q7},  [%[odd_in],:128]     \n\t"
    : [in] "+&r" (in)
    : [even_in] "r" (even_in), [odd_in] "r" (odd_in), [dct16_k] "r" (dct8_k)
    : "memory", "cc",
      "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7"
  );

  MPG_DCT_8 (even_in, even_out);
  MPG_DCT_8 (odd_in, odd_out);

  __asm__ volatile (
      "vld1.64       {q0-q1}, [%[even_out],:128]    \n\t"
      "vld1.64       {q2-q3}, [%[odd_out],:128]     \n\t"
      "vswp          q1, q2                         \n\t"        
      "vadd.f32      s4, s4, s5                     \n\t"
      "vadd.f32      s5, s5, s6                     \n\t"
      "vadd.f32      s6, s6, s7                     \n\t"
      "vadd.f32      s7, s7, s12                    \n\t"
      "vst2.32       {q0-q1}, [%[out],:128]!        \n\t"
      "vadd.f32      s12, s12, s13                  \n\t"
      "vadd.f32      s13, s13, s14                  \n\t"
      "vadd.f32      s14, s14, s15                  \n\t"
      "vst2.32       {q2-q3}, [%[out],:128]!        \n\t"
    : [out] "+&r" (out)
    : [even_out] "r" (even_out), [odd_out] "r" (odd_out)
    : "memory", "cc",
      "q0", "q1", "q2", "q3"
  );
}
#else
STATIC_INLINE void
MPG_DCT_16 (gfloat in[16], gfloat out[16])
{
  __CACHE_LINE_DECL_ALIGN(gfloat even_in[8]);
  __CACHE_LINE_DECL_ALIGN(gfloat even_out[8]);
  __CACHE_LINE_DECL_ALIGN(gfloat odd_in[8]);
  __CACHE_LINE_DECL_ALIGN(gfloat odd_out[8]);
  gfloat a, b;

  a = in[0]; b = in[15];
  even_in[0] = a + b;
  odd_in[0] = (a - b) * synth_cos64_table[1];
  a = in[1]; b = in[14];
  even_in[1] = a + b;
  odd_in[1] = (a - b) * synth_cos64_table[5];
  a = in[2]; b = in[13];
  even_in[2] = a + b;
  odd_in[2] = (a - b) * synth_cos64_table[9];
  a = in[3]; b = in[12];
  even_in[3] = a + b;
  odd_in[3] = (a - b) * synth_cos64_table[13];
  a = in[4]; b = in[11];
  even_in[4] = a + b;
  odd_in[4] = (a - b) * synth_cos64_table[17];
  a = in[5]; b = in[10];
  even_in[5] = a + b;
  odd_in[5] = (a - b) * synth_cos64_table[21];
  a = in[6]; b = in[9];
  even_in[6] = a + b;
  odd_in[6] = (a - b) * synth_cos64_table[25];
  a = in[7]; b = in[8];
  even_in[7] = a + b;
  odd_in[7] = (a - b) * synth_cos64_table[29];

  MPG_DCT_8 (even_in, even_out);
  MPG_DCT_8 (odd_in, odd_out);
  
  out[0] = even_out[0];
  out[1] = odd_out[0] + odd_out[1];  
  out[2] = even_out[1];
  out[3] = odd_out[1] + odd_out[2];  
  out[4] = even_out[2];
  out[5] = odd_out[2] + odd_out[3];  
  out[6] = even_out[3];
  out[7] = odd_out[3] + odd_out[4];  
  out[8] = even_out[4];
  out[9] = odd_out[4] + odd_out[5];  
  out[10] = even_out[5];
  out[11] = odd_out[5] + odd_out[6];  
  out[12] = even_out[6];
  out[13] = odd_out[6] + odd_out[7];  
  out[14] = even_out[7];
  out[15] = odd_out[7];
}
#endif

STATIC_INLINE void
MPG_DCT_32 (gfloat in[32], gfloat out[32])
{
  gint i;
  __CACHE_LINE_DECL_ALIGN(gfloat even_in[16]);
  __CACHE_LINE_DECL_ALIGN(gfloat even_out[16]);
  __CACHE_LINE_DECL_ALIGN(gfloat odd_in[16]);
  __CACHE_LINE_DECL_ALIGN(gfloat odd_out[16]);

  for (i = 0; i < 16; i++) {
    even_in[i] = in[i] + in[31 - i];
    odd_in[i] = (in[i] - in[31 - i]) * synth_cos64_table[2 * i];
  }

  MPG_DCT_16 (even_in, even_out);
  MPG_DCT_16 (odd_in, odd_out);

  for (i = 0; i < 15; i++) {
    out[2 * i] = even_out[i];
    out[2 * i + 1] = odd_out[i] + odd_out[i + 1];
  }
  out[30] = even_out[15];
  out[31] = odd_out[15];
}

#if defined(USE_ARM_NEON)

#define WIN_MAC \
  "  vld1.32     {q0-q1}, [r3,:128], r5         \n\t" /* read win */           \
  "  vld1.32     {q2-q3}, [r4,:128], r5         \n\t" /* read uvec */          \
  "  pld         [r3]                           \n\t"                          \
  "  pld         [r4]                           \n\t"                          \
  "  vmla.f32    q4, q0, q2                     \n\t" /* acc += uvec * win */  \
  "  vmla.f32    q5, q1, q3                     \n\t"

STATIC_INLINE void
mp3_dewindow_output (gfloat *uvec, short *samples, gfloat* window)
{
  __asm__ volatile (
      "pld           [%[win]]                       \n\t"
      "pld           [%[uvec]]                      \n\t"
      "mov           r5, #32*4                      \n\t" /* step = 32 floats */
      "mov           ip, #4                         \n\t" /* ip = 4 */
      "0:                                           \n\t"
      "  add         r3, %[win], r5                 \n\t" /* pw = win */
      "  add         r4, %[uvec], r5                \n\t" /* puvec = uvec */
      "  vld1.32     {q0-q1}, [%[win],:128]!        \n\t" /* read win */
      "  vld1.32     {q2-q3}, [%[uvec],:128]!       \n\t" /* read uvec */
      "  pld         [r3]                           \n\t"
      "  pld         [r4]                           \n\t"
      "  vmul.f32    q4, q0, q2                     \n\t" /* acc = uvec * win */
      "  vmul.f32    q5, q1, q3                     \n\t"
      WIN_MAC
      WIN_MAC
      WIN_MAC
      WIN_MAC
      WIN_MAC
      WIN_MAC
      WIN_MAC
      WIN_MAC
      WIN_MAC
      WIN_MAC
      WIN_MAC
      WIN_MAC
      WIN_MAC
      WIN_MAC
      WIN_MAC
      "  vcvt.s32.f32 q4, q4, #31                   \n\t"
      "  vcvt.s32.f32 q5, q5, #31                   \n\t"
      "  vshrn.s32   d0, q4, #16                    \n\t"
      "  vshrn.s32   d1, q5, #16                    \n\t"
      "  vst1.64     {d0-d1}, [%[samp],:128]!       \n\t"
      "  pld         [%[win]]                       \n\t"
      "  pld         [%[uvec]]                      \n\t"
      "  subs        ip, ip, #1                     \n\t"
      "  bne         0b                             \n\t"

    : [win] "+&r" (window),
      [samp] "+&r" (samples),
      [uvec] "+&r" (uvec)
    :
    : "memory", "cc", "ip", "r3", "r4", "r5",
      "q0", "q1", "q2", "q3", "q4", "q5"
  );
}

#else
STATIC_INLINE void
mp3_dewindow_output (gfloat *u_vec, short *samples, gfloat* window)
{
  gint i;
  gfloat *u_vec0;

  /* dewindowing */
  for (i = 0; i < HAN_SIZE; i++)
    u_vec[i] *= dewindow[i];

  /* Now calculate 32 samples */
  for (i = 0; i < 32; i++) {
    gfloat sum;
    u_vec0 = u_vec + i;
    sum = u_vec0[1 << 5];
    sum += u_vec0[2 << 5];
    sum += u_vec0[3 << 5];
    sum += u_vec0[4 << 5];
    sum += u_vec0[5 << 5];
    sum += u_vec0[6 << 5];
    sum += u_vec0[7 << 5];
    sum += u_vec0[8 << 5];
    sum += u_vec0[9 << 5];
    sum += u_vec0[10 << 5];
    sum += u_vec0[11 << 5];
    sum += u_vec0[12 << 5];
    sum += u_vec0[13 << 5];
    sum += u_vec0[14 << 5];
    sum += u_vec0[15 << 5];
    u_vec0[0] += sum;
  }

  for (i = 0; i < 32; i++) {
    gfloat sample = u_vec[i];
    if (sample > 0) {
      sample = sample * SCALE + 0.5f;
      if (sample < (SCALE - 1)) {
        samples[i] = (short) (sample);
      } else {
        samples[i] = (short) (SCALE - 1);
      }
    } else {
      sample = sample * SCALE - 0.5f;
      if (sample > -SCALE) {
        samples[i] = (short) (sample);
      } else {
        samples[i] = (short) (-SCALE);
      }
    }
  }
}
#endif

#if defined(USE_ARM_NEON)
STATIC_INLINE void
build_uvec (gfloat *u_vec, gfloat *cur_synbuf, gint k)
{
  __asm__ volatile (
      "mov           ip, #8                         \n\t" /* i = 8 */
      "mov           r5, #512                       \n\t"
      "sub           r5, r5, #1                     \n\t" /* r5 = 511 */
      "veor          d0, d0                         \n\t"
      "0:                                           \n\t"
      "  add         r4, %[k], #16                  \n\t"      
      "  add         r4, %[cur_synbuf], r4, lsl #2  \n\t"
      "  pld         [r4]                           \n\t"
      "  mov         r3, %[u_vec]                   \n\t"
      "  vstr        s0, [r3, #16*4]                \n\t"
      "  vld1.64     {q1-q2}, [r4,:128]!            \n\t"
      "  vld1.64     {q3-q4}, [r4,:128]!            \n\t"
      "  vst1.64     {q1-q2}, [r3,:128]!            \n\t"
      "  vst1.64     {q3-q4}, [r3,:128]!            \n\t"
      "  add         r3, r3, #4                     \n\t"
      "  vneg.f32    q1, q1                         \n\t"
      "  vneg.f32    q2, q2                         \n\t"
      "  vneg.f32    q3, q3                         \n\t"
      "  vneg.f32    q4, q4                         \n\t"
      "  vrev64.f32  q1, q1                         \n\t"
      "  vrev64.f32  q2, q2                         \n\t"
      "  vrev64.f32  q3, q3                         \n\t"
      "  vrev64.f32  q4, q4                         \n\t"
      "  vswp        d2, d3                         \n\t"
      "  vswp        d6, d7                         \n\t"
      "  vswp        d8, d9                         \n\t"                  
      "  vswp        d4, d5                         \n\t"
      "  vst1.64     {q4}, [r3]!                    \n\t"
      "  vst1.64     {q3}, [r3]!                    \n\t"
      "  vst1.64     {q2}, [r3]!                    \n\t"
      "  vst1.64     {q1}, [r3]!                    \n\t"
      "  add         %[k], %[k], #32                \n\t" /* k += 32 */
      "  and         %[k], %[k], r5                 \n\t" /* k &= 511 */
      "  add         r4, %[cur_synbuf], %[k], lsl #2\n\t"
      "  pld         [r4]                           \n\t"
      "  add         r3, %[u_vec], #48*4            \n\t"
      "  vld1.64     {q1-q2}, [r4,:128]!            \n\t"
      "  vld1.64     {q3-q4}, [r4,:128]!            \n\t"
      "  vldr.32     s2, [r4]                       \n\t"
      "  vneg.f32    q1, q1                         \n\t"
      "  vneg.f32    q2, q2                         \n\t"
      "  vneg.f32    q3, q3                         \n\t"
      "  vneg.f32    q4, q4                         \n\t"
      "  vst1.64     {q1-q2}, [r3,:128]!            \n\t"
      "  vst1.64     {q3-q4}, [r3,:128]!            \n\t"
      "  vneg.f32    s2, s2                         \n\t"
      "  add         r3, %[u_vec], #32*4            \n\t"
      "  vrev64.f32  q1, q1                         \n\t"
      "  vrev64.f32  q3, q3                         \n\t"
      "  vrev64.f32  q4, q4                         \n\t"
      "  vrev64.f32  q2, q2                         \n\t"
      "  vswp        d2, d3                         \n\t"
      "  vswp        d6, d7                         \n\t"
      "  vswp        d8, d9                         \n\t"
      "  vswp        d4, d5                         \n\t"
      "  vstmia      r3!, {s2}                      \n\t"
      "  vstmia      r3!, {q4}                      \n\t"
      "  vstmia      r3!, {q3}                      \n\t"
      "  vstmia      r3!, {q2}                      \n\t"
      "  vstmia      r3!, {q1}                      \n\t"
      "  subs        ip, ip, #1                     \n\t" /* i-- */
      "  add         %[u_vec], %[u_vec], #64*4      \n\t"
      "  add         %[k], %[k], #32                \n\t" /* k += 32 */
      "  and         %[k], %[k], r5                 \n\t" /* k &= 511 */
      "  bne         0b                             \n\t"
    : [u_vec] "+&r" (u_vec), [k] "+&r" (k)
    : [cur_synbuf] "r" (cur_synbuf)
    : "memory", "cc", "r3", "r4", "r5", "ip",
      "q0", "q1", "q2", "q3", "q4"
  );
}
#else
STATIC_INLINE void
build_uvec (gfloat *u_vec, gfloat *cur_synbuf, gint k)
{
  gint i, j;

  for (j = 0; j < 8; j++) {
    for (i = 0; i < 16; i++) {
      /* Copy first 32 elements */
      u_vec [i] = cur_synbuf [k + i + 16];
      u_vec [i + 17] = -cur_synbuf [k + 31 - i];
    }
    
    /* k wraps at the synthesis buffer boundary  */
    k = (k + 32) & 511;

    for (i = 0; i < 16; i++) {
      /* Copy next 32 elements */
      u_vec [i + 32] = -cur_synbuf [k + 16 - i];
      u_vec [i + 48] = -cur_synbuf [k + i];
    }
    u_vec [16] = 0;

    /* k wraps at the synthesis buffer boundary  */
    k = (k + 32) & 511;
    u_vec += 64;
  }
}
#endif

/* Synthesis matrixing variant which uses a 32 point DCT */
static void
mp3_SubBandSynthesis (mp3tl * tl ATTR_UNUSED, frame_params * fr_ps,
    float *polyPhaseIn, gint channel, short *samples)
{
  gint k;
  gfloat *cur_synbuf = fr_ps->synbuf[channel];
  __CACHE_LINE_DECL_ALIGN(gfloat u_vec[HAN_SIZE]);

  /* Shift down 32 samples in the fifo, which should always leave room */
  k = fr_ps->bufOffset[channel];
  k = (k - 32) & 511;
  fr_ps->bufOffset[channel] = k;

  /* DCT part */
  MPG_DCT_32 (polyPhaseIn, cur_synbuf + k);

  /* Build the U vector */
  build_uvec (u_vec, cur_synbuf, k);

  /* Dewindow and output samples */
  mp3_dewindow_output (u_vec, samples, (gfloat*) dewindow);
}

/************************* Layer III routines **********************/

static gboolean
III_get_side_info (guint8 * data, III_side_info_t * si, frame_params * fr_ps)
{
  int ch, gr, i;
  int stereo = fr_ps->stereo;
  huffdec_bitbuf bb;

  h_setbuf (&bb, data, fr_ps->header.side_info_slots);

  if (fr_ps->header.version == MPEG_VERSION_1) {
    si->main_data_begin = h_getbits (&bb, 9);
    if (stereo == 1)
      si->private_bits = h_getbits (&bb, 5);
    else
      si->private_bits = h_getbits (&bb, 3);

    for (ch = 0; ch < stereo; ch++) {
      guint8 scfsi = (guint8) h_getbits (&bb, 4);
      si->scfsi[0][ch] = scfsi & 0x08;
      si->scfsi[1][ch] = scfsi & 0x04;
      si->scfsi[2][ch] = scfsi & 0x02;
      si->scfsi[3][ch] = scfsi & 0x01;
    }

    for (gr = 0; gr < 2; gr++) {
      for (ch = 0; ch < stereo; ch++) {
        gr_info_t *gi = &(si->gr[gr][ch]);

        gi->part2_3_length = h_getbits (&bb, 12);
        gi->big_values = h_getbits (&bb, 9);
        /* Add 116 to avoid doing it in the III_dequantize loop */
        gi->global_gain = h_getbits (&bb, 8) + 116;
        gi->scalefac_compress = h_getbits (&bb, 4);
        gi->window_switching_flag = h_get1bit (&bb);
        if (gi->window_switching_flag) {
          gi->block_type = h_getbits (&bb, 2);
          gi->mixed_block_flag = h_get1bit (&bb);
          gi->table_select[0] = h_getbits (&bb, 5);
          gi->table_select[1] = h_getbits (&bb, 5);
          for (i = 0; i < 3; i++)
            gi->subblock_gain[i] = h_getbits (&bb, 3);

          if (gi->block_type == 0) {
            GST_WARNING ("Side info bad: block_type == 0 in split block.");
            return FALSE;
          } else if (gi->block_type == 2 && gi->mixed_block_flag == 0) {
            gi->region0_count = 8;      /* MI 9; */
            gi->region1_count = 12;
          } else {
            gi->region0_count = 7;      /* MI 8; */
            gi->region1_count = 13;
          }
        } else {
          for (i = 0; i < 3; i++)
            gi->table_select[i] = h_getbits (&bb, 5);
          gi->region0_count = h_getbits (&bb, 4);
          gi->region1_count = h_getbits (&bb, 3);
          gi->block_type = 0;
        }
        gi->preflag = h_get1bit (&bb);
        /* Add 1 & multiply by 2 to avoid doing it in the III_dequantize loop */
        gi->scalefac_scale = 2 * (h_get1bit (&bb) + 1);
        gi->count1table_select = h_get1bit (&bb);
      }
    }
  } else {                      /* Layer 3 LSF */

    si->main_data_begin = h_getbits (&bb, 8);
    if (stereo == 1)
      si->private_bits = h_getbits (&bb, 1);
    else
      si->private_bits = h_getbits (&bb, 2);

    for (gr = 0; gr < 1; gr++) {
      for (ch = 0; ch < stereo; ch++) {
        gr_info_t *gi = &(si->gr[gr][ch]);

        gi->part2_3_length = h_getbits (&bb, 12);
        gi->big_values = h_getbits (&bb, 9);
        /* Add 116 to avoid doing it in the III_dequantize loop */
        gi->global_gain = h_getbits (&bb, 8) + 116;
        gi->scalefac_compress = h_getbits (&bb, 9);
        gi->window_switching_flag = h_get1bit (&bb);
        if (gi->window_switching_flag) {
          gi->block_type = h_getbits (&bb, 2);
          gi->mixed_block_flag = h_get1bit (&bb);
          gi->table_select[0] = h_getbits (&bb, 5);
          gi->table_select[1] = h_getbits (&bb, 5);
          for (i = 0; i < 3; i++)
            gi->subblock_gain[i] = h_getbits (&bb, 3);

          /* Set region_count parameters since they are
           * implicit in this case. */
          if (gi->block_type == 0) {
            GST_WARNING ("Side info bad: block_type == 0 in split block.\n");
            return FALSE;
          } else if (gi->block_type == 2 && gi->mixed_block_flag == 0) {
            gi->region0_count = 8;      /* MI 9; */
            gi->region1_count = 12;
          } else {
            gi->region0_count = 7;      /* MI 8; */
            gi->region1_count = 13;
          }
        } else {
          for (i = 0; i < 3; i++)
            gi->table_select[i] = h_getbits (&bb, 5);
          gi->region0_count = h_getbits (&bb, 4);
          gi->region1_count = h_getbits (&bb, 3);
          gi->block_type = 0;
        }

        gi->preflag = 0;
        /* Add 1 & multiply by 2 to avoid doing it in the III_dequantize loop */
        gi->scalefac_scale = 2 * (h_get1bit (&bb) + 1);
        gi->count1table_select = h_get1bit (&bb);
      }
    }
  }

  return TRUE;
}

static const gint slen_table[2][16] = {
  {0, 0, 0, 0, 3, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4},
  {0, 1, 2, 3, 0, 1, 2, 3, 1, 2, 3, 1, 2, 3, 2, 3}
};

struct
{
  gint l[23];
  gint s[15];
} static const sfBandIndex[] = {
  /* MPEG-1 */
  {
    /* 44.1 khz */
    { 0, 4, 8, 12, 16, 20, 24, 30, 36, 44, 52, 62, 74, 90, 110, 134, 162, 196,
      238, 288, 342, 418, 576},
    { 0, 4, 8, 12, 16, 22, 30, 40, 52, 66, 84, 106, 136, 192, 192 }
  }, {
    /* 48khz */
    { 0, 4, 8, 12, 16, 20, 24, 30, 36, 42, 50, 60, 72, 88, 106, 128, 156, 190,
      230, 276, 330, 384, 576},
    { 0, 4, 8, 12, 16, 22, 28, 38, 50, 64, 80, 100, 126, 192, 192 }
  }, {
    /* 32khz */
    { 0, 4, 8, 12, 16, 20, 24, 30, 36, 44, 54, 66, 82, 102, 126, 156, 194, 240,
      296, 364, 448, 550, 576 },
    { 0, 4, 8, 12, 16, 22, 30, 42, 58, 78, 104, 138, 180, 192, 192 }
  },
  /* MPEG-2 */
  {
    /* 22.05 khz */
    { 0, 6, 12, 18, 24, 30, 36, 44, 54, 66, 80, 96, 116, 140, 168, 200, 238,
      284, 336, 396, 464, 522, 576 },
    { 0, 4, 8, 12, 18, 24, 32, 42, 56, 74, 100, 132, 174, 192, 192 }
  }, {
    /* 24khz */
    { 0, 6, 12, 18, 24, 30, 36, 44, 54, 66, 80, 96, 114, 136, 162, 194, 232,
      278, 330, 394, 464, 540, 576 },
    { 0, 4, 8, 12, 18, 26, 36, 48, 62, 80, 104, 136, 180, 192, 192 }
  }, {
    /* 16 khz */
    { 0, 6, 12, 18, 24, 30, 36, 44, 54, 66, 80, 96, 116, 140, 168, 200, 238,
      284, 336, 396, 464, 522, 576 },
    { 0, 4, 8, 12, 18, 26, 36, 48, 62, 80, 104, 134, 174, 192, 192 }
  },
  /* MPEG-2.5 */
  {
    /* 11025 */
    { 0, 6, 12, 18, 24, 30, 36, 44, 54, 66, 80, 96, 116, 140, 168, 200, 238,
      284, 336, 396, 464, 522, 576 },
    { 0, 4, 8, 12, 18, 26, 36, 48, 62, 80, 104, 134, 174, 192, 192 }
  }, {
    /* 12khz */
    { 0, 6, 12, 18, 24, 30, 36, 44, 54, 66, 80, 96, 116, 140, 168, 200, 238,
      284, 336, 396, 464, 522, 576 },
    { 0, 4, 8, 12, 18, 26, 36, 48, 62, 80, 104, 134, 174, 192, 192 }
  }, {
    /* 8khz */
    { 0, 12, 24, 36, 48, 60, 72, 88, 108, 132, 160, 192, 232, 280, 336, 400,
      476, 566, 568, 570, 572, 574, 576 },
    { 0, 8, 16, 24, 36, 52, 72, 96, 124, 160, 162, 164, 166, 192, 192 }
  }
};

/* Offset into the sfBand table for each MPEG version */
static const guint sfb_offset[] = { 6, 0 /* invalid */ , 3, 0 };

static void
III_get_scale_factors (III_scalefac_t * scalefac, III_side_info_t * si,
    int gr, int ch, mp3tl * tl)
{
  int sfb, window;
  gr_info_t *gr_info = &(si->gr[gr][ch]);
  huffdec_bitbuf *bb = &tl->c_impl.bb;
  gint slen0, slen1;

  slen0 = slen_table[0][gr_info->scalefac_compress];
  slen1 = slen_table[1][gr_info->scalefac_compress];
  if (gr_info->window_switching_flag && (gr_info->block_type == 2)) {
    if (gr_info->mixed_block_flag) {    /* MIXED *//* NEW - ag 11/25 */
      for (sfb = 0; sfb < 8; sfb++)
        (*scalefac)[ch].l[sfb] = h_getbits (bb, slen0);

      for (sfb = 3; sfb < 6; sfb++)
        for (window = 0; window < 3; window++)
          (*scalefac)[ch].s[window][sfb] = h_getbits (bb, slen0);

      for ( /* sfb = 6 */ ; sfb < 12; sfb++)
        for (window = 0; window < 3; window++)
          (*scalefac)[ch].s[window][sfb] = h_getbits (bb, slen1);

      for (sfb = 12, window = 0; window < 3; window++)
        (*scalefac)[ch].s[window][sfb] = 0;
    } else {
      /* SHORT block */
      for (sfb = 0; sfb < 6; sfb++)
        for (window = 0; window < 3; window++)
          (*scalefac)[ch].s[window][sfb] = h_getbits (bb, slen0);
      for ( /* sfb = 6 */ ; sfb < 12; sfb++)
        for (window = 0; window < 3; window++)
          (*scalefac)[ch].s[window][sfb] = h_getbits (bb, slen1);

      for (window = 0; window < 3; window++)
        (*scalefac)[ch].s[window][12] = 0;
    }
  } else {
    gint i;
    const gint l_sfbtable[5] = { 0, 6, 11, 16, 21 };
    /* LONG types 0,1,3 */
    if (gr == 0) {
      for (sfb = 0; sfb < 11; sfb++) {
        (*scalefac)[ch].l[sfb] = h_getbits (bb, slen0);
      }
      for (sfb = 11; sfb < 21; sfb++) {
        (*scalefac)[ch].l[sfb] = h_getbits (bb, slen1);
      }
    } else {
      for (i = 0; i < 2; i++) {
        if (si->scfsi[i][ch] == 0) {
          for (sfb = l_sfbtable[i]; sfb < l_sfbtable[i + 1]; sfb++) {
            (*scalefac)[ch].l[sfb] = h_getbits (bb, slen0);
          }
        }
      }
      for ( /* i = 2 */ ; i < 4; i++) {
        if (si->scfsi[i][ch] == 0) {
          for (sfb = l_sfbtable[i]; sfb < l_sfbtable[i + 1]; sfb++) {
            (*scalefac)[ch].l[sfb] = h_getbits (bb, slen1);
          }
        }
      }
    }
    (*scalefac)[ch].l[21] = 0;
  }
}

/*** new MPEG2 stuff ***/

static const guint nr_of_sfb_block[6][3][4] = {
  {{6, 5, 5, 5}, {9, 9, 9, 9}, {6, 9, 9, 9}},
  {{6, 5, 7, 3}, {9, 9, 12, 6}, {6, 9, 12, 6}},
  {{11, 10, 0, 0}, {18, 18, 0, 0}, {15, 18, 0, 0}},
  {{7, 7, 7, 0}, {12, 12, 12, 0}, {6, 15, 12, 0}},
  {{6, 6, 6, 3}, {12, 9, 9, 6}, {6, 12, 9, 6}},
  {{8, 8, 5, 0}, {15, 12, 9, 0}, {6, 18, 9, 0}}
};

static void
III_get_LSF_scale_data (guint * scalefac_buffer, III_side_info_t * si,
    gint gr, gint ch, mp3tl * tl)
{
  short i, j, k;
  short blocktypenumber;
  short blocknumber = -1;

  gr_info_t *gr_info = &(si->gr[gr][ch]);
  guint scalefac_comp, int_scalefac_comp, new_slen[4];

  huffdec_bitbuf *bb = &tl->c_impl.bb;
  fr_header *hdr = &tl->fr_ps.header;

  scalefac_comp = gr_info->scalefac_compress;

  blocktypenumber = 0;
  if ((gr_info->block_type == 2) && (gr_info->mixed_block_flag == 0))
    blocktypenumber = 1;

  if ((gr_info->block_type == 2) && (gr_info->mixed_block_flag == 1))
    blocktypenumber = 2;

  if (!(((hdr->mode_ext == 1) || (hdr->mode_ext == 3)) && (ch == 1))) {
    if (scalefac_comp < 400) {
      new_slen[0] = (scalefac_comp >> 4) / 5;
      new_slen[1] = (scalefac_comp >> 4) % 5;
      new_slen[2] = (scalefac_comp % 16) >> 2;
      new_slen[3] = (scalefac_comp % 4);
      gr_info->preflag = 0;
      blocknumber = 0;
    } else if (scalefac_comp < 500) {
      new_slen[0] = ((scalefac_comp - 400) >> 2) / 5;
      new_slen[1] = ((scalefac_comp - 400) >> 2) % 5;
      new_slen[2] = (scalefac_comp - 400) % 4;
      new_slen[3] = 0;
      gr_info->preflag = 0;
      blocknumber = 1;
    } else if (scalefac_comp < 512) {
      new_slen[0] = (scalefac_comp - 500) / 3;
      new_slen[1] = (scalefac_comp - 500) % 3;
      new_slen[2] = 0;
      new_slen[3] = 0;
      gr_info->preflag = 1;
      blocknumber = 2;
    }
  }

  if ((((hdr->mode_ext == 1) || (hdr->mode_ext == 3)) && (ch == 1))) {
    /*   intensity_scale = scalefac_comp %2; */
    int_scalefac_comp = scalefac_comp >> 1;

    if (int_scalefac_comp < 180) {
      new_slen[0] = int_scalefac_comp / 36;
      new_slen[1] = (int_scalefac_comp % 36) / 6;
      new_slen[2] = (int_scalefac_comp % 36) % 6;
      new_slen[3] = 0;
      gr_info->preflag = 0;
      blocknumber = 3;
    } else if (int_scalefac_comp < 244) {
      new_slen[0] = ((int_scalefac_comp - 180) % 64) >> 4;
      new_slen[1] = ((int_scalefac_comp - 180) % 16) >> 2;
      new_slen[2] = (int_scalefac_comp - 180) % 4;
      new_slen[3] = 0;
      gr_info->preflag = 0;
      blocknumber = 4;
    } else if (int_scalefac_comp < 255) {
      new_slen[0] = (int_scalefac_comp - 244) / 3;
      new_slen[1] = (int_scalefac_comp - 244) % 3;
      new_slen[2] = 0;
      new_slen[3] = 0;
      gr_info->preflag = 0;
      blocknumber = 5;
    }
  }

  if (blocknumber < 0) {
    GST_WARNING ("Invalid block number");
    return;
  }

  k = 0;
  for (i = 0; i < 4; i++) {
    guint slen = new_slen[i];
    if (slen == 0) {
      for (j = nr_of_sfb_block[blocknumber][blocktypenumber][i]; j > 0; j--) {
        scalefac_buffer[k] = 0;
        k++;
      }
    } else {
      for (j = nr_of_sfb_block[blocknumber][blocktypenumber][i]; j > 0; j--) {
        scalefac_buffer[k] = h_getbits (bb, slen);
        k++;
      }
    }
  }
  for (; k < 45; k++)
    scalefac_buffer[k] = 0;
}

static void
III_get_LSF_scale_factors (III_scalefac_t * scalefac, III_side_info_t * si,
    int gr, int ch, mp3tl * tl)
{
  int sfb, k = 0, window;
  gr_info_t *gr_info = &(si->gr[gr][ch]);
  guint *scalefac_buffer;

  scalefac_buffer = tl->c_impl.scalefac_buffer;
  III_get_LSF_scale_data (scalefac_buffer, si, gr, ch, tl);

  if (gr_info->window_switching_flag && (gr_info->block_type == 2)) {
    if (gr_info->mixed_block_flag) {    /* MIXED *//* NEW - ag 11/25 */
      for (sfb = 0; sfb < 8; sfb++) {
        (*scalefac)[ch].l[sfb] = scalefac_buffer[k];
        k++;
      }
      for (sfb = 3; sfb < 12; sfb++)
        for (window = 0; window < 3; window++) {
          (*scalefac)[ch].s[window][sfb] = scalefac_buffer[k];
          k++;
        }
      for (sfb = 12, window = 0; window < 3; window++)
        (*scalefac)[ch].s[window][sfb] = 0;
    } else {                    /* SHORT */
      for (sfb = 0; sfb < 12; sfb++)
        for (window = 0; window < 3; window++) {
          (*scalefac)[ch].s[window][sfb] = scalefac_buffer[k];
          k++;
        }
      for (sfb = 12, window = 0; window < 3; window++)
        (*scalefac)[ch].s[window][sfb] = 0;
    }
  } else {                      /* LONG types 0,1,3 */
    for (sfb = 0; sfb < 21; sfb++) {
      (*scalefac)[ch].l[sfb] = scalefac_buffer[k];
      k++;
    }
    (*scalefac)[ch].l[21] = 0;
  }
}

#define HUFFBITS guint32
#define HTSIZE  34
#define MXOFF   250

/* do the huffman-decoding 						*/
/* note! for counta,countb -the 4 bit value is returned in y, discard x */
static inline gboolean
huffman_decoder (huffdec_bitbuf * bb, gint tnum, int *x, int *y, int *v, int *w)
{
  HUFFBITS level;
  guint point = 0;
  gboolean error = TRUE;
  const struct huffcodetab *h;

  g_return_val_if_fail (tnum >= 0 && tnum <= HTSIZE, FALSE);

  /* Grab a ptr to the huffman table to use */
  h = huff_tables + tnum;

  level = (guint32) (1) << (sizeof (HUFFBITS) * 8 - 1);

  /* table 0 needs no bits */
  if (h->treelen == 0) {
    *x = *y = *v = *w = 0;
    return TRUE;
  }

  /* Lookup in Huffman table. */
  do {
    if (h->val[point][0] == 0) {        /*end of tree */
      *x = h->val[point][1] >> 4;
      *y = h->val[point][1] & 0xf;

      error = FALSE;
      break;
    }
    if (h_get1bit (bb)) {
      while (h->val[point][1] >= MXOFF)
        point += h->val[point][1];
      point += h->val[point][1];
    } else {
      while (h->val[point][0] >= MXOFF)
        point += h->val[point][0];
      point += h->val[point][0];
    }
    level >>= 1;
  } while (level || (point < h->treelen));

  /* Check for error. */
  if (error) {
    /* set x and y to a medium value as a simple concealment */
    GST_WARNING ("Illegal Huffman code in data.");
    *x = (h->xlen - 1) << 1;
    *y = (h->ylen - 1) << 1;
  }

  /* Process sign encodings for quadruples tables. */
  if (h->quad_table) {
    *v = (*y >> 3) & 1;
    *w = (*y >> 2) & 1;
    *x = (*y >> 1) & 1;
    *y = *y & 1;

    if (*v && (h_get1bit (bb) == 1))
      *v = -*v;
    if (*w && (h_get1bit (bb) == 1))
      *w = -*w;
    if (*x && (h_get1bit (bb) == 1))
      *x = -*x;
    if (*y && (h_get1bit (bb) == 1))
      *y = -*y;
  }
  /* Process sign and escape encodings for dual tables. */
  else {
    /* x and y are reversed in the test bitstream.
       Reverse x and y here to make test bitstream work. */

    if (h->linbits && ((h->xlen - 1) == *x))
      *x += h_getbits (bb, h->linbits);
    if (*x && (h_get1bit (bb) == 1))
      *x = -*x;

    if (h->linbits && ((h->ylen - 1) == *y))
      *y += h_getbits (bb, h->linbits);
    if (*y && (h_get1bit (bb) == 1))
      *y = -*y;
  }

  return !error;
}

static gboolean
III_huffman_decode (gint is[SBLIMIT][SSLIMIT], III_side_info_t * si,
    gint ch, gint gr, gint part2_start, mp3tl * tl)
{
  guint i;
  int x, y;
  int v = 0, w = 0;
  gint h;                       /* Index of the huffman table to use */
  guint region1Start;
  guint region2Start;
  int sfreq;
  guint grBits;
  gr_info_t *gi = &(si->gr[gr][ch]);
  huffdec_bitbuf *bb = &tl->c_impl.bb;
  frame_params *fr_ps = &tl->fr_ps;

  /* Calculate index. */
  sfreq = sfb_offset[fr_ps->header.version] + fr_ps->header.srate_idx;

  /* Find region boundary for short block case. */
  if ((gi->window_switching_flag) && (gi->block_type == 2)) {
    /* Region2. */
    if (fr_ps->header.version == MPEG_VERSION_2_5
          && fr_ps->header.srate_idx == 2) {
      region1Start = 72;
	} else {
      region1Start = 36;          /* sfb[9/3]*3=36 */
    }
    region2Start = 576;         /* No Region2 for short block case. */
  } else {                      /* Find region boundary for long block case. */
    region1Start = sfBandIndex[sfreq].l[gi->region0_count + 1]; /* MI */
    region2Start = sfBandIndex[sfreq].l[gi->region0_count + gi->region1_count + 2];     /* MI */
  }

  /* Read bigvalues area. */
  /* i < SSLIMIT * SBLIMIT => gi->big_values < SSLIMIT * SBLIMIT/2 */
  for (i = 0; i < gi->big_values * 2; i += 2) {
    if (i < region1Start)
      h = gi->table_select[0];
    else if (i < region2Start)
      h = gi->table_select[1];
    else
      h = gi->table_select[2];

    if (!huffman_decoder (bb, h, &x, &y, &v, &w))
      return FALSE;
    is[i / SSLIMIT][i % SSLIMIT] = x;
    is[(i + 1) / SSLIMIT][(i + 1) % SSLIMIT] = y;
  }

  /* Read count1 area. */
  h = gi->count1table_select + 32;
  grBits = part2_start + gi->part2_3_length;

  while ((h_sstell (bb) < grBits) && (i + 3) < (SBLIMIT * SSLIMIT)) {
    if (!huffman_decoder (bb, h, &x, &y, &v, &w))
      return FALSE;

    is[i / SSLIMIT][i % SSLIMIT] = v;
    is[(i + 1) / SSLIMIT][(i + 1) % SSLIMIT] = w;
    is[(i + 2) / SSLIMIT][(i + 2) % SSLIMIT] = x;
    is[(i + 3) / SSLIMIT][(i + 3) % SSLIMIT] = y;
    i += 4;
  }

  if (h_sstell (bb) > grBits) {
    /* Didn't end exactly at the grBits boundary. Rewind one entry. */
    if (i >= 4)
      i -= 4;
    h_rewindNbits (bb, h_sstell (bb) - grBits);
  }

  /* Dismiss any stuffing Bits */
  if (h_sstell (bb) < grBits)
    h_flushbits (bb, grBits - h_sstell (bb));

  g_assert (i <= SSLIMIT * SBLIMIT);

  /* Zero out rest. */
  for (; i < SSLIMIT * SBLIMIT; i++)
    is[i / SSLIMIT][i % SSLIMIT] = 0;

  return TRUE;
}

static const gint pretab[22] =
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 3, 3, 3, 2, 0 };

static void
III_dequantize_sample (gint is[SBLIMIT][SSLIMIT],
    gfloat xr[SBLIMIT][SSLIMIT],
    III_scalefac_t * scalefac,
    gr_info_t * gr_info, gint ch, gint gr, frame_params * fr_ps)
{
  int ss, sb, cb = 0, sfreq;

//   int stereo = fr_ps->stereo;
  int next_cb_boundary;
  int cb_begin = 0;
  int cb_width = 0;
  gint tmp;
  gint16 pow_factor;
  gboolean is_short_blk;

  /* Calculate index. */
  sfreq = sfb_offset[fr_ps->header.version] + fr_ps->header.srate_idx;

  /* choose correct scalefactor band per block type, initalize boundary */
  if (gr_info->window_switching_flag && (gr_info->block_type == 2)) {
    if (gr_info->mixed_block_flag) {
      next_cb_boundary = sfBandIndex[sfreq].l[1];       /* LONG blocks: 0,1,3 */
    } else {
      next_cb_boundary = sfBandIndex[sfreq].s[1] * 3;   /* pure SHORT block */
      cb_width = sfBandIndex[sfreq].s[1];
      cb_begin = 0;
    }
  } else {
    next_cb_boundary = sfBandIndex[sfreq].l[1]; /* LONG blocks: 0,1,3 */
  }

  /* apply formula per block type */
  for (sb = 0; sb < SBLIMIT; sb++) {
    gint sb_off = sb * 18;
    is_short_blk = gr_info->window_switching_flag &&
        (((gr_info->block_type == 2) && (gr_info->mixed_block_flag == 0)) ||
        ((gr_info->block_type == 2) && gr_info->mixed_block_flag && (sb >= 2)));

    for (ss = 0; ss < SSLIMIT; ss++) {
      if (sb_off + ss == next_cb_boundary) {    /* Adjust critical band boundary */
        if (gr_info->window_switching_flag && (gr_info->block_type == 2)) {
          if (gr_info->mixed_block_flag) {
            if ((sb_off + ss) == sfBandIndex[sfreq].l[8]) {
              next_cb_boundary = sfBandIndex[sfreq].s[4] * 3;
              cb = 3;
              cb_width = sfBandIndex[sfreq].s[cb + 1] -
                  sfBandIndex[sfreq].s[cb];
              cb_begin = sfBandIndex[sfreq].s[cb] * 3;
            } else if ((sb_off + ss) < sfBandIndex[sfreq].l[8])
              next_cb_boundary = sfBandIndex[sfreq].l[(++cb) + 1];
            else {
              next_cb_boundary = sfBandIndex[sfreq].s[(++cb) + 1] * 3;
              cb_width = sfBandIndex[sfreq].s[cb + 1] -
                  sfBandIndex[sfreq].s[cb];
              cb_begin = sfBandIndex[sfreq].s[cb] * 3;
            }
          } else {
            next_cb_boundary = sfBandIndex[sfreq].s[(++cb) + 1] * 3;
            cb_width = sfBandIndex[sfreq].s[cb + 1] - sfBandIndex[sfreq].s[cb];
            cb_begin = sfBandIndex[sfreq].s[cb] * 3;
          }
        } else                  /* long blocks */
          next_cb_boundary = sfBandIndex[sfreq].l[(++cb) + 1];
      }

      /* Compute overall (global) scaling. */
      pow_factor = gr_info->global_gain;

      /* Do long/short dependent scaling operations. */
      if (is_short_blk) {
        pow_factor -=
            8 * gr_info->subblock_gain[((sb_off + ss) - cb_begin) / cb_width];
        pow_factor -= gr_info->scalefac_scale *
            (*scalefac)[ch].s[(sb_off + ss - cb_begin) / cb_width][cb];
      } else {
        /* LONG block types 0,1,3 & 1st 2 subbands of switched blocks */
        pow_factor -= gr_info->scalefac_scale *
            ((*scalefac)[ch].l[cb] + gr_info->preflag * pretab[cb]);
      }

#if 1
      /* g_assert (pow_factor >= 0 && pow_factor <
         (sizeof (pow_2_table) / sizeof (pow_2_table[0]))); */
      xr[sb][ss] = pow_2_table[pow_factor];
#else
      /* Old method using powf */
      pow_factor -= 326;
      if (pow_factor >= (-140))
        xr[sb][ss] = powf (2.0, 0.25 * (pow_factor));
      else
        xr[sb][ss] = 0;
#endif

      /* Scale quantized value. */
      tmp = is[sb][ss];
      if (tmp >= 0) {
        xr[sb][ss] *= pow_43_table[tmp];
      } else {
        xr[sb][ss] *= -1.0f * pow_43_table[-tmp];
      }
    }
  }
}

static void
III_reorder (gfloat xr[SBLIMIT][SSLIMIT], gfloat ro[SBLIMIT][SSLIMIT],
    gr_info_t * gr_info, frame_params * fr_ps)
{
  int sfreq;
  int sfb, sfb_start, sfb_lines;
  int sb, ss, window, freq, src_line, des_line;

  /* Calculate index. */
  sfreq = sfb_offset[fr_ps->header.version] + fr_ps->header.srate_idx;

  if (gr_info->window_switching_flag && (gr_info->block_type == 2)) {
    for (sb = 0; sb < SBLIMIT; sb++)
      for (ss = 0; ss < SSLIMIT; ss++)
        ro[sb][ss] = 0;

    if (gr_info->mixed_block_flag) {
      /* NO REORDER FOR LOW 2 SUBBANDS */
      for (sb = 0; sb < 2; sb++)
        for (ss = 0; ss < SSLIMIT; ss++) {
          ro[sb][ss] = xr[sb][ss];
        }
      /* REORDERING FOR REST SWITCHED SHORT */
      for (sfb = 3, sfb_start = sfBandIndex[sfreq].s[3],
          sfb_lines = sfBandIndex[sfreq].s[4] - sfb_start;
          sfb < 13; sfb++, sfb_start = sfBandIndex[sfreq].s[sfb],
          (sfb_lines = sfBandIndex[sfreq].s[sfb + 1] - sfb_start))
        for (window = 0; window < 3; window++)
          for (freq = 0; freq < sfb_lines; freq++) {
            src_line = sfb_start * 3 + window * sfb_lines + freq;
            des_line = (sfb_start * 3) + window + (freq * 3);
            ro[des_line / SSLIMIT][des_line % SSLIMIT] =
                xr[src_line / SSLIMIT][src_line % SSLIMIT];
          }
    } else {                    /* pure short */
      for (sfb = 0, sfb_start = 0, sfb_lines = sfBandIndex[sfreq].s[1];
          sfb < 13; sfb++, sfb_start = sfBandIndex[sfreq].s[sfb],
          (sfb_lines = sfBandIndex[sfreq].s[sfb + 1] - sfb_start))
        for (window = 0; window < 3; window++)
          for (freq = 0; freq < sfb_lines; freq++) {
            src_line = sfb_start * 3 + window * sfb_lines + freq;
            des_line = (sfb_start * 3) + window + (freq * 3);
            ro[des_line / SSLIMIT][des_line % SSLIMIT] =
                xr[src_line / SSLIMIT][src_line % SSLIMIT];
          }
    }
  } else {                      /*long blocks */
    for (sb = 0; sb < SBLIMIT; sb++)
      for (ss = 0; ss < SSLIMIT; ss++)
        ro[sb][ss] = xr[sb][ss];
  }
}

static void
III_i_stereo_k_values (gint is_pos, gfloat io, gint i, gfloat k[2][576])
{
  if (is_pos == 0) {
    k[0][i] = 1;
    k[1][i] = 1;
  } else if ((is_pos % 2) == 1) {
    k[0][i] = powf (io, ((is_pos + 1) / 2.0f));
    k[1][i] = 1;
  } else {
    k[0][i] = 1;
    k[1][i] = powf (io, (is_pos / 2.0f));
  }
}

static void
III_stereo (gfloat xr[2][SBLIMIT][SSLIMIT], gfloat lr[2][SBLIMIT][SSLIMIT],
    III_scalefac_t * scalefac, gr_info_t * gr_info, frame_params * fr_ps)
{
  int sfreq;
  int stereo = fr_ps->stereo;
  int ms_stereo = (fr_ps->header.mode == MPG_MD_JOINT_STEREO) &&
      (fr_ps->header.mode_ext & 0x2);
  int i_stereo = (fr_ps->header.mode == MPG_MD_JOINT_STEREO) &&
      (fr_ps->header.mode_ext & 0x1);
  int sfb;
  int i, j, sb, ss;
  short is_pos[SBLIMIT * SSLIMIT];
  gfloat is_ratio[SBLIMIT * SSLIMIT];
  gfloat io;
  gfloat k[2][SBLIMIT * SSLIMIT];

  int lsf = (fr_ps->header.version != MPEG_VERSION_1);

  if ((gr_info->scalefac_compress % 2) == 1) {
    io = 0.707106781188f;
  } else {
    io = 0.840896415256f;
  }

  /* Calculate index. */
  sfreq = sfb_offset[fr_ps->header.version] + fr_ps->header.srate_idx;

  /* intialization */
  for (i = 0; i < SBLIMIT * SSLIMIT; i++)
    is_pos[i] = 7;

  if ((stereo == 2) && i_stereo) {
    if (gr_info->window_switching_flag && (gr_info->block_type == 2)) {
      if (gr_info->mixed_block_flag) {
        int max_sfb = 0;

        for (j = 0; j < 3; j++) {
          int sfbcnt;

          sfbcnt = 2;
          for (sfb = 12; sfb >= 3; sfb--) {
            int lines;

            lines = sfBandIndex[sfreq].s[sfb + 1] - sfBandIndex[sfreq].s[sfb];
            i = 3 * sfBandIndex[sfreq].s[sfb] + (j + 1) * lines - 1;
            while (lines > 0) {
              if (xr[1][i / SSLIMIT][i % SSLIMIT] != 0.0f) {
                sfbcnt = sfb;
                sfb = -10;
                lines = -10;
              }
              lines--;
              i--;
            }
          }
          sfb = sfbcnt + 1;

          if (sfb > max_sfb)
            max_sfb = sfb;

          while (sfb < 12) {
            sb = sfBandIndex[sfreq].s[sfb + 1] - sfBandIndex[sfreq].s[sfb];
            i = 3 * sfBandIndex[sfreq].s[sfb] + j * sb;
            for (; sb > 0; sb--) {
              is_pos[i] = (*scalefac)[1].s[j][sfb];
              if (is_pos[i] != 7) {
                if (lsf) {
                  III_i_stereo_k_values (is_pos[i], io, i, k);
                } else {
                  is_ratio[i] = tanf (is_pos[i] * (float)(PI / 12));
                }
              }
              i++;
            }
            sfb++;
          }

          sb = sfBandIndex[sfreq].s[12] - sfBandIndex[sfreq].s[11];
          sfb = 3 * sfBandIndex[sfreq].s[11] + j * sb;
          sb = sfBandIndex[sfreq].s[13] - sfBandIndex[sfreq].s[12];

          i = 3 * sfBandIndex[sfreq].s[11] + j * sb;
          for (; sb > 0; sb--) {
            is_pos[i] = is_pos[sfb];
            is_ratio[i] = is_ratio[sfb];
            k[0][i] = k[0][sfb];
            k[1][i] = k[1][sfb];
            i++;
          }
        }
        if (max_sfb <= 3) {
          i = 2;
          ss = 17;
          sb = -1;
          while (i >= 0) {
            if (xr[1][i][ss] != 0.0f) {
              sb = i * 18 + ss;
              i = -1;
            } else {
              ss--;
              if (ss < 0) {
                i--;
                ss = 17;
              }
            }
          }
          i = 0;
          while (sfBandIndex[sfreq].l[i] <= sb)
            i++;
          sfb = i;
          i = sfBandIndex[sfreq].l[i];
          for (; sfb < 8; sfb++) {
            sb = sfBandIndex[sfreq].l[sfb + 1] - sfBandIndex[sfreq].l[sfb];
            for (; sb > 0; sb--) {
              is_pos[i] = (*scalefac)[1].l[sfb];
              if (is_pos[i] != 7) {
                if (lsf) {
                  III_i_stereo_k_values (is_pos[i], io, i, k);
                } else {
                  is_ratio[i] = tanf (is_pos[i] * (float)(PI / 12));
                }
              }
              i++;
            }
          }
        }
      } else {
        for (j = 0; j < 3; j++) {
          int sfbcnt;

          sfbcnt = -1;
          for (sfb = 12; sfb >= 0; sfb--) {
            int lines;

            lines = sfBandIndex[sfreq].s[sfb + 1] - sfBandIndex[sfreq].s[sfb];
            i = 3 * sfBandIndex[sfreq].s[sfb] + (j + 1) * lines - 1;
            while (lines > 0) {
              if (xr[1][i / SSLIMIT][i % SSLIMIT] != 0.0f) {
                sfbcnt = sfb;
                sfb = -10;
                lines = -10;
              }
              lines--;
              i--;
            }
          }
          sfb = sfbcnt + 1;
          while (sfb < 12) {
            sb = sfBandIndex[sfreq].s[sfb + 1] - sfBandIndex[sfreq].s[sfb];
            i = 3 * sfBandIndex[sfreq].s[sfb] + j * sb;
            for (; sb > 0; sb--) {
              is_pos[i] = (*scalefac)[1].s[j][sfb];
              if (is_pos[i] != 7) {
                if (lsf) {
                  III_i_stereo_k_values (is_pos[i], io, i, k);
                } else {
                  is_ratio[i] = tanf (is_pos[i] * (float)(PI / 12));
                }
              }
              i++;
            }
            sfb++;
          }

          sb = sfBandIndex[sfreq].s[12] - sfBandIndex[sfreq].s[11];
          sfb = 3 * sfBandIndex[sfreq].s[11] + j * sb;
          sb = sfBandIndex[sfreq].s[13] - sfBandIndex[sfreq].s[12];

          i = 3 * sfBandIndex[sfreq].s[11] + j * sb;
          for (; sb > 0; sb--) {
            is_pos[i] = is_pos[sfb];
            is_ratio[i] = is_ratio[sfb];
            k[0][i] = k[0][sfb];
            k[1][i] = k[1][sfb];
            i++;
          }
        }
      }
    } else {
      i = 31;
      ss = 17;
      sb = 0;
      while (i >= 0) {
        if (xr[1][i][ss] != 0.0f) {
          sb = i * 18 + ss;
          i = -1;
        } else {
          ss--;
          if (ss < 0) {
            i--;
            ss = 17;
          }
        }
      }
      i = 0;
      while (sfBandIndex[sfreq].l[i] <= sb)
        i++;
      sfb = i;
      i = sfBandIndex[sfreq].l[i];
      for (; sfb < 21; sfb++) {
        sb = sfBandIndex[sfreq].l[sfb + 1] - sfBandIndex[sfreq].l[sfb];
        for (; sb > 0; sb--) {
          is_pos[i] = (*scalefac)[1].l[sfb];
          if (is_pos[i] != 7) {
            if (lsf) {
              III_i_stereo_k_values (is_pos[i], io, i, k);
            } else {
              is_ratio[i] = tanf (is_pos[i] * (float)(PI / 12));
            }
          }
          i++;
        }
      }
      sfb = sfBandIndex[sfreq].l[20];
      if (i > sfBandIndex[sfreq].l[21])
        sb = 576 - i;
      else
        sb = 576 - sfBandIndex[sfreq].l[21];

      for (; sb > 0; sb--) {
        is_pos[i] = is_pos[sfb];
        is_ratio[i] = is_ratio[sfb];
        k[0][i] = k[0][sfb];
        k[1][i] = k[1][sfb];
        i++;
      }
    }
  }
#if 0
  for (ch = 0; ch < 2; ch++)
    for (sb = 0; sb < SBLIMIT; sb++)
      for (ss = 0; ss < SSLIMIT; ss++)
        lr[ch][sb][ss] = 0;
#else
  memset (lr, 0, sizeof (gfloat) * 2 * SBLIMIT * SSLIMIT);
#endif

  if (stereo == 2)
    for (sb = 0; sb < SBLIMIT; sb++)
      for (ss = 0; ss < SSLIMIT; ss++) {
        i = (sb * 18) + ss;
        if (is_pos[i] == 7) {
          if (ms_stereo) {
            lr[0][sb][ss] = (xr[0][sb][ss] + xr[1][sb][ss]) * 0.707106781188f;
            lr[1][sb][ss] = (xr[0][sb][ss] - xr[1][sb][ss]) * 0.707106781188f;
          } else {
            lr[0][sb][ss] = xr[0][sb][ss];
            lr[1][sb][ss] = xr[1][sb][ss];
          }
        } else if (i_stereo) {
          if (lsf) {
            lr[0][sb][ss] = xr[0][sb][ss] * k[0][i];
            lr[1][sb][ss] = xr[0][sb][ss] * k[1][i];
          } else {
            lr[0][sb][ss] = xr[0][sb][ss] * (is_ratio[i] / (1 + is_ratio[i]));
            lr[1][sb][ss] = xr[0][sb][ss] * (1 / (1 + is_ratio[i]));
          }
        } else {
          GST_WARNING ("Error in stereo processing");
        }
  } else                        /* mono , bypass xr[0][][] to lr[0][][] */
    for (sb = 0; sb < SBLIMIT; sb++)
      for (ss = 0; ss < SSLIMIT; ss++)
        lr[0][sb][ss] = xr[0][sb][ss];

}

static const gfloat cs_table[8] = {
  0.85749292571f, 0.88174199732f, 0.94962864910f, 0.98331459249f,
  0.99551781607f, 0.99916055818f, 0.99989919524f, 0.99999315507f
};

static const gfloat ca_table[8] = {
  -0.51449575543f, -0.47173196857f, -0.31337745420f, -0.18191319961f,
  -0.09457419253f, -0.04096558289f, -0.01419856857f, -0.00369997467f
};

static void
III_antialias (gfloat xr[SBLIMIT][SSLIMIT],
    gfloat hybridIn[SBLIMIT][SSLIMIT], gr_info_t * gr_info)
{
  gfloat bu, bd;                /* upper and lower butterfly inputs */
  int ss, sb, sblim;

  /* clear all inputs */
  for (sb = 0; sb < SBLIMIT; sb++)
    for (ss = 0; ss < SSLIMIT; ss++)
      hybridIn[sb][ss] = xr[sb][ss];

  if (gr_info->window_switching_flag && (gr_info->block_type == 2) &&
      !gr_info->mixed_block_flag)
    return;

  if (gr_info->window_switching_flag && gr_info->mixed_block_flag &&
      (gr_info->block_type == 2))
    sblim = 1;
  else
    sblim = SBLIMIT - 1;

  /* 31 alias-reduction operations between each pair of sub-bands */
  /* with 8 butterflies between each pair                         */

  for (sb = 0; sb < sblim; sb++)
    for (ss = 0; ss < 8; ss++) {
      bu = xr[sb][17 - ss];
      bd = xr[sb + 1][ss];
      hybridIn[sb][17 - ss] = (bu * cs_table[ss]) - (bd * ca_table[ss]);
      hybridIn[sb + 1][ss] = (bd * cs_table[ss]) + (bu * ca_table[ss]);
    }
}

static inline void imdct_9pt (gfloat invec[9], gfloat outvec[9]);

#define ICOS24(i) (cos24_table[(i)])
#define COS18(i) (cos18_table[(i)])
#define ICOS36_A(i) (icos72_table[4*(i)+1])
#define ICOS72_A(i) (icos72_table[2*(i)])

/* Short (12 point) version of the IMDCT performed
   as 2 x 3-point IMDCT */
static inline void
inv_mdct_s (gfloat invec[6], gfloat outvec[12])
{
  int i;
  gfloat H[6], h[6], even_idct[3], odd_idct[3], *tmp;
  gfloat t0, t1, t2;
  /* sqrt (3) / 2.0 */
  const gfloat sqrt32 = 0.8660254037844385965883020617184229195117950439453125f;

  /* Preprocess the input to the two 3-point IDCT's */
  tmp = invec;
  for (i = 1; i < 6; i++) {
    H[i] = tmp[0] + tmp[1];
    tmp++;
  }

  /* 3-point IMDCT */
  t0 = H[4] / 2.0f + invec[0];
  t1 = H[2] * sqrt32;
  even_idct[0] = t0 + t1;
  even_idct[1] = invec[0] - H[4];
  even_idct[2] = t0 - t1;
  /* END 3-point IMDCT */

  /* 3-point IMDCT */
  t2 = H[3] + H[5];

  t0 = (t2) / 2.0f + H[1];
  t1 = (H[1] + H[3]) * sqrt32;
  odd_idct[0] = t0 + t1;
  odd_idct[1] = H[1] - t2;
  odd_idct[2] = t0 - t1;
  /* END 3-point IMDCT */

  /* Post-Twiddle */
  odd_idct[0] *= ICOS24 (1);
  odd_idct[1] *= ICOS24 (5);
  odd_idct[2] *= ICOS24 (9);

  h[0] = (even_idct[0] + odd_idct[0]) * ICOS24 (0);
  h[1] = (even_idct[1] + odd_idct[1]) * ICOS24 (2);
  h[2] = (even_idct[2] + odd_idct[2]) * ICOS24 (4);

  h[3] = (even_idct[2] - odd_idct[2]) * ICOS24 (6);
  h[4] = (even_idct[1] - odd_idct[1]) * ICOS24 (8);
  h[5] = (even_idct[0] - odd_idct[0]) * ICOS24 (10);

  /* Rearrange the 6 values from the IDCT to the output vector */
  outvec[0] = h[3];
  outvec[1] = h[4];
  outvec[2] = h[5];
  outvec[3] = -h[5];
  outvec[4] = -h[4];
  outvec[5] = -h[3];
  outvec[6] = -h[2];
  outvec[7] = -h[1];
  outvec[8] = -h[0];
  outvec[9] = -h[0];
  outvec[10] = -h[1];
  outvec[11] = -h[2];
}

static inline void
inv_mdct_l (gfloat invec[18], gfloat outvec[36])
{
  int i;
  gfloat H[17], h[18], even[9], odd[9], even_idct[9], odd_idct[9], *tmp;

  for (i = 0; i < 17; i++)
    H[i] = invec[i] + invec[i + 1];

  even[0] = invec[0];
  odd[0] = H[0];
  tmp = H;
  for (i = 1; i < 9; i++) {
    even[i] = tmp[1];
    odd[i] = tmp[0] + tmp[2];
    tmp += 2;
  }

  imdct_9pt (even, even_idct);
  imdct_9pt (odd, odd_idct);

  for (i = 0; i < 9; i++) {
    odd_idct[i] *= ICOS36_A (i);
    h[i] = (even_idct[i] + odd_idct[i]) * ICOS72_A (i);
  }
  for ( /* i = 9 */ ; i < 18; i++) {
    h[i] = (even_idct[17 - i] - odd_idct[17 - i]) * ICOS72_A (i);
  }

  /* Rearrange the 18 values from the IDCT to the output vector */
  outvec[0] = h[9];
  outvec[1] = h[10];
  outvec[2] = h[11];
  outvec[3] = h[12];
  outvec[4] = h[13];
  outvec[5] = h[14];
  outvec[6] = h[15];
  outvec[7] = h[16];
  outvec[8] = h[17];

  outvec[9] = -h[17];
  outvec[10] = -h[16];
  outvec[11] = -h[15];
  outvec[12] = -h[14];
  outvec[13] = -h[13];
  outvec[14] = -h[12];
  outvec[15] = -h[11];
  outvec[16] = -h[10];
  outvec[17] = -h[9];

  outvec[35] = outvec[18] = -h[8];
  outvec[34] = outvec[19] = -h[7];
  outvec[33] = outvec[20] = -h[6];
  outvec[32] = outvec[21] = -h[5];
  outvec[31] = outvec[22] = -h[4];
  outvec[30] = outvec[23] = -h[3];
  outvec[29] = outvec[24] = -h[2];
  outvec[28] = outvec[25] = -h[1];
  outvec[27] = outvec[26] = -h[0];
}

static inline void
imdct_9pt (gfloat invec[9], gfloat outvec[9])
{
  int i;
  gfloat even_idct[5], odd_idct[4];
  gfloat t0, t1, t2;

  /* BEGIN 5 Point IMDCT */
  t0 = invec[6] / 2.0f + invec[0];
  t1 = invec[0] - invec[6];
  t2 = invec[2] - invec[4] - invec[8];

  even_idct[0] = t0 + invec[2] * COS18 (2)
      + invec[4] * COS18 (4) + invec[8] * COS18 (8);

  even_idct[1] = t2 / 2.0f + t1;
  even_idct[2] = t0 - invec[2] * COS18 (8)
      - invec[4] * COS18 (2) + invec[8] * COS18 (4);

  even_idct[3] = t0 - invec[2] * COS18 (4)
      + invec[4] * COS18 (8) - invec[8] * COS18 (2);

  even_idct[4] = t1 - t2;
  /* END 5 Point IMDCT */

  /* BEGIN 4 Point IMDCT */
  {
    gfloat odd1, odd2;
    odd1 = invec[1] + invec[3];
    odd2 = invec[3] + invec[5];
    t0 = (invec[5] + invec[7]) * 0.5f + invec[1];

    odd_idct[0] = t0 + odd1 * COS18 (2) + odd2 * COS18 (4);
    odd_idct[1] = (invec[1] - invec[5]) * 1.5f - invec[7];
    odd_idct[2] = t0 - odd1 * COS18 (8) - odd2 * COS18 (2);
    odd_idct[3] = t0 - odd1 * COS18 (4) + odd2 * COS18 (8);
  }
  /* END 4 Point IMDCT */

  /* Adjust for non power of 2 IDCT */
  odd_idct[0] += invec[7] * COS18 (8);
  odd_idct[1] -= invec[7] * COS18 (6);
  odd_idct[2] += invec[7] * COS18 (4);
  odd_idct[3] -= invec[7] * COS18 (2);

  /* Post-Twiddle */
  odd_idct[0] *= 0.5f / COS18 (1);
  odd_idct[1] *= 0.5f / COS18 (3);
  odd_idct[2] *= 0.5f / COS18 (5);
  odd_idct[3] *= 0.5f / COS18 (7);

  for (i = 0; i < 4; i++) {
    outvec[i] = even_idct[i] + odd_idct[i];
  }
  outvec[4] = even_idct[4];
  /* Mirror into the other half of the vector */
  for (i = 5; i < 9; i++) {
    outvec[i] = even_idct[8 - i] - odd_idct[8 - i];
  }
}

static void
inv_mdct (gfloat in[18], gfloat out[36], gint block_type)
{
  int i, j;

  if (block_type == 2) {
    gfloat tmp[12], tin[18], *tmpptr;
    for (i = 0; i < 36; i++) {
      out[i] = 0.0f;
    }

    /* The short blocks input vector has to be re-arranged */
    tmpptr = tin;
    for (i = 0; i < 3; i++) {
      gfloat *v = &(in[i]);     /* Input vector */
      for (j = 0; j < 6; j++) {
        tmpptr[j] = *v;
        v += 3;
      }
      tmpptr += 6;
    }

    for (i = 0; i < 18; i += 6) {
      tmpptr = &(out[i + 6]);

      inv_mdct_s (&(tin[i]), tmp);

      /* The three short blocks must be windowed and overlapped added
       * with each other */
      for (j = 0; j < 12; j++) {
        tmpptr[j] += tmp[j] * mdct_swin[2][j];
      }
    }                           /* end for (i... */
  } else {                      /* block_type != 2 */
    inv_mdct_l (in, out);

    /* Window the imdct result */
    for (i = 0; i < 36; i++)
      out[i] = out[i] * mdct_swin[block_type][i];
  }
}
static void
init_hybrid (mp3cimpl_info * c_impl)
{
  int i, j, k;

  for (i = 0; i < 2; i++)
    for (j = 0; j < SBLIMIT; j++)
      for (k = 0; k < SSLIMIT; k++)
        c_impl->prevblck[i][j][k] = 0.0f;
}

/* III_hybrid
 * Parameters:
 *   double fsIn[SSLIMIT]      - freq samples per subband in
 *   double tsOut[SSLIMIT]     - time samples per subband out
 *   int sb, ch
 *   gr_info_t *gr_info
 *   frame_params *fr_ps
 */
static void
III_hybrid (gfloat fsIn[SSLIMIT], gfloat tsOut[SSLIMIT], int sb, int ch,
    gr_info_t * gr_info, mp3tl * tl)
{
  gfloat rawout[36];
  int i;
  i = (gr_info->window_switching_flag && gr_info->mixed_block_flag &&
      (sb < 2)) ? 0 : gr_info->block_type;

  inv_mdct (fsIn, rawout, i);

  /* overlap addition */
  for (i = 0; i < SSLIMIT; i++) {
    tsOut[i] = rawout[i] + tl->c_impl.prevblck[ch][sb][i];
    tl->c_impl.prevblck[ch][sb][i] = rawout[i + 18];
  }
}

/* Invert the odd frequencies for odd subbands in preparation for polyphase
 * filtering */
static void
III_frequency_inversion (gfloat hybridOut[SBLIMIT][SSLIMIT],
    mp3tl * tl ATTR_UNUSED)
{
  guint ss, sb;

  for (ss = 1; ss < 18; ss += 2) {
    for (sb = 1; sb < SBLIMIT; sb += 2) {
      hybridOut[sb][ss] = -hybridOut[sb][ss];
    }
  }
}

static void
III_subband_synthesis (mp3tl * tl, frame_params * fr_ps,
    gfloat hybridOut[SBLIMIT][SSLIMIT], gint channel,
    short samples[SSLIMIT][SBLIMIT])
{
  gint ss, sb;
  gfloat polyPhaseIn[SBLIMIT];  /* PolyPhase Input. */

  for (ss = 0; ss < 18; ss++) {
    /* Each of the 32 subbands has 18 samples. On each iteration, we take
     * one sample from each subband, (32 samples), and use a 32 point DCT
     * to perform matrixing, and copy the result into the synthesis
     * buffer fifo. */
    for (sb = 0; sb < SBLIMIT; sb++) {
      polyPhaseIn[sb] = hybridOut[sb][ss];
    }

    mp3_SubBandSynthesis (tl, fr_ps, polyPhaseIn, channel,
        &(tl->pcm_sample[channel][ss][0]));
  }
}

static Mp3TlRetcode
c_decode_mp3 (mp3tl * tl)
{
  III_scalefac_t III_scalefac;
  III_side_info_t III_side_info;
  huffdec_bitbuf *bb;
  guint gr, ch, sb;
  gint diff;
  fr_header *hdr;
  guint8 side_info[32];         /* At most 32 bytes side info for MPEG-1 stereo */
  gboolean MainDataOK;

  hdr = &tl->fr_ps.header;
  bb = &tl->c_impl.bb;

  /* Check enough side_info data available */
  if (bs_bits_avail (tl->bs) < hdr->side_info_slots * 8)
    return MP3TL_ERR_NEED_DATA;

  bs_getbytes (tl->bs, side_info, hdr->side_info_slots);
  if (!III_get_side_info (side_info, &III_side_info, &tl->fr_ps)) {
    GST_DEBUG ("Bad side info");
    return MP3TL_ERR_BAD_FRAME;
  }

  /* Check enough main_data available */
  if (bs_bits_avail (tl->bs) < hdr->main_slots * 8)
    return MP3TL_ERR_NEED_DATA;

  /* Verify that sufficient main_data was extracted from */
  /* the previous sync interval */
  diff = tl->c_impl.main_data_end - III_side_info.main_data_begin;
  MainDataOK = (diff >= 0);
  if (!MainDataOK) {
    GST_DEBUG ("MainDataEnd: %d MainDataBegin: %d delta: %d",
        tl->c_impl.main_data_end, III_side_info.main_data_begin, diff);
  }

  /* Copy the remaining main data in the bit reservoir to the start of the
   * huffman bit buffer, and then append the incoming bytes */
  if (MainDataOK) {
    if (diff > 0) {
      memmove (tl->c_impl.hb_buf, tl->c_impl.hb_buf + diff,
          III_side_info.main_data_begin);
      tl->c_impl.main_data_end = III_side_info.main_data_begin;
    }
  }
  /* And append the incoming bytes to the reservoir */
  bs_getbytes (tl->bs, tl->c_impl.hb_buf + tl->c_impl.main_data_end,
      hdr->main_slots);
  tl->c_impl.main_data_end += hdr->main_slots;

  if (!MainDataOK) {
    GST_DEBUG ("Bad frame - not enough main data bits");
    return MP3TL_ERR_BAD_FRAME;
  }

  /* And setup the huffman bitstream reader for this data */
  h_setbuf (bb, tl->c_impl.hb_buf, tl->c_impl.main_data_end);

  /* Clear the scale factors to avoid problems with badly coded files
   * that try to reuse scalefactors from the first granule when they didn't
   * supply them. */
  memset (III_scalefac, 0, sizeof (III_scalefac_t));

  for (gr = 0; gr < tl->n_granules; gr++) {
    gfloat lr[2][SBLIMIT][SSLIMIT], ro[2][SBLIMIT][SSLIMIT];

    for (ch = 0; ch < hdr->channels; ch++) {
      gint is[SBLIMIT][SSLIMIT];        /* Quantized samples. */
      int part2_start;

      part2_start = h_sstell (bb);
      if (hdr->version == MPEG_VERSION_1) {
        III_get_scale_factors (&III_scalefac, &III_side_info, gr, ch, tl);
      } else {
        III_get_LSF_scale_factors (&III_scalefac, &III_side_info, gr, ch, tl);
      }

      if (III_side_info.gr[gr][ch].big_values > ((SBLIMIT * SSLIMIT) / 2)) {
        GST_DEBUG ("Bad side info decoding frame: big_values");
        return MP3TL_ERR_BAD_FRAME;
      }

      if (!III_huffman_decode (is, &III_side_info, ch, gr, part2_start, tl)) {
        GST_DEBUG ("Failed to decode huffman info");
        return MP3TL_ERR_BAD_FRAME;
      }

#ifdef HUFFMAN_DEBUG
      {
        gint i, j;
        fprintf (stderr, "\nFrame %" G_GUINT64_FORMAT ", granule %d, channel %d\n",
            tl->frame_num, gr, ch);
        for (i = 0 ; i < 32; i++) {
          fprintf (stderr, "SB %02d: ", i);
          for (j = 0; j < 18; j++) {
            fprintf (stderr, "%4d ", is[i][j]);
          }
          fprintf (stderr, "\n");
        }
      }
#endif

      III_dequantize_sample (is, ro[ch], &III_scalefac,
          &(III_side_info.gr[gr][ch]), ch, gr, &tl->fr_ps);

#ifdef DEQUANT_DEBUG
      {
        gint i, j;
        fprintf (stderr, "\nFrame %" G_GUINT64_FORMAT ", granule %d, channel %d\n",
            tl->frame_num, gr, ch);
        for (i = 0 ; i < 32; i++) {
          fprintf (stderr, "SB %02d: ", i);
          for (j = 0; j < 18; j++) {
            fprintf (stderr, "%+f ", ro[ch][i][j]);
          }
          fprintf (stderr, "\n");
        }
      }
#endif
    }

    III_stereo (ro, lr, &III_scalefac, &(III_side_info.gr[gr][0]), &tl->fr_ps);

    for (ch = 0; ch < hdr->channels; ch++) {
      gfloat re[SBLIMIT][SSLIMIT];
      gfloat hybridIn[SBLIMIT][SSLIMIT];        /* Hybrid filter input */
      gfloat hybridOut[SBLIMIT][SSLIMIT];       /* Hybrid filter out */
      gr_info_t *gi = &(III_side_info.gr[gr][ch]);

      III_reorder (lr[ch], re, gi, &tl->fr_ps);

      /* Antialias butterflies. */
      III_antialias (re, hybridIn, gi);
#if 0
      int i;
      g_print ("HybridIn\n");
      for (sb = 0; sb < SBLIMIT; sb++) {
        g_print ("SB %02d: ", sb);
        for (i = 0; i < SSLIMIT; i++) {
          g_print ("%06f ", hybridIn[sb][i]);
        }
        g_print ("\n");
      }
#endif

      for (sb = 0; sb < SBLIMIT; sb++) {
        /* Hybrid synthesis. */
        III_hybrid (hybridIn[sb], hybridOut[sb], sb, ch, gi, tl);
      }

      /* Frequency inversion for polyphase. Invert odd numbered indices */
      III_frequency_inversion (hybridOut, tl);

#if 0
      g_print ("HybridOut\n");
      for (sb = 0; sb < SBLIMIT; sb++) {
        g_print ("SB %02d: ", sb);
        for (i = 0; i < SSLIMIT; i++) {
          g_print ("%06f ", hybridOut[sb][i]);
        }
        g_print ("\n");
      }
#endif

      /* Polyphase synthesis */
      III_subband_synthesis (tl, &tl->fr_ps, hybridOut, ch, tl->pcm_sample[ch]);
#if 0
      if (ch == 0) {
        g_print ("synth\n");

        for (i = 0; i < SSLIMIT; i++) {
          g_print ("SS %02d: ", i);
          for (sb = 0; sb < SBLIMIT; sb++) {
            g_print ("%04d ", tl->pcm_sample[ch][sb][i]);
          }
          g_print ("\n");
        }
      }
#endif

    }
    /* Output PCM sample points for one granule. */
    out_fifo (tl->pcm_sample, 18, &tl->fr_ps, tl->sample_buf,
        &tl->sample_w, SAMPLE_BUF_SIZE);
  }

  return MP3TL_ERR_OK;
}

static gboolean
mp3_c_init (mp3tl * tl)
{
  init_hybrid (&tl->c_impl);
  return TRUE;
}

static void
mp3_c_flush (mp3tl * tl)
{
  init_hybrid (&tl->c_impl);
  h_reset (&tl->c_impl.bb);
  memset (tl->c_impl.hb_buf, 0, HDBB_BUFSIZE);
  tl->c_impl.main_data_end = 0;
}

/* Minimum size in bytes of an MP3 frame */
#define MIN_FRAME_SIZE 24

mp3tl *
mp3tl_new (Bit_stream_struc * bs, Mp3TlMode mode)
{
  mp3tl *tl;
  void *alloc_memory;

  g_return_val_if_fail (bs != NULL, NULL);
  g_return_val_if_fail (mode == MP3TL_MODE_16BIT, NULL);

  alloc_memory = calloc(1, (sizeof (mp3tl) + __CACHE_LINE_BYTES));

  tl = (mp3tl *) __CACHE_LINE_ALIGN(alloc_memory);
  g_return_val_if_fail (tl != NULL, NULL);

  tl->alloc_memory = alloc_memory;
  tl->bs = bs;
  tl->need_sync = TRUE;
  tl->need_header = TRUE;
  tl->at_eos = FALSE;
  tl->lost_sync = TRUE;

  tl->sample_size = 16;
  tl->sample_buf = NULL;
  tl->sample_w = 0;
  tl->stream_layer = 0;
  tl->error_count = 0;

  tl->fr_ps.alloc = NULL;
  init_syn_filter (&tl->fr_ps);

  tl->free_first = TRUE;

  if (!mp3_c_init (tl)) {
    free (tl);
    return NULL;
  }

  return tl;
}

void
mp3tl_free (mp3tl * tl)
{
  g_return_if_fail (tl != NULL);

  free (tl->alloc_memory);
};

void
mp3tl_set_eos (mp3tl * tl, gboolean is_eos)
{
  tl->at_eos = is_eos;
}

Mp3TlRetcode
mp3tl_sync (mp3tl * tl)
{
  g_return_val_if_fail (tl != NULL, MP3TL_ERR_PARAM);

  if (tl->need_sync) {
    guint64 sync_start ATTR_UNUSED;

    /* Find a sync word, with valid header */
    bs_reset (tl->bs);

    /* Need at least sync word + header bits */
    if (bs_bits_avail (tl->bs) < SYNC_WORD_LNGTH + HEADER_LNGTH)
      return MP3TL_ERR_NO_SYNC;

    sync_start = bs_pos (tl->bs);
    GST_LOG ("Starting sync search at %" G_GUINT64_FORMAT " (byte %"
        G_GUINT64_FORMAT ")", sync_start, sync_start / 8);

    do {
      gboolean sync;
      guint64 offset;
      guint64 frame_start;
      fr_header *hdr = &tl->fr_ps.header;
      gboolean valid = TRUE;
      guint64 total_offset ATTR_UNUSED;

      sync = bs_seek_sync (tl->bs);
      offset = bs_read_pos (tl->bs) - bs_pos (tl->bs);
      total_offset = bs_read_pos (tl->bs) - sync_start;

      if (!sync) {
        /* Leave the last byte in the stream, as it might be the first byte
         * of our sync word later */
        if (offset > 8)
          bs_consume (tl->bs, (guint32) (offset - 8));

        tl->lost_sync = TRUE;
        GST_LOG ("Not enough data in buffer for a sync sequence");
        return MP3TL_ERR_NO_SYNC;
      }
      g_assert (offset >= SYNC_WORD_LNGTH);

      /* Check if we skipped any data to find the sync word */
      if (offset != SYNC_WORD_LNGTH) {
        GST_DEBUG ("Skipped %" G_GUINT64_FORMAT " bits to find sync", offset);
        tl->lost_sync = TRUE;
      }

      /* Remember the start of frame */
      frame_start = bs_read_pos (tl->bs) - SYNC_WORD_LNGTH;

      /* Look ahead and check the header details */
      if (bs_bits_avail (tl->bs) < HEADER_LNGTH) {
        /* Consume bytes to the start of sync word and go get more data */
        bs_consume (tl->bs, (guint32) (offset - SYNC_WORD_LNGTH));
        tl->lost_sync = TRUE;

        GST_LOG ("Not enough data in buffer to read header");
        return MP3TL_ERR_NO_SYNC;
      }

      /* Read header bits */
      GST_LOG ("Reading header at %" G_GUINT64_FORMAT " (byte %"
          G_GUINT64_FORMAT ")", bs_read_pos (tl->bs), bs_read_pos (tl->bs) / 8);
      if (!read_header (tl, hdr)) {
        valid = FALSE;
        GST_LOG ("Bad header");
      } else {
        /* Fill out the derived header details */
        hdr->sample_size = tl->sample_size;
        if (!set_hdr_data_slots (hdr)) {
          GST_LOG ("Bad header (slots)");
          valid = FALSE;
        }

        /* Data is not allowed to suddenly change to a different layer */
        if (tl->stream_layer != 0 && hdr->layer != tl->stream_layer) {
          GST_LOG ("Bad header (layer changed)");
          valid = FALSE;
        }
      }

      /* FIXME: Could check the CRC to confirm a sync point */

      /* If we skipped any to find the sync, and we have enough data, 
       * jump ahead to where we expect the next frame to be and confirm 
       * that there is a sync word there */
      if (valid && tl->lost_sync) {
        gint64 remain;

        remain = hdr->frame_bits - (bs_read_pos (tl->bs) - frame_start);
        if (hdr->frame_bits < (8 * MIN_FRAME_SIZE)) {
          GST_LOG ("Header indicates a frame too small to be correct");
          valid = FALSE;
        } else if (bs_bits_avail (tl->bs) >= hdr->frame_bits) {
          guint32 sync_word;
          fr_header next_hdr;

          GST_DEBUG ("Peeking ahead %u bits to check sync (%"
              G_GINT64_FORMAT ", %" G_GUINT64_FORMAT ", %"
              G_GUINT64_FORMAT ")", hdr->frame_bits, remain,
              (guint64) bs_read_pos (tl->bs), (guint64) frame_start);

          /* Skip 'remain' bits */
          bs_skipbits (tl->bs, (guint32) (remain - 1));

          /* Read a sync word and check */
          sync_word = bs_getbits_aligned (tl->bs, SYNC_WORD_LNGTH);

          if (sync_word != SYNC_WORD) {
            valid = FALSE;
            GST_LOG ("No next sync word %u bits later @ %" G_GUINT64_FORMAT
                ". Got 0x%03x", hdr->frame_bits,
                bs_read_pos (tl->bs) - SYNC_WORD_LNGTH, sync_word);
          } else if (!read_header (tl, &next_hdr)) {
            GST_LOG ("Invalid header at next indicated frame");
            valid = FALSE;
          } else {
            /* Check that the samplerate and layer for the next header is 
             * the same */
            if ((hdr->layer != next_hdr.layer) ||
                (hdr->sample_rate != next_hdr.sample_rate) ||
                (hdr->copyright != next_hdr.copyright) ||
                (hdr->original != next_hdr.original) ||
                (hdr->emphasis != next_hdr.emphasis)) {
              valid = FALSE;
              GST_LOG ("Invalid header at next indicated frame");
            }
          }

          if (valid)
            GST_LOG ("Good - found a valid frame %u bits later.",
                hdr->frame_bits);
        } else if (!tl->at_eos) {
          GST_LOG ("Not enough data in buffer to test next header");

          /* Not at the end of stream, so wait for more data to validate the 
           * frame with */
          /* Consume bytes to the start of sync word and go get more data */
          bs_consume (tl->bs, (guint32) (offset - SYNC_WORD_LNGTH));
          return MP3TL_ERR_NO_SYNC;
        }
      }

      if (!valid) {
        /* Move past the first byte of the sync word and keep looking */
        bs_consume (tl->bs, (guint32) (offset - SYNC_WORD_LNGTH + 8));
      } else {
        /* Consume everything up to the start of sync word */
        if (offset > SYNC_WORD_LNGTH)
          bs_consume (tl->bs, (guint32) (offset - SYNC_WORD_LNGTH));

        tl->need_sync = FALSE;
        GST_DEBUG ("OK after %" G_GUINT64_FORMAT " offset",
            total_offset - SYNC_WORD_LNGTH);
      }
    } while (tl->need_sync);

    if (bs_pos (tl->bs) != sync_start)
      GST_DEBUG ("Skipped %" G_GUINT64_FORMAT " bits, found sync",
          bs_pos (tl->bs) - sync_start);
  }

  return MP3TL_ERR_OK;
}

Mp3TlRetcode
mp3tl_decode_header (mp3tl * tl, const fr_header ** ret_hdr)
{
  fr_header *hdr;
  Mp3TlRetcode ret;

  g_return_val_if_fail (tl != NULL, MP3TL_ERR_PARAM);
  hdr = &tl->fr_ps.header;
  if (G_LIKELY (ret_hdr != NULL))
    *ret_hdr = hdr;

  if (!tl->need_header)
    return MP3TL_ERR_OK;

  if ((ret = mp3tl_sync (tl)) != MP3TL_ERR_OK)
    return ret;

  /* Restart the read ptr and move past the sync word */
  bs_reset (tl->bs);
  bs_getbits (tl->bs, SYNC_WORD_LNGTH);

  /* If there are less than header bits available, something went 
   * wrong in the sync */
  g_assert (bs_bits_avail (tl->bs) >= HEADER_LNGTH);

  GST_DEBUG ("Frame is %d bytes (%d bits)",
      hdr->frame_bits / 8, hdr->frame_bits);

  /* Consume the header and sync word */
  bs_consume (tl->bs, SYNC_WORD_LNGTH + HEADER_LNGTH);

  tl->need_header = FALSE;
  return MP3TL_ERR_OK;
}

Mp3TlRetcode
mp3tl_gather_frame (mp3tl * tl, guint64 * _offset, gint * _length)
{
  guint64 sync_start ATTR_UNUSED;
  gboolean sync;
  guint64 offset;
  guint64 frame_start;
  fr_header *hdr;
  gboolean valid = TRUE;

  /* Find a sync word, with valid header */
  bs_reset (tl->bs);

  /* Need at least sync word + header bits */
  if (bs_bits_avail (tl->bs) < SYNC_WORD_LNGTH + HEADER_LNGTH)
    return MP3TL_ERR_NO_SYNC;

  sync_start = bs_pos (tl->bs);
  GST_LOG ("Starting sync search at %" G_GUINT64_FORMAT " (byte %"
      G_GUINT64_FORMAT ")", sync_start, sync_start / 8);

  hdr = &tl->fr_ps.header;
  sync = bs_seek_sync (tl->bs);
  offset = bs_read_pos (tl->bs) - bs_pos (tl->bs);

  if (!sync) {
    GST_LOG ("Not enough data for a sync sequence");
    return MP3TL_ERR_NO_SYNC;
  }

  /* Check if we skipped any data to find the sync word */
  if (offset != SYNC_WORD_LNGTH) {
    GST_DEBUG ("Skipped %" G_GUINT64_FORMAT " bits to find sync", offset);
  }

  /* Remember the start of frame */
  frame_start = bs_read_pos (tl->bs) - SYNC_WORD_LNGTH;

  /* Look ahead and check the header details */
  if (bs_bits_avail (tl->bs) < HEADER_LNGTH) {
    GST_LOG ("Not enough data to read header");
    return MP3TL_ERR_NO_SYNC;
  }

  /* Read header bits */
  GST_LOG ("Reading header at %" G_GUINT64_FORMAT " (byte %"
      G_GUINT64_FORMAT ")", bs_read_pos (tl->bs), bs_read_pos (tl->bs) / 8);
  if (!read_header (tl, hdr)) {
    valid = FALSE;
    GST_LOG ("Bad header");
  } else {
    /* Fill out the derived header details */
    hdr->sample_size = tl->sample_size;
    if (!set_hdr_data_slots (hdr)) {
      GST_LOG ("Bad header (slots)");
      valid = FALSE;
    }

    /* Data is not allowed to suddenly change to a different layer */
    if (tl->stream_layer != 0 && hdr->layer != tl->stream_layer) {
      GST_LOG ("Bad header (layer changed)");
      valid = FALSE;
    }
  }

  /* If we skipped any to find the sync, and we have enough data, 
   * jump ahead to where we expect the next frame to be and confirm 
   * that there is a sync word there */
  if (valid) {
    gint64 remain;

    remain = hdr->frame_bits - (bs_read_pos (tl->bs) - frame_start);
    if (hdr->frame_bits < (8 * MIN_FRAME_SIZE)) {
      GST_LOG ("Header indicates a frame too small to be correct");
    } else if (bs_bits_avail (tl->bs) >= hdr->frame_bits) {
      guint32 sync_word;
      fr_header next_hdr;

      GST_DEBUG ("Peeking ahead %u bits to check sync (%"
          G_GINT64_FORMAT ", %" G_GUINT64_FORMAT ", %"
          G_GUINT64_FORMAT ")", hdr->frame_bits, remain,
          (guint64) bs_read_pos (tl->bs), (guint64) frame_start);

      /* Skip 'remain' bits */
      bs_skipbits (tl->bs, (guint32) (remain - 1));

      /* Read a sync word and check */
      sync_word = bs_getbits_aligned (tl->bs, SYNC_WORD_LNGTH);

      if (sync_word != SYNC_WORD) {
        valid = FALSE;
        GST_LOG ("No next sync word %u bits later @ %" G_GUINT64_FORMAT
            ". Got 0x%03x", hdr->frame_bits,
            bs_read_pos (tl->bs) - SYNC_WORD_LNGTH, sync_word);
      } else if (!read_header (tl, &next_hdr)) {
        GST_LOG ("Invalid header at next indicated frame");
        valid = FALSE;
      } else {
        /* Check that the samplerate and layer for the next header is 
         * the same */
        if ((hdr->layer != next_hdr.layer) ||
            (hdr->sample_rate != next_hdr.sample_rate) ||
            (hdr->copyright != next_hdr.copyright) ||
            (hdr->original != next_hdr.original) ||
            (hdr->emphasis != next_hdr.emphasis)) {
          valid = FALSE;
          GST_LOG ("Invalid header at next indicated frame");
        }
      }

      if (valid) {
        GST_LOG ("Good - found a valid frame %u bits later.",
            hdr->frame_bits);
      }
    } else if (!tl->at_eos) {
      GST_LOG ("Not enough data in buffer to test next header");
      /* Not at the end of stream, so wait for more data to validate the 
       * frame with */
      return MP3TL_ERR_NO_SYNC;
    }
    *_offset = frame_start >> 3;
    *_length = hdr->frame_bits >> 3;
    tl->lost_sync = FALSE;
  } else {
    *_offset = (frame_start + SYNC_WORD_LNGTH) >> 3;
    return MP3TL_ERR_NO_SYNC;
  }

  return MP3TL_ERR_OK;
}

/*********************************************************************
 * Decode the current frame into the samples buffer
 *********************************************************************/
Mp3TlRetcode
mp3tl_decode_frame (mp3tl * tl, guint8 * samples, guint bufsize)
{
  fr_header *hdr;
  int i, j;
  int error_protection;
  guint new_crc;
  Mp3TlRetcode ret;
  gint64 frame_start_pos;

  g_return_val_if_fail (tl != NULL, MP3TL_ERR_PARAM);
  g_return_val_if_fail (samples != NULL, MP3TL_ERR_PARAM);

  hdr = &tl->fr_ps.header;

  if ((ret = mp3tl_decode_header (tl, NULL)) != MP3TL_ERR_OK)
    return ret;

  /* Check that the buffer is big enough to hold the decoded samples */
  if (bufsize < hdr->frame_samples * (hdr->sample_size / 8) * hdr->channels)
    return MP3TL_ERR_PARAM;

  bs_reset (tl->bs);

  GST_LOG ("Starting decode of frame size %u bits, with %u bits in buffer",
      hdr->frame_bits, (guint) bs_bits_avail (tl->bs));

  /* Got enough bits for the decode? (exclude the header) */
  if (bs_bits_avail (tl->bs) <
      hdr->frame_bits - (SYNC_WORD_LNGTH + HEADER_LNGTH))
    return MP3TL_ERR_NEED_DATA;

  hdr_to_frps (&tl->fr_ps);

  if (hdr->version == MPEG_VERSION_1)
    tl->n_granules = 2;
  else
    tl->n_granules = 1;

  tl->stream_layer = hdr->layer;

  error_protection = hdr->error_protection;

  /* We're about to start reading bits out of the stream, 
   * after which we'll need a new sync and header
   */
  tl->need_sync = TRUE;
  tl->need_header = TRUE;

  /* Set up the output buffer */
  tl->sample_w = 0;
  tl->sample_buf = (gint16 *) samples;

  /* Remember the start of the frame */
  frame_start_pos = bs_read_pos (tl->bs) - (SYNC_WORD_LNGTH + HEADER_LNGTH);

  /* Retrieve the CRC from the stream */
  if (error_protection)
    tl->old_crc = bs_getbits (tl->bs, 16);

  switch (hdr->layer) {
    case 1:{
      guint bit_alloc[2][SBLIMIT], scale_index[2][3][SBLIMIT];
      guint ch;

      I_decode_bitalloc (tl->bs, bit_alloc, &tl->fr_ps);
      I_decode_scale (tl->bs, bit_alloc, scale_index, &tl->fr_ps);

      /* Compute and check the CRC */
      if (error_protection) {
        I_CRC_calc (&tl->fr_ps, bit_alloc, &new_crc);
        if (new_crc != tl->old_crc) {
          tl->error_count++;
          GST_DEBUG ("CRC mismatch - Bad frame");
          return MP3TL_ERR_BAD_FRAME;
        }
      }

      for (i = 0; i < SCALE_BLOCK; i++) {
        I_buffer_sample (tl->bs, tl->sample, bit_alloc, &tl->fr_ps);
        I_dequant_and_scale_sample (tl->sample, tl->fraction, bit_alloc,
            scale_index, &tl->fr_ps);

        for (ch = 0; ch < hdr->channels; ch++) {
          mp3_SubBandSynthesis (tl, &tl->fr_ps, &(tl->fraction[ch][0][0]), ch,
              &((tl->pcm_sample)[ch][0][0]));
        }
        out_fifo (tl->pcm_sample, 1, &tl->fr_ps, tl->sample_buf, &tl->sample_w,
            SAMPLE_BUF_SIZE);
      }
      break;
    }

    case 2:{
      guint bit_alloc[2][SBLIMIT], scfsi[2][SBLIMIT],
          scale_index[2][3][SBLIMIT];
      guint ch;

      /* Choose bit allocations table */
      II_pick_table (&tl->fr_ps);

      /* Read band bit allocations from the data and scale */
      II_decode_bitalloc (tl->bs, bit_alloc, &tl->fr_ps);
      II_decode_scale (tl->bs, scfsi, bit_alloc, scale_index, &tl->fr_ps);

      if (error_protection) {
        II_CRC_calc (&tl->fr_ps, bit_alloc, scfsi, &new_crc);
        if (new_crc != tl->old_crc) {
          tl->error_count++;
          GST_DEBUG ("CRC mismatch - Bad frame");
          return MP3TL_ERR_BAD_FRAME;
        }
      }

      for (i = 0; i < SCALE_BLOCK; i++) {
        II_buffer_sample (tl->bs, (tl->sample), bit_alloc, &tl->fr_ps);
        II_dequant_and_scale_sample (tl->sample, bit_alloc, tl->fraction,
            scale_index, i >> 2, &tl->fr_ps);

        for (j = 0; j < 3; j++)
          for (ch = 0; ch < hdr->channels; ch++) {
            mp3_SubBandSynthesis (tl, &tl->fr_ps, &((tl->fraction)[ch][j][0]),
                ch, &((tl->pcm_sample)[ch][j][0]));
          }
        out_fifo (tl->pcm_sample, 3, &tl->fr_ps, tl->sample_buf, &tl->sample_w,
            SAMPLE_BUF_SIZE);
      }
      break;
    }
    case 3:
      ret = c_decode_mp3 (tl);
      if (ret == MP3TL_ERR_BAD_FRAME) {
        /* Consume the data from our bitreader */
        bs_consume (tl->bs, hdr->frame_bits - (SYNC_WORD_LNGTH + HEADER_LNGTH));
        return ret;
      }

      if (ret != MP3TL_ERR_OK)
        return ret;
      break;
    default:
      GST_WARNING ("Unknown layer %d, invalid bitstream.", hdr->layer);
      return MP3TL_ERR_STREAM;
  }

  /* skip ancillary data   HP 22-nov-95 */
  if (hdr->bitrate_idx != 0) {  /* if not free-format */
    /* Ancillary bits are any left in the frame that didn't get used */
    gint64 anc_len = hdr->frame_slots * hdr->bits_per_slot;

    anc_len -= bs_read_pos (tl->bs) - frame_start_pos;

    if (anc_len > 0) {
      GST_DEBUG ("Skipping %" G_GINT64_FORMAT " ancillary bits", anc_len);
      do {
        bs_getbits (tl->bs, (guint32) MIN (anc_len, MAX_LENGTH));
        anc_len -= MAX_LENGTH;
      } while (anc_len > 0);
    }
  }

  tl->frame_num++;
  tl->bits_used += hdr->frame_bits;

  /* Consume the data */
  bs_consume (tl->bs, hdr->frame_bits - (SYNC_WORD_LNGTH + HEADER_LNGTH));

  GST_DEBUG ("Used %u bits = %u slots plus %u", hdr->frame_bits,
      hdr->frame_slots, hdr->frame_bits % hdr->bits_per_slot);

  GST_DEBUG ("Avg slots/frame so far = %.3f; b/smp = %.2f; br = %.3f kbps",
      (float) tl->bits_used / (tl->frame_num * hdr->bits_per_slot),
      (float) tl->bits_used / (tl->frame_num * hdr->frame_samples),
      (float) (1.0/1000 * tl->bits_used) / (tl->frame_num * hdr->frame_samples) *
      s_rates[hdr->version][hdr->srate_idx]);

  /* Correctly decoded a frame, so assume we're synchronised */
  tl->lost_sync = FALSE;

  return MP3TL_ERR_OK;
}

void
mp3tl_flush (mp3tl * tl)
{
  GST_LOG ("Flush");
  /* Clear out the bytestreams */
  bs_flush (tl->bs);

  tl->need_header = TRUE;
  tl->need_sync = TRUE;
  tl->lost_sync = TRUE;

  init_syn_filter (&tl->fr_ps);

  tl->sample_buf = NULL;
  tl->sample_w = 0;
  memset (tl->pcm_sample, 0, sizeof (tl->pcm_sample));

  mp3_c_flush (tl);
}

Mp3TlRetcode
mp3tl_skip_frame (mp3tl * tl)
{
  fr_header *hdr;
  Mp3TlRetcode ret;

  g_return_val_if_fail (tl != NULL, MP3TL_ERR_PARAM);

  hdr = &tl->fr_ps.header;

  if ((ret = mp3tl_decode_header (tl, NULL)) != MP3TL_ERR_OK)
    return ret;

  bs_reset (tl->bs);

  /* Got enough bits to consume? (exclude the header) */
  if (bs_bits_avail (tl->bs) <
      hdr->frame_bits - (SYNC_WORD_LNGTH + HEADER_LNGTH))
    return MP3TL_ERR_NEED_DATA;

  hdr_to_frps (&tl->fr_ps);

  if (hdr->version == MPEG_VERSION_1)
    tl->n_granules = 2;
  else
    tl->n_granules = 1;

  tl->stream_layer = hdr->layer;

  /* We're about to start reading bits out of the stream, 
   * after which we'll need a new sync and header
   */
  tl->need_sync = TRUE;
  tl->need_header = TRUE;

  tl->frame_num++;
  tl->bits_used += hdr->frame_bits;

  /* Consume the data */
  bs_consume (tl->bs, hdr->frame_bits - (SYNC_WORD_LNGTH + HEADER_LNGTH));

  GST_DEBUG ("Skipped %u bits = %u slots plus %u", hdr->frame_bits,
      hdr->frame_slots, hdr->frame_bits % hdr->bits_per_slot);

  GST_DEBUG ("Avg slots/frame so far = %.3f; b/smp = %.2f; br = %.3f kbps",
      (float) tl->bits_used / (tl->frame_num * hdr->bits_per_slot),
      (float) tl->bits_used / (tl->frame_num * hdr->frame_samples),
      (float) (1.0/1000 * tl->bits_used) / (tl->frame_num * hdr->frame_samples) *
      s_rates[hdr->version][hdr->srate_idx]);

  return MP3TL_ERR_OK;
}

const char *
mp3tl_get_err_reason (mp3tl * tl)
{
  return tl->reason;
}

#define ID3_HDR_MIN 10

Mp3TlRetcode
mp3tl_skip_id3(mp3tl * tl)
{
  guint8 buf[ID3_HDR_MIN];

  bs_reset(tl->bs);

  if (bs_bits_avail(tl->bs) < 8 * ID3_HDR_MIN) {
    GST_DEBUG("Not enough data to read ID3 header");
    return MP3TL_ERR_NEED_DATA;
  }

  bs_getbytes(tl->bs, buf, ID3_HDR_MIN);

  /* skip ID3 tag, if present */
  if (!memcmp(buf, "ID3", 3)) {
    guint32 bytes_needed = (buf[6] << (7*3)) | (buf[7] << (7*2)) | (buf[8] << 7) | buf[9];

    if (bs_bits_avail(tl->bs) < 8 * bytes_needed) {
      GST_DEBUG("Not enough data to read ID3 tag (need %d)", bytes_needed);
      return MP3TL_ERR_NEED_DATA;
    }

    bs_consume(tl->bs, 8 * (ID3_HDR_MIN + bytes_needed));
    GST_DEBUG("ID3 tag found, skipping %d bytes", (ID3_HDR_MIN + bytes_needed));
  }

  bs_reset(tl->bs);
  return MP3TL_ERR_OK;
}

#define XING_HDR_MIN 8

Mp3TlRetcode
mp3tl_skip_xing(mp3tl * tl, const fr_header * hdr)
{
  const guint32 xing_id = 0x58696e67;     /* 'Xing' in hex */
  const guint32 info_id = 0x496e666f;     /* 'Info' in hex - found in LAME CBR files */
  guint32 xing_offset;
  guint32 read_id;

  if (hdr->version == MPEG_VERSION_1) {   /* MPEG-1 file */
    if (hdr->channels == 1)
      xing_offset = 0x11;
    else
      xing_offset = 0x20;
  } else {                                /* MPEG-2 header */
    if (hdr->channels == 1)
      xing_offset = 0x09;
    else
      xing_offset = 0x11;
  }

  bs_reset(tl->bs);

  if (bs_bits_avail(tl->bs) < 8 * (xing_offset + XING_HDR_MIN)) {
    GST_DEBUG("Not enough data to read Xing header");
    return MP3TL_ERR_NEED_DATA;
  }

  /* Read 4 bytes from the frame at the specified location */
  bs_skipbits(tl->bs, 8 * xing_offset);
  read_id = bs_getbits(tl->bs, 32);

  /* skip Xing header, if present */
  if (read_id == xing_id || read_id == info_id) {

    bs_consume(tl->bs, hdr->frame_bits);
    GST_DEBUG("Xing header found, skipping %d bytes", hdr->frame_bits / 8);
    return MP3TL_ERR_STREAM;

  } else {
    GST_DEBUG("No Xing header found");
  }

  bs_reset(tl->bs);
  return MP3TL_ERR_OK;
}

} // namespace flump3dec
