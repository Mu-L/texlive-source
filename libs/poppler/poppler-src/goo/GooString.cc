//========================================================================
//
// GooString.cc
//
// Simple variable-length string type.
//
// Copyright 1996-2003 Glyph & Cog, LLC
//
//========================================================================

//========================================================================
//
// Modified under the Poppler project - http://poppler.freedesktop.org
//
// All changes made under the Poppler project to this file are licensed
// under GPL version 2 or later
//
// Copyright (C) 2006 Kristian Høgsberg <krh@redhat.com>
// Copyright (C) 2006 Krzysztof Kowalczyk <kkowalczyk@gmail.com>
// Copyright (C) 2007 Jeff Muizelaar <jeff@infidigm.net>
// Copyright (C) 2008-2011, 2016, 2017 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2011 Kenji Uno <ku@digitaldolphins.jp>
// Copyright (C) 2012, 2013 Fabio D'Urso <fabiodurso@hotmail.it>
// Copyright (C) 2012, 2017 Adrian Johnson <ajohnson@redneon.com>
// Copyright (C) 2012 Pino Toscano <pino@kde.org>
// Copyright (C) 2013 Jason Crain <jason@aquaticape.us>
// Copyright (C) 2015 William Bader <williambader@hotmail.com>
// Copyright (C) 2016 Jakub Alba <jakubalba@gmail.com>
// Copyright (C) 2018 Klarälvdalens Datakonsult AB, a KDAB Group company, <info@kdab.com>. Work sponsored by the LiMux project of the city of Munich
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#include <config.h>

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <math.h>
#include "gmem.h"
#include "GooString.h"

static const int MAXIMUM_DOUBLE_PREC = 16;

//------------------------------------------------------------------------

union GooStringFormatArg {
  int i;
  Guint ui;
  long l;
  Gulong ul;
  long long ll;
  unsigned long long ull;
  double f;
  char c;
  char *s;
  GooString *gs;
};

enum GooStringFormatType {
  fmtIntDecimal,
  fmtIntHex,
  fmtIntHexUpper,
  fmtIntOctal,
  fmtIntBinary,
  fmtUIntDecimal,
  fmtUIntHex,
  fmtUIntHexUpper,
  fmtUIntOctal,
  fmtUIntBinary,
  fmtLongDecimal,
  fmtLongHex,
  fmtLongHexUpper,
  fmtLongOctal,
  fmtLongBinary,
  fmtULongDecimal,
  fmtULongHex,
  fmtULongHexUpper,
  fmtULongOctal,
  fmtULongBinary,
  fmtLongLongDecimal,
  fmtLongLongHex,
  fmtLongLongHexUpper,
  fmtLongLongOctal,
  fmtLongLongBinary,
  fmtULongLongDecimal,
  fmtULongLongHex,
  fmtULongLongHexUpper,
  fmtULongLongOctal,
  fmtULongLongBinary,
  fmtDouble,
  fmtDoubleTrimSmallAware,
  fmtDoubleTrim,
  fmtChar,
  fmtString,
  fmtGooString,
  fmtSpace
};

static const char *formatStrings[] = {
  "d", "x", "X", "o", "b", "ud", "ux", "uX", "uo", "ub",
  "ld", "lx", "lX", "lo", "lb", "uld", "ulx", "ulX", "ulo", "ulb",
  "lld", "llx", "llX", "llo", "llb",
  "ulld", "ullx", "ullX", "ullo", "ullb",
  "f", "gs", "g",
  "c",
  "s",
  "t",
  "w",
  nullptr
};

//------------------------------------------------------------------------

int inline GooString::roundedSize(int len) {
  int delta;
  if (len <= STR_STATIC_SIZE-1)
      return STR_STATIC_SIZE;
  delta = len < 256 ? 7 : 255;
  return ((len + 1) + delta) & ~delta;
}

