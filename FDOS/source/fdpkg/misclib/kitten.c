/* Functions that emulate UNIX catgets */

/* Copyright (C) 1999,2000,2001 Jim Hall <jhall@freedos.org> */
/* Kitten version 2003 by Tom Ehlert, heavily modified by Eric Auer 2003 */

/*
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef NO_KITTEN

#include <stdio.h>			/* sprintf */
#ifndef _MICROC_
#include <stdlib.h>			/* getenv  */
#include <string.h>			/* strchr */
#include <dos.h>
#ifndef __PACIFIC__
#include <fcntl.h>
#else
#define O_RDONLY 0
#define O_TEXT 0
#endif
#else
#include <intr.h>
#include <file.h>
#define O_RDONLY READONLY
#define O_TEXT 0
#endif
/* assert we are running in small model */
/* else pointer below has to be done correctly */
/* char verify_small_pointers[sizeof(void*) == 2 ? 1 : -1]; */

#define __KITTEN_C 1

#include "kitten.h"

char catcontents[8192];

struct catstring{
  char key1;
  char key2;
  char *text;
};

/* Micro-C does not support typedef */
#define catstring_t struct catstring

catstring_t catpoints[128];


/* Local prototypes */

int catread (char *catfile);		/* Reads a catfile into the hash */
char *processEscChars(char *line);  /* Converts c escape sequences to chars */

int get_char(int file); /* not meant for external use    */
/* external use would cause consistency problems if */
/* value or related file of the file handle changes */

int mystrtoul(char *src, int base, int size);


/* Globals */

nl_catd _kitten_catalog = 0;	/* _kitten_catalog descriptor or 0 */
char catfile[128];		/* full path to _kitten_catalog */

char getlbuf[8192]; /* read buffer for better speed  */
char * getlp;       /* current point in buffer       */
int getlrem = -1;   /* remaining bytes in buffer     */
char lastcr = 0;    /* for 2byte CR LF sequences     */


#ifndef _MICROC_
#ifndef __DJGPP__

/* DOS handle based file usage */

#ifdef __WATCOMC__

#pragma aux dos_open = \
"mov ax, 0x3d00" \
"int 0x21" \
"jnc noerror" \
"mov ax, 0xffff" \
"noerror:" \
parm [dx] [ax] value [ax];

#pragma aux dos_read = \
"mov ah, 0x3f" \
"int 0x21" \
"jnc noerror" \
"xor ax, ax" \
"noerror:" \
parm [bx] [dx] [cx] value [ax];

#pragma aux dos_write = \
"mov ah, 0x40" \
"int 0x21" \
"jnc noerror" \
"mov ax, 0" \
"noerror:" \
parm [bx] [dx] [cx] value [ax];

#pragma aux dos_close = \
"mov ah, 0x3e" \
"int 0x21" \
parm [bx];

#else

int dos_open(char *filename, int mode)
{
#ifdef __WATCOMC__
	__asm {
		mov ax, 0x3D00
		mov dx, filename
		mov ax, mode
		int 0x21
		jnc noerror
		mov ax, 0xFFFF
		noerror:
		ret;
	}
#else
	union REGS r;
        struct SREGS s;
	if (mode);		/* mode ignored - readonly supported */
	r.h.ah = 0x3d;
	r.h.al = 0;		/* read mode only supoported now !! */
	r.x.dx = FP_OFF(filename);
        s.ds = FP_SEG(filename);
	intdosx(&r,&r,&s);
	return ( (r.x.cflag) ? -1 : (int)r.x.ax );
#endif
}

int dos_read(int file, void *ptr, unsigned count)
{
#ifdef __WATCOMC__
	__asm {
		mov ah, 0x3F
		mov bx, file
		mov dx, ptr
		mov cx, count
		int 0x21
		jnc noerror
		xor ax, ax
		noerror:
		ret
	}
#else
	union REGS r;
        struct SREGS s;
	r.h.ah = 0x3f;
	r.x.bx = file;
	r.x.cx = count;
	r.x.dx = FP_OFF(ptr);
        s.ds = FP_SEG(ptr);
	intdosx(&r,&r,&s);
	return ( (r.x.cflag) ? 0 : r.x.ax );
#endif
}

