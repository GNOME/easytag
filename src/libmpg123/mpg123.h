/*
 * mpg123 defines 
 * used source: musicout.h from mpegaudio package
 */

#ifndef __MPG123_H__
#define __MPG123_H__


#include <stdio.h>
#include <string.h>
#include <math.h>
#include <gtk/gtk.h>

#include "dxhead.h"

#define real float

/*  #define         MAX_NAME_SIZE           81 */
#define         SBLIMIT                 32
#define         SCALE_BLOCK             12
#define         SSLIMIT                 18

#define         MPG_MD_STEREO           0
#define         MPG_MD_JOINT_STEREO     1
#define         MPG_MD_DUAL_CHANNEL     2
#define         MPG_MD_MONO             3


struct al_table
{
	short bits;
	short d;
};

struct frame
{
	struct al_table *alloc;
	int (*synth) (real *, int, unsigned char *, int *);
	int (*synth_mono) (real *, unsigned char *, int *);
	int stereo;
	int jsbound;
	int single;
	int II_sblimit;
	int down_sample_sblimit;
	int lsf;
	int mpeg25;
	int down_sample;
	int header_change;
	int lay;
	int (*do_layer) (struct frame * fr);
	int error_protection;
	int bitrate_index;
	int sampling_frequency;
	int padding;
	int extension;
	int mode;
	int mode_ext;
	int copyright;
	int original;
	int emphasis;
	int framesize;		/* computed framesize */
};


struct bitstream_info
{
	int bitindex;
	unsigned char *wordpointer;
};

extern struct bitstream_info bsi;

/* ------ Declarations from "common.c" ------ */
extern unsigned int mpg123_get1bit(void);
extern unsigned int mpg123_getbits(int);
extern unsigned int mpg123_getbits_fast(int);

extern int mpg123_head_check(unsigned long);

extern void mpg123_set_pointer(long);

extern unsigned char *mpg123_pcm_sample;
extern int mpg123_pcm_point;

struct gr_info_s
{
	int scfsi;
	unsigned part2_3_length;
	unsigned big_values;
	unsigned scalefac_compress;
	unsigned block_type;
	unsigned mixed_block_flag;
	unsigned table_select[3];
	unsigned subblock_gain[3];
	unsigned maxband[3];
	unsigned maxbandl;
	unsigned maxb;
	unsigned region1start;
	unsigned region2start;
	unsigned preflag;
	unsigned scalefac_scale;
	unsigned count1table_select;
	real *full_gain[3];
	real *pow2gain;
};

struct III_sideinfo
{
	unsigned main_data_begin;
	unsigned private_bits;
	struct
	{
		struct gr_info_s gr[2];
	}
	ch[2];
};

int mpg123_stream_check_for_xing_header(struct frame *fr, XHEADDATA * xhead);

extern int mpg123_do_layer3(struct frame *fr);
extern int mpg123_do_layer2(struct frame *fr);
extern int mpg123_do_layer1(struct frame *fr);


extern void mpg123_init_layer3(int);
extern void mpg123_init_layer2(void);

int    mpg123_decode_header(struct frame *fr, unsigned long newhead);
double mpg123_compute_bpf(struct frame *fr);
double mpg123_compute_tpf(struct frame *fr);


extern unsigned char *mpg123_conv16to8;
extern long mpg123_freqs[9];
extern real mpg123_muls[27][64];
extern real mpg123_decwin[512 + 32];
extern real *mpg123_pnts[5];

extern int tabsel_123[2][3][16];

guint mpg123_get_song_time(FILE * file);

#endif