// Make sure that the buffer is big enough to contain <newLength> characters
// plus terminating 0.
// We assume that if this is being called from the constructor, <s> was set
// to NULL and <length> was set to 0 to indicate unused string before calling us.
void inline GooString::resize(int newLength) {
  char *s1 = s;

  if (!s || (roundedSize(length) != roundedSize(newLength))) {
    // requires re-allocating data for string
    if (newLength < STR_STATIC_SIZE) {
      s1 = sStatic;
    } else {
      // allocate a rounded amount
      if (s == sStatic)
	s1 = (char*)gmalloc(roundedSize(newLength));
      else
	s1 = (char*)grealloc(s, roundedSize(newLength));
    }
    if (s == sStatic || s1 == sStatic) {
      // copy the minimum, we only need to if are moving to or
      // from sStatic.
      // assert(s != s1) the roundedSize condition ensures this
      if (newLength < length) {
	memcpy(s1, s, newLength);
      } else if (length > 0) {
	memcpy(s1, s, length);
      }
      if (s != sStatic)
	gfree(s);
    }

  }

  s = s1;
  length = newLength;
  s[length] = '\0';
}

GooString* GooString::Set(const char *newStr, int newLen)
{
    if (!newStr) {
        clear();
        return this;
    }

    if (newLen == CALC_STRING_LEN) {
        newLen = strlen(newStr);
    } else {
        assert(newLen >= 0);
    }

    resize(newLen);
    memmove(s, newStr, newLen);

    return this;
}

GooString::GooString() {
  s = nullptr;
  length = 0;
  Set(nullptr);

#if __cplusplus >= 201103L
  static_assert(sizeof(GooString) == GooString::STR_FINAL_SIZE, "You should check memory alignment or STR_STATIC_SIZE calculation.");
#endif
}

GooString::GooString(const char *sA) {
  s = nullptr;
  length = 0;
  Set(sA, CALC_STRING_LEN);
}

GooString::GooString(const char *sA, int lengthA) {
  s = nullptr;
  length = 0;
  Set(sA, lengthA);
}

GooString::GooString(const GooString *str, int idx, int lengthA) {
  s = nullptr;
  length = 0;
  assert(idx + lengthA <= str->length);
  Set(str->getCString() + idx, lengthA);
}

GooString::GooString(const GooString *str) {
  s = nullptr;
  length = 0;
  Set(str->getCString(), str->length);
}

GooString::GooString(GooString *str1, GooString *str2) {
  s = nullptr;
  length = 0;
  resize(str1->length + str2->length);
  memcpy(s, str1->getCString(), str1->length);
  memcpy(s + str1->length, str2->getCString(), str2->length);
}

GooString *GooString::fromInt(int x) {
  char buf[24]; // enough space for 64-bit ints plus a little extra
  char *p;
  int len;
  formatInt(x, buf, sizeof(buf), gFalse, 0, 10, &p, &len);
  return new GooString(p, len);
}

GooString *GooString::format(const char *fmt, ...) {
  va_list argList;
  GooString *s;

  s = new GooString();
  va_start(argList, fmt);
  s->appendfv(fmt, argList);
  va_end(argList);
  return s;
}

GooString *GooString::formatv(const char *fmt, va_list argList) {
  GooString *s;

  s = new GooString();
  s->appendfv(fmt, argList);
  return s;
}

GooString::~GooString() {
  if (s != sStatic)
    gfree(s);
}

GooString *GooString::clear() {
  resize(0);
  return this;
}

GooString *GooString::append(char c) {
  return append((const char*)&c, 1);
}

GooString *GooString::append(const GooString *str) {
  return append(str->getCString(), str->getLength());
}

GooString *GooString::append(const char *str, int lengthA) {
  int prevLen = length;
  if (CALC_STRING_LEN == lengthA)
    lengthA = strlen(str);
  resize(length + lengthA);
  memcpy(s + prevLen, str, lengthA);
  return this;
}

