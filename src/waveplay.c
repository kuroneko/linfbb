/************************************************************************
    Copyright (C) 1986-2000 by

    F6FBB - Jean-Paul ROUBELAT
    jpr@f6fbb.org

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Parts of code have been taken from many other softwares.
    Thanks for the help.
************************************************************************/
/*
   vplay.c - plays and records 
             CREATIVE LABS VOICE-files, Microsoft WAVE-files and raw data

   Autor:    Michael Beck - beck@informatik.hu-berlin.de
*/

#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#ifndef __STDC__
#include <getopt.h>
#endif /* __STDC__ */
#include <fcntl.h>
#include <sys/ioctl.h>
#ifdef __STDC__
#include <sys/soundcard.h>
#else /* __STDC__ */
#include <linux/soundcard.h>
#endif /* __STDC__ */
#include "fmtheaders.h"

/* trying to define independent ioctl for sounddrivers version 1 and 2+ ,
   but it dosn't work everywere */
#ifdef SOUND_VERSION
#define IOCTL(a,b,c)		ioctl(a,b,&c)
#else
#define IOCTL(a,b,c)		(c = ioctl(a,b,c) )
#endif

#define DEFAULT_DSP_SPEED 	8000

#define AUDIO "/dev/dsp"

#define min(a,b) 		((a) <= (b) ? (a) : (b))

#define VOC_FMT			0
#define WAVE_FMT		1
#define RAW_DATA		2

/* global data */

static int dsp_speed = DEFAULT_DSP_SPEED, dsp_stereo = 0;
static int samplesize   = 8;
static int quiet_mode   = 0;
static u_long count;
static int audio, abuf_size, zbuf_size;
static u_char *audiobuf, *zerobuf;

/* needed prototypes */

static int r_play (char *name);

/*
 test, if it's a .WAV file, 0 if ok (and set the speed, stereo etc.)
                          < 0 if not
*/
int test_wavefile(void *buffer)
{
  WaveHeader *wp = buffer;

  if (wp->main_chunk == RIFF && wp->chunk_type == WAVE &&
      wp->sub_chunk == FMT && wp->data_chunk == DATA) 
    {
      if (wp->format != PCM_CODE) 
	{
	  fprintf (stderr, "can't play not PCM-coded WAVE-files\n");
	  return (-1);
	}
      if (wp->modus > 2) 
	{
	  fprintf (stderr, "can't play WAVE-files with %d tracks\n", wp->modus);
	  return (-1);
	}
      
      dsp_stereo = (wp->modus == WAVE_STEREO) ? 1 : 0;
      samplesize = wp->bit_p_spl;
      dsp_speed = wp->sample_fq; 
      count = wp->data_length;
      return 0;
    }
  return -1;
}
 
/* if need a SYNC, 
   (is needed if we plan to change speed, stereo ... during output)
*/
#ifdef __STDC__
int sync_dsp(void)
#else /* __STDC__ */
inline int sync_dsp(void)
#endif /* __STDC__ */
{
  if (ioctl (audio, SNDCTL_DSP_SYNC, NULL) < 0) 
    {
      perror(AUDIO);
      return (0);
    }
  return(1);
}

/* setting the speed for output */
int set_dsp_speed (int dsp_speed)
{
  if (IOCTL(audio, SNDCTL_DSP_SPEED, dsp_speed) < 0) 
    {
      fprintf (stderr, "unable to set audio speed\n");
      perror (AUDIO);
      return(0);
    }
  return(1);
}

/* if to_mono: 
   compress 8 bit stereo data 2:1, needed if we want play 'one track';
   this is usefull, if you habe SB 1.0 - 2.0 (I have 1.0) and want
   hear the sample (in Mono)
   if to_8:
   compress 16 bit to 8 by using hi-byte; wave-files use signed words,
   so we need to convert it in "unsigned" sample (0x80 is now zero)

   WARNING: this procedure can't compress 16 bit stereo to 16 bit mono,
            because if you have a 16 (or 12) bit card you should have
            stereo (or I'm wrong ?? )
   */
#ifdef __STDC__
u_long 
one_channel(u_char *buf, u_long l, char to_mono, char to_8)
#else /* __STDC__ */
inline u_long one_channel(char *buf, u_long l, char to_mono, char to_8)
#endif /* __STDC__ */
{
  u_char *w  = buf;
  u_char *w2 = buf;
  char ofs = 0;
  u_long incr = 0; 
  u_long c, ret;

  if (to_mono) 
    ++incr;
  if (to_8) 
    {
      ++incr; 
      ++w2; 
      ofs = 128; 
    }
  ret = c = l >> incr;
  incr = incr << 1;
  
  while (c--) 
    {
      *w++ = *w2 + ofs; 
      w2 += incr;
    }
  return ret;
}