/*int dos_write(int file, void *ptr, unsigned count)
{
#ifdef __WATCOMC__
	__asm {
		mov ah, 0x40
		mov bx, file
		mov dx, ptr
		mov cx, count
		int 0x21
		jnc noerror
		mov ax, 0
		noerror:
		ret ax
	}
#else
	union REGS r;
	struct SREGS s;
	r.h.ah = 0x40;
	r.x.bx = file;
	r.x.cx = count;
	r.x.dx = FP_OFF(ptr);
        s.ds = FP_SEG(ptr);
	intdosx(&r,&r,&s);
	return ( (r.x.cflag) ? 0 : r.x.ax );
#endif
}*/

void dos_close(int file)
{
#ifdef __WATCOMC__
	__asm {
		mov ah, 0x3E
		mov bx, file
		int 0x21
	}
#else
	union REGS r;
	r.h.ah = 0x3e;
	r.x.bx = file;
	intdos(&r,&r);
#endif
}
#endif /*WATCOMC*/

#endif /*DJGPP*/
#endif /*Micro-C*/


/* Functions */

/**
 * On success, catgets() returns a pointer to an internal
 * buffer area containing the null-terminated message string.
 * On failure, catgets() returns the value 'message'.
 */

char * kittengets(int setnum, int msgnum, char * message)
{
/* 
 * In Micro-C, variables must be defined at the start of the
 * function and may not be immediately assigned a value
 */
#ifdef _MICROC_
  int i;
  i = 0;
#else
  int i = 0;
#endif
  
  while ((catpoints[i].key1 != setnum) || (catpoints[i].key2 != msgnum)) {
    if ((catpoints[i].text == NULL) || (i>127)) /* at EOF */
      return message;
    i++;
  }

  if (catpoints[i].text == NULL)
    return message;
  else
    return (catpoints[i].text);
}


/**
 * Initialize kitten for program (name).
 */

nl_catd kittenopen(char *name)
{
  /* catopen() returns a message _kitten_catalog descriptor  *
   * of type nl_catd on success.  On failure, it returns -1. */

  char catlang[3];			/* from LANG environment var. */
  char *nlsptr;				/* ptr to NLSPATH */
  char *lang;                           /* ptr to LANG */
  int i;
#ifdef _MICROC_
  char *tok;
  int toklen;
#endif
  
  /* Open the _kitten_catalog file */
  /* The value of `_kitten_catalog' will be set based on catread */

  if (_kitten_catalog) { /* Already one open */
    return (-1);
  }

  for (i=0; i<128; i++)
    catpoints[i].text = NULL;

  if (strchr (name, '\\')) {
    /* unusual case: 'name' is a filename */
    _kitten_catalog = catread (name);
    if (_kitten_catalog)
      return (_kitten_catalog);
  }

  /* If the message _kitten_catalog file name does not contain a directory *
   * separator, then we need to try to locate the message _kitten_catalog. */

  /* We will need the value of LANG, and may need a 2-letter abbrev of
     LANG later on, so get it now. */

  lang = getenv ("LANG");

  if (lang == NULL) {
      /* Return failure - we won't be able to locate the cat file */
      return (-1);
  }

  if ( ( strlen(lang) < 2 ) ||
       ( (strlen(lang) > 2) && (lang[2] != '-') ) ) {
      /* Return failure - we won't be able to locate the cat file */
      return (-1);
  }

  memcpy(catlang, lang, 2);
  /* we copy the full LANG value or the part before "-" if "-" found */
  catlang[2] = '\0';

  /* step through NLSPATH */

  nlsptr = getenv ("NLSPATH");

  if (nlsptr == NULL) {
      /* Return failure - we won't be able to locate the cat file */
      return (-1);
   }

  catfile[0] = '\0';

  while (nlsptr && nlsptr[0]) {
#ifdef _MICROC_
      tok = strchr(nlsptr, ';');
#else
      char *tok = strchr(nlsptr, ';');
      int toklen;
#endif

      if (tok == NULL)
        toklen = strlen(nlsptr); /* last segment */
      else
        toklen = tok - nlsptr; /* segment terminated by ';' */
      
      /* catfile = malloc(toklen+1+strlen(name)+1+strlen(lang)+1); */
      /* Try to find the _kitten_catalog file in each path from NLSPATH */

      if ((toklen+6+strlen(name)) > sizeof(catfile)) {
        return 0; /* overflow in NLSPATH, should never happen */
      }

      /* Rule #1: %NLSPATH%\%LANG%\cat */

        memcpy(catfile, nlsptr, toklen);
        strcpy(catfile+toklen,"\\");
        strcat(catfile,catlang);
        strcat(catfile,"\\");
        strcat(catfile,name);
        _kitten_catalog = catread (catfile);
        if (_kitten_catalog)
	  return (_kitten_catalog);

      /* Rule #2: %NLSPATH%\cat.%LANG% */

        /* memcpy(catfile, nlsptr, toklen); */
        strcpy(catfile+toklen,"\\");
        strcat(catfile,name);
        strcat(catfile,".");
        strcat(catfile,catlang);
        _kitten_catalog = catread (catfile);
        if (_kitten_catalog)
	  return (_kitten_catalog);

      /* Grab next tok for the next while iteration */

        nlsptr = tok;
        if (nlsptr) nlsptr++;
      
    } /* while tok */

  /* We could not find it.  Return failure. */

  return (-1);
}