GooString *GooString::appendf(const char *fmt, ...) {
  va_list argList;

  va_start(argList, fmt);
  appendfv(fmt, argList);
  va_end(argList);
  return this;
}

GooString *GooString::appendfv(const char *fmt, va_list argList) {
  GooStringFormatArg *args;
  int argsLen, argsSize;
  GooStringFormatArg arg;
  int idx, width, prec;
  GBool reverseAlign, zeroFill;
  GooStringFormatType ft;
  char buf[65];
  int len, i;
  const char *p0, *p1;
  char *str;
  GooStringFormatArg argsBuf[ 8 ];

  argsLen = 0;
  argsSize = sizeof(argsBuf) / sizeof(argsBuf[0]);
  args = argsBuf;

  p0 = fmt;
  while (*p0) {
    if (*p0 == '{') {
      ++p0;
      if (*p0 == '{') {
	++p0;
	append('{');
      } else {

	// parse the format string
	if (!(*p0 >= '0' && *p0 <= '9')) {
	  break;
	}
	idx = *p0 - '0';
	for (++p0; *p0 >= '0' && *p0 <= '9'; ++p0) {
	  idx = 10 * idx + (*p0 - '0');
	}
	if (*p0 != ':') {
	  break;
	}
	++p0;
	if (*p0 == '-') {
	  reverseAlign = gTrue;
	  ++p0;
	} else {
	  reverseAlign = gFalse;
	}
	width = 0;
	zeroFill = *p0 == '0';
	for (; *p0 >= '0' && *p0 <= '9'; ++p0) {
	  width = 10 * width + (*p0 - '0');
	}
	if (width < 0) {
	  width = 0;
	}
	if (*p0 == '.') {
	  ++p0;
	  prec = 0;
	  for (; *p0 >= '0' && *p0 <= '9'; ++p0) {
	    prec = 10 * prec + (*p0 - '0');
	  }
	} else {
	  prec = 0;
	}
	for (ft = (GooStringFormatType)0;
	     formatStrings[ft];
	     ft = (GooStringFormatType)(ft + 1)) {
	  if (!strncmp(p0, formatStrings[ft], strlen(formatStrings[ft]))) {
	    break;
	  }
	}
	if (!formatStrings[ft]) {
	  break;
	}
	p0 += strlen(formatStrings[ft]);
	if (*p0 != '}') {
	  break;
	}
	++p0;

	// fetch the argument
	if (idx > argsLen) {
	  break;
	}
	if (idx == argsLen) {
	  if (argsLen == argsSize) {
	    argsSize *= 2;
	    if (args == argsBuf) {
	      args = (GooStringFormatArg *)gmallocn(argsSize, sizeof(GooStringFormatArg));
	      memcpy(args, argsBuf, argsLen * sizeof(GooStringFormatArg));
	    } else {
	      args = (GooStringFormatArg *)greallocn(args, argsSize,
						 sizeof(GooStringFormatArg));
	    }
	  }
	  switch (ft) {
	  case fmtIntDecimal:
	  case fmtIntHex:
	  case fmtIntHexUpper:
	  case fmtIntOctal:
	  case fmtIntBinary:
	  case fmtSpace:
	    args[argsLen].i = va_arg(argList, int);
	    break;
	  case fmtUIntDecimal:
	  case fmtUIntHex:
	  case fmtUIntHexUpper:
	  case fmtUIntOctal:
	  case fmtUIntBinary:
	    args[argsLen].ui = va_arg(argList, Guint);
	    break;
	  case fmtLongDecimal:
	  case fmtLongHex:
	  case fmtLongHexUpper:
	  case fmtLongOctal:
	  case fmtLongBinary:
	    args[argsLen].l = va_arg(argList, long);
	    break;
	  case fmtULongDecimal:
	  case fmtULongHex:
	  case fmtULongHexUpper:
	  case fmtULongOctal:
	  case fmtULongBinary:
	    args[argsLen].ul = va_arg(argList, Gulong);
	    break;
	  case fmtLongLongDecimal:
	  case fmtLongLongHex:
	  case fmtLongLongHexUpper:
	  case fmtLongLongOctal:
	  case fmtLongLongBinary:
	    args[argsLen].ll = va_arg(argList, long long);
	    break;
	  case fmtULongLongDecimal:
	  case fmtULongLongHex:
	  case fmtULongLongHexUpper:
	  case fmtULongLongOctal:
	  case fmtULongLongBinary:
	    args[argsLen].ull = va_arg(argList, unsigned long long);
	    break;
	  case fmtDouble:
	  case fmtDoubleTrim:
	  case fmtDoubleTrimSmallAware:
	    args[argsLen].f = va_arg(argList, double);
	    break;
	  case fmtChar:
	    args[argsLen].c = (char)va_arg(argList, int);
	    break;
	  case fmtString:
	    args[argsLen].s = va_arg(argList, char *);
	    break;
	  case fmtGooString:
	    args[argsLen].gs = va_arg(argList, GooString *);
	    break;
	  }
	  ++argsLen;
	}

	// format the argument
	arg = args[idx];
	switch (ft) {
	case fmtIntDecimal:
	  formatInt(arg.i, buf, sizeof(buf), zeroFill, width, 10, &str, &len);
	  break;
	case fmtIntHex:
	  formatInt(arg.i, buf, sizeof(buf), zeroFill, width, 16, &str, &len);
	  break;
	case fmtIntHexUpper:
	  formatInt(arg.i, buf, sizeof(buf), zeroFill, width, 16, &str, &len,
		    gTrue);
	  break;
	case fmtIntOctal:
	  formatInt(arg.i, buf, sizeof(buf), zeroFill, width, 8, &str, &len);
	  break;
	case fmtIntBinary:
	  formatInt(arg.i, buf, sizeof(buf), zeroFill, width, 2, &str, &len);
	  break;
	case fmtUIntDecimal:
	  formatUInt(arg.ui, buf, sizeof(buf), zeroFill, width, 10,
		     &str, &len);
	  break;
	case fmtUIntHex:
	  formatUInt(arg.ui, buf, sizeof(buf), zeroFill, width, 16,
		     &str, &len);
	  break;
	case fmtUIntHexUpper:
	  formatUInt(arg.ui, buf, sizeof(buf), zeroFill, width, 16,
		     &str, &len, gTrue);
	  break;
	case fmtUIntOctal:
	  formatUInt(arg.ui, buf, sizeof(buf), zeroFill, width, 8, &str, &len);
	  break;
	case fmtUIntBinary:
	  formatUInt(arg.ui, buf, sizeof(buf), zeroFill, width, 2, &str, &len);
	  break;
	case fmtLongDecimal:
	  formatInt(arg.l, buf, sizeof(buf), zeroFill, width, 10, &str, &len);
	  break;
	case fmtLongHex:
	  formatInt(arg.l, buf, sizeof(buf), zeroFill, width, 16, &str, &len);
	  break;
	case fmtLongHexUpper:
	  formatInt(arg.l, buf, sizeof(buf), zeroFill, width, 16, &str, &len,
		    gTrue);
	  break;
	case fmtLongOctal:
	  formatInt(arg.l, buf, sizeof(buf), zeroFill, width, 8, &str, &len);
	  break;
	case fmtLongBinary:
	  formatInt(arg.l, buf, sizeof(buf), zeroFill, width, 2, &str, &len);
	  break;
	case fmtULongDecimal:
	  formatUInt(arg.ul, buf, sizeof(buf), zeroFill, width, 10,
		     &str, &len);
	  break;
	case fmtULongHex:
	  formatUInt(arg.ul, buf, sizeof(buf), zeroFill, width, 16,
		     &str, &len);
	  break;
	case fmtULongHexUpper:
	  formatUInt(arg.ul, buf, sizeof(buf), zeroFill, width, 16,
		     &str, &len, gTrue);
	  break;
	case fmtULongOctal:
	  formatUInt(arg.ul, buf, sizeof(buf), zeroFill, width, 8, &str, &len);
	  break;
	case fmtULongBinary:
	  formatUInt(arg.ul, buf, sizeof(buf), zeroFill, width, 2, &str, &len);
	  break;
	case fmtLongLongDecimal:
	  formatInt(arg.ll, buf, sizeof(buf), zeroFill, width, 10, &str, &len);
	  break;
	case fmtLongLongHex:
	  formatInt(arg.ll, buf, sizeof(buf), zeroFill, width, 16, &str, &len);
	  break;
	case fmtLongLongHexUpper:
	  formatInt(arg.ll, buf, sizeof(buf), zeroFill, width, 16, &str, &len,
		    gTrue);
	  break;
	case fmtLongLongOctal:
	  formatInt(arg.ll, buf, sizeof(buf), zeroFill, width, 8, &str, &len);
	  break;
	case fmtLongLongBinary:
	  formatInt(arg.ll, buf, sizeof(buf), zeroFill, width, 2, &str, &len);
	  break;
	case fmtULongLongDecimal:
	  formatUInt(arg.ull, buf, sizeof(buf), zeroFill, width, 10,
		     &str, &len);
	  break;
	case fmtULongLongHex:
	  formatUInt(arg.ull, buf, sizeof(buf), zeroFill, width, 16,
		     &str, &len);
	  break;
	case fmtULongLongHexUpper:
	  formatUInt(arg.ull, buf, sizeof(buf), zeroFill, width, 16,
		     &str, &len, gTrue);
	  break;
	case fmtULongLongOctal:
	  formatUInt(arg.ull, buf, sizeof(buf), zeroFill, width, 8,
		     &str, &len);
	  break;
	case fmtULongLongBinary:
	  formatUInt(arg.ull, buf, sizeof(buf), zeroFill, width, 2,
		     &str, &len);
	  break;
	case fmtDouble:
	  formatDouble(arg.f, buf, sizeof(buf), prec, gFalse, &str, &len);
	  break;
	case fmtDoubleTrim:
	  formatDouble(arg.f, buf, sizeof(buf), prec, gTrue, &str, &len);
	  break;
	case fmtDoubleTrimSmallAware:
	  formatDoubleSmallAware(arg.f, buf, sizeof(buf), prec, gTrue, &str, &len);
	  break;
	case fmtChar:
	  buf[0] = arg.c;
	  str = buf;
	  len = 1;
	  reverseAlign = !reverseAlign;
	  break;
	case fmtString:
	  str = arg.s;
	  len = strlen(str);
	  reverseAlign = !reverseAlign;
	  break;
	case fmtGooString:
	  str = arg.gs->getCString();
	  len = arg.gs->getLength();
	  reverseAlign = !reverseAlign;
	  break;
	case fmtSpace:
	  str = buf;
	  len = 0;
	  width = arg.i;
	  break;
	}

	// append the formatted arg, handling width and alignment
	if (!reverseAlign && len < width) {
	  for (i = len; i < width; ++i) {
	    append(' ');
	  }
	}
	append(str, len);
	if (reverseAlign && len < width) {
	  for (i = len; i < width; ++i) {
	    append(' ');
	  }
	}
      }

    } else if (*p0 == '}') {
      ++p0;
      if (*p0 == '}') {
	++p0;
      }
      append('}');
      
    } else {
      for (p1 = p0 + 1; *p1 && *p1 != '{' && *p1 != '}'; ++p1) ;
      append(p0, p1 - p0);
      p0 = p1;
    }
  }

  if (args != argsBuf) {
    gfree(args);
  }

  return this;
}