int play (char *name)
{
  int retour;

  audio = open (AUDIO, O_WRONLY, 0);
  if (audio == -1) 
    {
      perror (AUDIO);
      return (0);
    }
  
  IOCTL(audio, SNDCTL_DSP_GETBLKSIZE, abuf_size);
  if (abuf_size < 4096 || abuf_size > 65536) 
    {
      if (abuf_size == -1)
	perror (AUDIO);
      else
	fprintf (stderr, "Invalid audio buffers size %d\n", abuf_size);
      return(0);
    }
  
  zbuf_size = 256;
  if ( (audiobuf = (u_char *)malloc (abuf_size)) == NULL ||
       (zerobuf  = (u_char *)malloc (zbuf_size)) == NULL )  
    {
      fprintf (stderr, "unable to allocate input/output buffer\n");
      return(0);
    }
  memset ((char *)zerobuf, 128, zbuf_size);
  
  retour = r_play (name);
  
  close (audio);
  return retour;
}

/* playing/recording raw data, this proc handels WAVE files and
   recording .VOCs (as one block) */ 
int recplay (int fd, int loaded, u_long count, int rtype, char *name)
{
  int l, real_l;
  u_long c;
  char one_chn = 0;
  char to_8 = 0;
  int tmps;

  if (!sync_dsp())
    return(0);

  if (!quiet_mode) 
    {
      if (samplesize != 8)
	fprintf(stderr, "%d bit, ", samplesize);
      fprintf (stderr, "Speed %d Hz ", dsp_speed);
      fprintf (stderr, "%s ...\n", dsp_stereo ? "Stereo" : "Mono");
    }

  tmps = samplesize;

  IOCTL(audio, SNDCTL_DSP_SAMPLESIZE, tmps);
  if (tmps != samplesize) 
    {
      fprintf (stderr, "unable to set %d bit sample size", samplesize);
      if (samplesize == 16) 
	{
	  samplesize = 8;
	  IOCTL(audio, SNDCTL_DSP_SAMPLESIZE, samplesize);
	  if (samplesize != 8) 
	    {
	      fprintf(stderr, "unable to set 8 bit sample size!\n");
	      return(0);
	    }
	  fprintf (stderr, "; playing 8 bit\n");
	  to_8 = 1;
	}
      else 
	{
	  fprintf (stderr, "\n");
	  return(0);
	}
    }
#ifdef SOUND_VERSION
  if (ioctl (audio, SNDCTL_DSP_STEREO, &dsp_stereo) < 0) 
    { 
#else  
  if (dsp_stereo != ioctl (audio, SNDCTL_DSP_STEREO, dsp_stereo) ) 
    { 
#endif
      fprintf (stderr, "can't play in Stereo; playing only one channel\n");
      dsp_stereo = MODE_MONO;
      one_chn = 1;
    }

  if (!set_dsp_speed (dsp_speed))
    return(0);
  
  while (count) 
    {
      c = count;
      
      if (c > abuf_size)
	c = abuf_size;
      
      if ((l = read (fd, (char *)audiobuf + loaded, c - loaded)) > 0) 
	{
	  l += loaded; loaded = 0;	/* correct the count; ugly but ... */
	  real_l = (one_chn || to_8) ? one_channel(audiobuf, l, one_chn, to_8) : l;
	  if (write (audio, (char *)audiobuf, real_l) != real_l) 
	    {
	      perror (AUDIO);
	      return(0);
	    }
	  count -= l;
	}
      else 
	{
	  if (l == -1) 
	    { 
	      perror (name);
	      return(0);
	    }
	  count = 0;	/* Stop */
	}
    }			/* while (count) */
  return(1);
}

/*
  let's play or record it (record_type says VOC/WAVE/raw)
*/
int r_play(char *name)
{
  int fd;
  int retour = 0;

  fprintf(stderr, "Playing %s : ", name);

  if ((fd = open (name, O_RDONLY, 0)) == -1) 
    {
      perror (name);
      return(0);
    }
  
  /* read bytes for WAVE-header */
  if (read (fd, (char *)audiobuf, sizeof(WaveHeader)) != sizeof(WaveHeader))
    perror ("r_play() read error");
  
  if (test_wavefile (audiobuf) >= 0)
    {
      retour = recplay (fd, 0, count, WAVE_FMT, name);
    }
  
  if (fd != 0)
    close(fd);

  return(retour);
}

