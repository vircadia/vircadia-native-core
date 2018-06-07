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

#ifndef __FLUMP3DEC_H__
#define __FLUMP3DEC_H__

#include <stdint.h>
#include <string.h>
#include <assert.h>

#if 0
#include <stdio.h>
#define G_GINT64_FORMAT "lld"
#define G_GUINT64_FORMAT "llu"

#define GST_LOG(f, ...)     do { printf(f "\n", __VA_ARGS__); } while (0)
#define GST_DEBUG(f, ...)   do { printf(f "\n", __VA_ARGS__); } while (0)
#define GST_WARNING(f, ...) do { printf(f "\n", __VA_ARGS__); } while (0)
#else
#define GST_LOG(f, ...)     do {} while (0)
#define GST_DEBUG(f, ...)   do {} while (0)
#define GST_WARNING(f, ...) do {} while (0)
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#define g_assert(cond)                  assert(cond)
#define g_return_if_fail(cond)          { if (!(cond)) return; }
#define g_return_val_if_fail(cond, val) { if (!(cond)) return (val); }

namespace flump3dec {

typedef char gchar;
typedef unsigned char guchar;
typedef int gint;
typedef unsigned int guint;
typedef float gfloat;
typedef double gdouble;
typedef int gboolean;
typedef size_t gsize;

typedef int8_t gint8;
typedef uint8_t guint8;
typedef int16_t gint16;
typedef uint16_t guint16;
typedef int32_t gint32;
typedef uint32_t guint32;
typedef int64_t gint64;
typedef uint64_t guint64;

/* Accumulator optimization on bitstream management */
#define ENABLE_OPT_BS 1

/* Bit stream reader definitions */
#define MAX_LENGTH      32      /* Maximum length of word written or
                                   read from bit stream */
#define BS_BYTE_SIZE    8

#if ENABLE_OPT_BS
#define BS_ACUM_SIZE 32
#else
#define BS_ACUM_SIZE 8
#endif

typedef struct BSReader
{
  guint64 bitpos;               /* Number of bits read so far */

  gsize size;                   /* Number of bytes in the buffer list */
  const guint8 *data;           /* Current data buffer */
  guint8 *cur_byte;             /* ptr to the current byte */
  guint8 cur_bit;               /* the next bit to be used in the current byte,
                                 * numbered from 8 down to 1 */
  gsize cur_used;               /* Number of bytes _completely_ consumed out of
                                 * the 'cur buffer' */
} BSReader;

typedef struct Bit_stream_struc
{
  BSReader master;              /* Master tracking position, advanced
                                 * by bs_consume() */
  BSReader read;                /* Current read position, set back to the 
                                 * master by bs_reset() */
} Bit_stream_struc;

/* Create and initialise a new bitstream reader */
Bit_stream_struc *bs_new ();

/* Release a bitstream reader */
void bs_free (Bit_stream_struc * bs);

/* Reset the current read position to the master position */
static inline void
bs_reset (Bit_stream_struc * bs)
{
  memcpy (&bs->read, &bs->master, sizeof (BSReader));
}

/* Reset master and read states */
static inline void
bs_flush (Bit_stream_struc * bs)
{
  g_return_if_fail (bs != NULL);

  bs->master.cur_bit = 8;
  bs->master.size = 0;
  bs->master.cur_used = 0;
  bs->master.cur_byte = NULL;
  bs->master.data = NULL;
  bs->master.bitpos = 0;

  bs_reset (bs);
}

/* Set data as the stream for processing */
gboolean bs_set_data (Bit_stream_struc * bs, const guint8 * data, gsize size);

/* Advance the master position by Nbits */
void bs_consume (Bit_stream_struc * bs, guint32 Nbits);

/* Number of bits available for reading */
static inline gsize bs_bits_avail (Bit_stream_struc * bs)
{
  return ((bs->read.size - bs->read.cur_used) * 8 + (bs->read.cur_bit - 8));
}

/* Extract N bytes from the bitstream into the out array. */
void bs_getbytes (Bit_stream_struc * bs, guint8 * out, guint32 N);

/* Advance the read pointer by N bits */
void bs_skipbits (Bit_stream_struc * bs, guint32 N);

/* give number of consumed bytes */
static inline gsize bs_get_consumed (Bit_stream_struc * bs)
{
  return bs->master.cur_used;
}

/* Current bitstream position in bits */
static inline guint64
bs_pos (Bit_stream_struc * bs)
{
  return bs->master.bitpos;
}

/* Current read bitstream position in bits */
static inline guint64
bs_read_pos (Bit_stream_struc * bs)
{
  return bs->read.bitpos;
}

/* Advances the read position to the first bit of next frame or
 * last byte in the buffer when the sync code is not found */
gboolean bs_seek_sync (Bit_stream_struc * bs);

/* Read N bits from the stream */
/* bs - bit stream structure */
/* N  - number of bits to read from the bit stream */
/* v  - output value */
static inline guint32
bs_getbits (Bit_stream_struc * bs, guint32 N)
{
  guint32 val = 0;
  gint j = N;

  g_assert (N <= MAX_LENGTH);

  while (j > 0) {
    gint tmp;
    gint k;
    gint mask;

    /* Move to the next byte if we consumed the current one */
    if (bs->read.cur_bit == 0) {
      bs->read.cur_bit = 8;
      bs->read.cur_used++;
      bs->read.cur_byte++;
    }

    /* Protect against data limit */
    if ((bs->read.cur_used >= bs->read.size)) {
      GST_WARNING ("Attempted to read beyond data");
      /* Return the bits we got so far */
      return val;
    }
    /* Take as many bits as we can from the current byte */
    k = MIN (j, bs->read.cur_bit);

    /* We want the k bits from the current byte, starting from
     * the cur_bit. Mask out the top 'already used' bits, then shift
     * the bits we want down to the bottom */
    mask = (1 << bs->read.cur_bit) - 1;
    tmp = bs->read.cur_byte[0] & mask;

    /* Trim off the bits we're leaving for next time */
    tmp = tmp >> (bs->read.cur_bit - k);

    /* Adjust our tracking vars */
    bs->read.cur_bit -= k;
    j -= k;
    bs->read.bitpos += k;

    /* Put these bits in the right spot in the output */
    val |= tmp << j;
  }

  return val;
}

/* Read 1 bit from the stream */
static inline guint32
bs_get1bit (Bit_stream_struc * bs)
{
  return bs_getbits (bs, 1);
}

/* read the next byte aligned N bits from the bit stream */
static inline guint32
bs_getbits_aligned (Bit_stream_struc * bs, guint32 N)
{
  guint32 align;

  align = bs->read.cur_bit;
  if (align != 8 && align != 0)
    bs_getbits (bs, align);

  return bs_getbits (bs, N);
}

/* MPEG Header Definitions - ID Bit Values */
#define         MPEG_VERSION_1          0x03
#define         MPEG_VERSION_2          0x02
#define         MPEG_VERSION_2_5        0x00

/* Header Information Structure */
typedef struct
{
  /* Stuff read straight from the MPEG header */
  guint version;
  guint layer;
  gboolean error_protection;

  gint bitrate_idx;             /* Index into the bitrate tables */
  guint srate_idx;              /* Index into the sample rate table */

  gboolean padding;
  gboolean extension;
  guint mode;
  guint mode_ext;
  gboolean copyright;
  gboolean original;
  guint emphasis;

  /* Derived attributes */
  guint bitrate;                /* Bitrate of the frame, kbps */
  guint sample_rate;            /* sample rate in Hz */
  guint sample_size;            /* in bits */
  guint frame_samples;          /* Number of samples per channels in this
                                   frame */
  guint channels;               /* Number of channels in the frame */

  guint bits_per_slot;          /* Number of bits per slot */
  guint frame_slots;            /* Total number of data slots in this frame */
  guint main_slots;             /* Slots of main data in this frame */
  guint frame_bits;             /* Number of bits in the frame, including header
                                   and sync word */
  guint side_info_slots;        /* Number of slots of side info in the frame */
} fr_header;

typedef struct mp3tl mp3tl;
typedef enum
{
  MP3TL_ERR_OK = 0,             /* Successful return code */
  MP3TL_ERR_NO_SYNC,            /* There was no sync word in the data buffer */
  MP3TL_ERR_NEED_DATA,          /* Not enough data in the buffer for the requested op */
  MP3TL_ERR_BAD_FRAME,          /* The frame data was corrupt and skipped */
  MP3TL_ERR_STREAM,             /* Encountered invalid data in the stream */
  MP3TL_ERR_UNSUPPORTED_STREAM, /* Encountered valid but unplayable data in 
                                 * the stream */
  MP3TL_ERR_PARAM,              /* Invalid parameter was passed in */
  MP3TL_ERR_UNKNOWN             /* Unspecified internal decoder error (bug) */
} Mp3TlRetcode;

typedef enum
{
  MP3TL_MODE_16BIT = 0          /* Decoder mode to use */
} Mp3TlMode;

mp3tl *mp3tl_new (Bit_stream_struc * bs, Mp3TlMode mode);

void mp3tl_free (mp3tl * tl);

void mp3tl_set_eos (mp3tl * tl, gboolean more_data);
Mp3TlRetcode mp3tl_sync (mp3tl * tl);
Mp3TlRetcode mp3tl_gather_frame (mp3tl * tl, guint64 * _offset, gint * _length);
Mp3TlRetcode mp3tl_decode_header (mp3tl * tl, const fr_header ** ret_hdr);
Mp3TlRetcode mp3tl_skip_frame (mp3tl * tl);
Mp3TlRetcode mp3tl_decode_frame (mp3tl * tl, guint8 * samples, guint bufsize);
const char *mp3tl_get_err_reason (mp3tl * tl);
void mp3tl_flush (mp3tl * tl);

Mp3TlRetcode mp3tl_skip_id3 (mp3tl * tl);
Mp3TlRetcode mp3tl_skip_xing (mp3tl * tl, const fr_header * hdr);

} // namespace flump3dec

#endif //__FLUMP3DEC_H__