static const char lowerCaseDigits[17] = "0123456789abcdef";
static const char upperCaseDigits[17] = "0123456789ABCDEF";

void GooString::formatInt(long long x, char *buf, int bufSize,
                          GBool zeroFill, int width, int base,
                          char **p, int *len, GBool upperCase) {
  const char *vals = upperCase ? upperCaseDigits : lowerCaseDigits;
  GBool neg;
  int start, i, j;
  unsigned long long abs_x;

  i = bufSize;
  if ((neg = x < 0)) {
    abs_x = -x;
  } else {
    abs_x = x;
  }
  start = neg ? 1 : 0;
  if (abs_x == 0) {
    buf[--i] = '0';
  } else {
    while (i > start && abs_x) {
      buf[--i] = vals[abs_x % base];
      abs_x /= base;
    }
  }
  if (zeroFill) {
    for (j = bufSize - i; i > start && j < width - start; ++j) {
      buf[--i] = '0';
    }
  }
  if (neg) {
    buf[--i] = '-';
  }
  *p = buf + i;
  *len = bufSize - i;
}

void GooString::formatUInt(unsigned long long x, char *buf, int bufSize,
                           GBool zeroFill, int width, int base,
                           char **p, int *len, GBool upperCase) {
  const char *vals = upperCase ? upperCaseDigits : lowerCaseDigits;
  int i, j;

  i = bufSize;
  if (x == 0) {
    buf[--i] = '0';
  } else {
    while (i > 0 && x) {
      buf[--i] = vals[x % base];
      x /= base;
    }
  }
  if (zeroFill) {
    for (j = bufSize - i; i > 0 && j < width; ++j) {
      buf[--i] = '0';
    }
  }
  *p = buf + i;
  *len = bufSize - i;
}

