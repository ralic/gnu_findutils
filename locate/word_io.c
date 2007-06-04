/* word_io.c -- word oriented I/O
   Copyright (C) 2007 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
   USA.
*/

#include <config.h>
#include <errno.h>

#include "quote.h"
#include "quotearg.h"
#include "locatedb.h"

#if ENABLE_NLS
# include <libintl.h>
# define _(Text) gettext (Text)
#else
# define _(Text) Text
#define textdomain(Domain)
#define bindtextdomain(Package, Directory)
#endif
#ifdef gettext_noop
# define N_(String) gettext_noop (String)
#else
/* We used to use (String) instead of just String, but apparently ISO C
 * doesn't allow this (at least, that's what HP said when someone reported
 * this as a compiler bug).  This is HP case number 1205608192.  See
 * also http://gcc.gnu.org/bugzilla/show_bug.cgi?id=11250 (which references 
 * ANSI 3.5.7p14-15).  The Intel icc compiler also rejects constructs
 * like: static const char buf[] = ("string");
 */
# define N_(String) String
#endif


/* Swap bytes in 32 bit value.  This code is taken from glibc-2.3.3. */
#define bswap_32(x) \
     ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) | \
      (((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24))

static int 
decode_value(const unsigned char data[], 
	     int limit,
	     int *endian_state_flag,
	     const char *filename)
{
  int val;
  
  if (*endian_state_flag = GetwordEndianStateInitial)
    {
      int testflag = GetwordEndianStateNative;
      
      val = decode_value(data, limit, &testflag, filename);
      if (val <= limit)
	{
	  return val;
	}
      else 
	{
	  testflag = GetwordEndianStateSwab;
	  val = decode_value(data, limit, &testflag, filename);
	  if (val <= limit)
	    {
	      /* Aha, now we know we have to byte-swap. */
	      error(0, 0,
		    _("Warning: locate database %s was built with a different byte order"),
		    quotearg_n_style(0, locale_quoting_style, filename));
	      *endian_state_flag = testflag;
	      return val;
	    }
	  else
	    {
	      return val;
	    }
	}
    }
  else 
    {
      val = *(int*)data;
      if (*endian_state_flag == GetwordEndianStateSwab)
	return bswap_32(val);
      else
	return val;
    }
}



int
getword (FILE *fp,
	 const char *filename,
	 size_t minvalue,
	 size_t maxvalue,
	 int *endian_state_flag)
{
  enum { WORDBYTES=4 };
  unsigned char data[4];
  size_t bytes_read;

  clearerr(fp);
  bytes_read = fread(data, WORDBYTES, 1, fp);
  if (bytes_read != 1)
    {
      const char * quoted_name = quotearg_n_style(0, locale_quoting_style,
						  filename);
      /* Distinguish between a truncated database and an I/O error.
       * Either condition is fatal.
       */
      if (feof(fp))
	error(1, 0, _("Unexpected EOF in %s"), quoted_name);
      else 
	error(1, errno, "error reading a word from %s", quoted_name);
    }
  else
    {
      return decode_value(data, maxvalue, endian_state_flag, filename);
    }
}