/**
 * Load a message catalog into memory.
 */

int catread (char *catfile)
{
  int   file;				/* pointer to the catfile */
  int   i;
  char  *where;
  char  *tok;
#ifdef _MICROC_
  char *msg;
  char *key;
  int key1;
  int key2;
#endif

  /* Get the whole catfile into a buffer and parse it */
  
  file = open (catfile, O_RDONLY | O_TEXT);
  if (file < 0) 
      /* Cannot open the file.  Return failure */
      return 0;

  for (i=0; i<128; i++)
    catpoints[i].text = NULL;

  for (i=0; (unsigned int)i<sizeof(catcontents); i++)
    catcontents[i] = '\0';

  /* Read the file into memory */
  i = read (file, catcontents, sizeof(catcontents)-1);

  if ((i == sizeof(catcontents)-1) || (i < 1))
      return 0; /* file was too big or too small */

  where = catcontents;
  i = 0; /* catpoints entry */

  do {
#ifndef _MICROC_
    char *msg;
    char *key;
    int key1 = 0;
    int key2 = 0;
#else
    key1 = 0;
    key2 = 0;
#endif

    tok = strchr(where, '\n');

    if (tok == NULL) { /* done? */
      close(file);
      return 1; /* success */
    }

    tok[0] = '\0'; /* terminate here */ 
    tok--; /* guess: \r before \n */
    if (tok[0] != '\r')
      tok++; /* if not, go back */
    else {
      tok[0] = '\0'; /* terminate here already */
      tok++;
    }
    tok++; /* this is where the next line starts */

    if ( (where[0] >= '0') && (where[0] <= '9') &&
         ( (msg = strchr(where,':')) != NULL) ) {
      /* Skip everything which starts with # or with no digit */
      /* Entries look like "1.2:This is a message" */

      msg[0] = '\0'; /* remove : */
      msg++; /* go past the : */

      if ((key = strchr (where, '.')) != NULL)	{
        key[0] = '\0'; /* turn . into terminator */
        key++; /* go past the . */
        key1 = mystrtoul(where, 10, strlen(where));
        key2 = mystrtoul(key, 10, strlen(key));

        if ((key1 >= 0) && (key2 >= 0)) {
          catpoints[i].key1 = key1;
          catpoints[i].key2 = key2;
          catpoints[i].text = processEscChars(msg);
          if (catpoints[i].text == NULL) /* ESC parse error */
	    catpoints[i].text = msg;
          i++; /* next entry! */
        } /* valid keys */

      } /* . found */

    } /* : and digit found */

    where = tok; /* go to next line */

  } while (1);
#ifdef __PACIFIC__
  return 0;
#endif
}


void kittenclose (void)
{
  /* close a message _kitten_catalog */
  _kitten_catalog = 0;
}


/**
 * Parse a string that represents an unsigned integer.
 * Returns -1 if an error is found. The first size
 * chars of the string are parsed.
 */

int mystrtoul(char *src, int base, int size)
{
#ifdef _MICROC_
        int ret;
        int digit;
        int ch;
        ret = 0;
#else
	int ret = 0;
#endif
	
	for (; size > 0; size--)  {
#ifdef _MICROC_
          ch = *src;
#else
	  int digit;
	  int ch = *src;
#endif
          src++;

	  if (ch >= '0' && ch <= '9')
	    digit = ch - '0';
	  else if (ch >= 'A' && ch <= 'Z')
            digit = ch - 'A' + 10;
	  else if (ch >= 'a' && ch <= 'z')
            digit = ch - 'a' + 10;
	  else
	    return -1;

          if (digit >= base)
  	    return -1;
	  
	  ret = ret * base + digit;
	} /* for */

	return ret;
}		  	  	
	  	