void GooString::formatDouble(double x, char *buf, int bufSize, int prec,
			   GBool trim, char **p, int *len) {
  GBool neg, started;
  double x2;
  int d, i, j;

  if ((neg = x < 0)) {
    x = -x;
  }
  x = floor(x * pow(10.0, prec) + 0.5);
  i = bufSize;
  started = !trim;
  for (j = 0; j < prec && i > 1; ++j) {
    x2 = floor(0.1 * (x + 0.5));
    d = (int)floor(x - 10 * x2 + 0.5);
    if (started || d != 0) {
      buf[--i] = '0' + d;
      started = gTrue;
    }
    x = x2;
  }
  if (i > 1 && started) {
    buf[--i] = '.';
  }
  if (i > 1) {
    do {
      x2 = floor(0.1 * (x + 0.5));
      d = (int)floor(x - 10 * x2 + 0.5);
      buf[--i] = '0' + d;
      x = x2;
    } while (i > 1 && x);
  }
  if (neg) {
    buf[--i] = '-';
  }
  *p = buf + i;
  *len = bufSize - i;
}

void GooString::formatDoubleSmallAware(double x, char *buf, int bufSize, int prec,
				      GBool trim, char **p, int *len)
{
  double absX = fabs(x);
  if (absX >= 0.1) {
    formatDouble(x, buf, bufSize, prec, trim, p, len);
  } else {
    while (absX < 0.1 && prec < MAXIMUM_DOUBLE_PREC)
    {
      absX = absX * 10;
      prec++;
    }
    formatDouble(x, buf, bufSize, prec, trim, p, len);
  }
}

GooString *GooString::insert(int i, char c) {
  return insert(i, (const char*)&c, 1);
}

GooString *GooString::insert(int i, const GooString *str) {
  return insert(i, str->getCString(), str->getLength());
}

GooString *GooString::insert(int i, const char *str, int lengthA) {
  int prevLen = length;
  if (CALC_STRING_LEN == lengthA)
    lengthA = strlen(str);

  resize(length + lengthA);
  memmove(s+i+lengthA, s+i, prevLen-i);
  memcpy(s+i, str, lengthA);
  return this;
}

GooString *GooString::del(int i, int n) {
  int j;

  if (i >= 0 && n > 0 && i + n > 0) {
    if (i + n > length) {
      n = length - i;
    }
    for (j = i; j <= length - n; ++j) {
      s[j] = s[j + n];
    }
    resize(length - n);
  }
  return this;
}

GooString *GooString::upperCase() {
  int i;

  for (i = 0; i < length; ++i) {
    if (islower(s[i]))
      s[i] = toupper(s[i]);
  }
  return this;
}

GooString *GooString::lowerCase() {
  int i;

  for (i = 0; i < length; ++i) {
    if (isupper(s[i]))
      s[i] = tolower(s[i]);
  }
  return this;
}

int GooString::cmp(const GooString *str) const {
  int n1, n2, i, x;
  char *p1, *p2;

  n1 = length;
  n2 = str->length;
  for (i = 0, p1 = s, p2 = str->s; i < n1 && i < n2; ++i, ++p1, ++p2) {
    x = *p1 - *p2;
    if (x != 0) {
      return x;
    }
  }
  return n1 - n2;
}