/**
 * Process strings, converting \n, \t, \v, \b, \r, \f, \\,
 * \ddd, \xdd and \x0dd to actual chars.
 * (Note: \x is an extension to support hexadecimal)
 * This is used to allow the messages to use c escape sequences.
 * Modifies the line in-place (always same size or shorter).
 * Returns a pointer to input string.
 */

char *processEscChars(char *line)
{
  /* used when converting \xdd and \ddd (hex or octal) characters */
  char ch;
#ifdef _MICROC_
  char *src;
  char *dst;
  int chx;
  src = line;
  dst = line;
#else
  char *src = line;
  char *dst = line; /* possible as dst is shorter than src */
#endif

  if (line == NULL)
    return line;

  /* cycle through copying characters, except when a \ is encountered. */
  while (*src != '\0') {
    ch = *src;
    src++;
    
    if (ch == '\\') {
      ch = *src; /* what follows slash? */
      src++;

      switch (ch) {
        case '\\': /* a single slash */
	  *dst = '\\';
	  dst++;
	  break;
        case 'n': /* a newline (linefeed) */
	  *dst = '\n';
	  dst++;
	  break;
        case 'r': /* a carriage return */
	  *dst = '\r';
	  dst++;
	  break;
        case 't': /* a horizontal tab */
	  *dst = '\t';
	  dst++;
	  break;
        case 'v': /* a vertical tab */
          *dst = '\v';
          dst++;
          break;
        case 'b': /* a backspace */
          *dst = '\b';
          dst++;
          break;
        case 'a': /* alert */
          *dst = '\a';
          dst++;
          break;
        case 'f': /* formfeed */
          *dst = '\f';
          dst++;
          break;
        case 'x': /* extension supporting hex numbers \xdd or \x0dd */
          {
#ifdef _MICROC_
            chx = mystrtoul(src, 16, 2); /* get value */
#else
            int chx = mystrtoul(src, 16, 2); /* get value */
#endif
            if (chx >= 0) { /* store character */
	       *dst = chx;
	       dst++;
               src += 2;
            } else /* error so just store x (loose slash) */
            {
              *dst = *src;
              dst++;
            }
          }
          break;
        default: /* just store letter (loose slash) or handle octal */
	  {
#ifdef _MICROC_
            chx = mystrtoul(src, 8, 3); /* get value */
#else
            int chx = mystrtoul(src, 8, 3); /* get value */
#endif
            if (chx >= 0) {/* store character */
	       *dst = chx;
	       dst++;
               src += 3;
            } else
            {
               *dst = *src;
               dst++;
            }
	  }
	  break;
      } /* switch */
    } /* if backslash */
      else
    {
      *dst = ch;
      dst++;
    }
  } /* while */

  /* ensure '\0' terminated */
  *dst = '\0';

  return line;
}


int get_char(int file) {
#ifdef _MICROC_
  int rval;
  rval = -1;
#else
  int rval = -1;
#endif

  if (getlrem <= 0) { /* (re)init buffer */
    getlrem = read(file, getlbuf, sizeof(getlbuf));
    if (getlrem <= 0)
      return -1; /* fail: read error / EOF */
    getlp = getlbuf; /* init pointer */
  }

  if (getlrem > 0) { /* consume byte from buffer */
    rval = getlp[0];
    getlp++;
    getlrem--;
  }

  return rval;
}


/**
 * Read a line of text from file. You must call this with
 * a null buffer or null size to flush buffers when you are
 * done with a file before using it on the next file. Cannot
 * be used for 2 files at the same time.
 */

int get_line (int file, char *str, int size)
{
  int ch;
#ifdef _MICROC_
  int success;
  success = 0;
#else
  int success = 0;
#endif

  if ((size == 0) || (str == NULL)) { /* re-init get_line buffers */
    getlp = getlbuf;
    getlrem = -1;
    lastcr = 0;
    return 0;
  }

  str[0] = '\0';

  while ( (size > 0) && (success == 0) ) {
    ch = get_char (file);
    if (ch < 0)
      break; /* (can cause fail if no \n found yet) */

    if (ch == '\r')
      ch = get_char (file); /* ignore \r */

    str[0] = ch;

    if ( (ch == '\n') || (ch == '\r') ) { /* done? */
      str[0] = '\0';
      return 1; /* success */
    }

    str++;
    size--;

  } /* while */

  str[0] = '\0'; /* terminate buffer */

  return success;

}

#endif /*NO_KITTEN*/