int GooString::cmpN(GooString *str, int n) const {
  int n1, n2, i, x;
  char *p1, *p2;

  n1 = length;
  n2 = str->length;
  for (i = 0, p1 = s, p2 = str->s;
       i < n1 && i < n2 && i < n;
       ++i, ++p1, ++p2) {
    x = *p1 - *p2;
    if (x != 0) {
      return x;
    }
  }
  if (i == n) {
    return 0;
  }
  return n1 - n2;
}

int GooString::cmp(const char *sA) const {
  int n1, i, x;
  const char *p1, *p2;

  n1 = length;
  for (i = 0, p1 = s, p2 = sA; i < n1 && *p2; ++i, ++p1, ++p2) {
    x = *p1 - *p2;
    if (x != 0) {
      return x;
    }
  }
  if (i < n1) {
    return 1;
  }
  if (*p2) {
    return -1;
  }
  return 0;
}

int GooString::cmpN(const char *sA, int n) const {
  int n1, i, x;
  const char *p1, *p2;

  n1 = length;
  for (i = 0, p1 = s, p2 = sA; i < n1 && *p2 && i < n; ++i, ++p1, ++p2) {
    x = *p1 - *p2;
    if (x != 0) {
      return x;
    }
  }
  if (i == n) {
    return 0;
  }
  if (i < n1) {
    return 1;
  }
  if (*p2) {
    return -1;
  }
  return 0;
}

GBool GooString::endsWith(const char *suffix) const {
  int suffixLen = strlen(suffix);

  if (length < suffixLen)
    return gFalse;

  return strcmp(s + length - suffixLen, suffix) == 0;
}

GBool GooString::hasUnicodeMarker(void) const
{
  return length > 1 && (s[0] & 0xff) == 0xfe && (s[1] & 0xff) == 0xff;
}

void GooString::prependUnicodeMarker()
{
    insert(0, (char)0xff);
    insert(0, (char)0xfe);
}

GooString *GooString::sanitizedName(GBool psmode) const
{
  GooString *name;
  char buf[8];
  int i;
  char c;

  name = new GooString();

  if (psmode)
  {
    // ghostscript chokes on names that begin with out-of-limits
    // numbers, e.g., 1e4foo is handled correctly (as a name), but
    // 1e999foo generates a limitcheck error
    c = getChar(0);
    if (c >= '0' && c <= '9') {
      name->append('f');
    }
  }

  for (i = 0; i < getLength(); ++i) {
    c = getChar(i);
    if (c <= (char)0x20 || c >= (char)0x7f ||
	c == ' ' ||
	c == '(' || c == ')' || c == '<' || c == '>' ||
	c == '[' || c == ']' || c == '{' || c == '}' ||
	c == '/' || c == '%' || c == '#') {
      sprintf(buf, "#%02x", c & 0xff);
      name->append(buf);
    } else {
      name->append(c);
    }
  }
  return name;
}