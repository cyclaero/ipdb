//  utils.c
//  ipdbtools
//
//  Created by Dr. Rolf Jansen on 2018-05-08.
//  Copyright © 2018 Dr. Rolf Jansen. All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without modification,
//  are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//  2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
//  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
//  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
//  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
//  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
//  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
//  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
//  OF THE POSSIBILITY OF SUCH DAMAGE.


#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "utils.h"


// String concat to dst with variable number of src/len pairs, whereby each len
// serves as the l parameter in strmlcpy(), i.e. strmlcpy(dst, src, ml, &len)
// m: Max. capacity of dst, including the final nul.
//    If m == 0, then the sum of the length of all src strings is returned in l - nothing is copied though.
// l: On entry, offset into dst or -1, when -1, the offset is the end of the initial string in dst
//    On exit, the length of the total concat, even if it would not fit into dst, maybe NULL.
// Returns the length of the resulting string in dst.
int strmlcat(char *dst, int m, int *l, ...)
{
   va_list     vl;
   int         k, n;
   const char *s;

   if (l && *l)
   {
      if (*l == -1)
         *l = strvlen(dst);
      n = k = *l;
   }
   else
      n = k = 0;

   va_start(vl, l);
   while (s = va_arg(vl, const char *))
   {
      k = va_arg(vl, int);
      if (n < m)
      {
         n += strmlcpy(&dst[n], s, m-n, &k);
         if (l) *l += k;
      }
      else
         if (l) *l += (k) ?: strvlen(s);
   }
   va_end(vl);

   return n;
}


// hex <-> bin conversions

int hex2val(char hex)
{
   switch (hex)
   {
      case '0' ... '9':
         return hex - '0';

      case 'A' ... 'Z':
         return 10 + hex - 'A';

      case 'a' ... 'z':
         return 10 + hex - 'a';

      default:
         return 0;
   }
}


void conv2Hex(uchar *bin, uchar *hex, uint16_t n)
{
   uchar c;
   int   i, j;

   for (i = 0, j = 0; i < n; i++)
   {
      c = (bin[i] >> 4) & 0xF;
      hex[j++] = (c <= 9) ? (c + '0') : (c + 'a' - 10);

      c = bin[i] & 0xF;
      hex[j++] = (c <= 9) ? (c + '0') : (c + 'a' - 10);
   }

   hex[j] = '\0';
}


void vonc2Hex(uchar *bin, uchar *hex, uint16_t n)
{
   uchar c;
   int   i, j;

   for (i = n - 1, j = 0; i >= 0; i--)
   {
      c = (bin[i] >> 4) & 0xF;
      hex[j++] = (c <= 9) ? (c + '0') : (c + 'a' - 10);

      c = bin[i] & 0xF;
      hex[j++] = (c <= 9) ? (c + '0') : (c + 'a' - 10);
   }

   hex[j] = '\0';
}


#pragma mark ••• Base64 Encoding/Decoding •••
// http://tools.ietf.org/html/rfc4648#section-4

static const uchar enc64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static inline uint encode3(uchar in[3])
{
   uint   result;
   uchar *r = (uchar *)&result;

   r[0] = enc64[in[0] >> 2];
   r[1] = enc64[((in[0] & 0x03) << 4) | ((in[1] & 0xF0) >> 4)];
   r[2] = enc64[((in[1] & 0x0F) << 2) | ((in[2] & 0xC0) >> 6)];
   r[3] = enc64[in[2] & 0x3F];

   return result;
}

static inline uint encode2(uchar in[2])
{
   uint   result;
   uchar *r = (uchar *)&result;

   r[0] = enc64[in[0] >> 2];
   r[1] = enc64[((in[0] & 0x03) << 4) | ((in[1] & 0xF0) >> 4)];
   r[2] = enc64[(in[1] & 0x0F) << 2];
   r[3] = (uchar)'=';

   return result;
}

static inline uint encode1(uchar in)
{
   uint   result;
   uchar *r = (uchar *)&result;

   r[0] = enc64[in >> 2];
   r[1] = enc64[(in & 0x03) << 4];
   r[2] = (uchar)'=';
   r[3] = (uchar)'=';

   return result;
}

char *base64Encode(uint head, char *data, size_t *length)
{
   uint   i, j;
   uint   tail    = *length % 3;                           // tail when grouping 3 bytes each together
   size_t outLen  = (*length + 3 - tail)/3*4;              // base64 encoding outputs 4 bytes for each 3 bytes input
          outLen += (outLen/76 + (outLen%76 != 0))*2 + 1;  // plus 2 bytes for crlf for each output line of 76 char and a final '\0'
          if (head) outLen += 4;

   char *p;
   char *result = p = allocate(outLen, default_align, false);

   if (result)
   {
      uint *line64 = (uint *)result;

      if (head)
         *line64++ = head;

      for (i = 0, j = 0; i < *length-tail; i += 3)
      {
         line64[j++] = encode3((uchar *)&data[i]);
         if (j == 19)
         {
            p = (char *)&line64[j];
            *p++ = '\r'; *p++ = '\n';
            line64 = (uint *)p;
            j = 0;
         }
      }

      if (tail)
      {
         line64[j++] = (tail == 1) ? encode1((uchar)data[i]) : encode2((uchar *)&data[i]);
         if (j == 19)
         {
            p = (char *)&line64[j];
            *p++ = '\r'; *p++ = '\n';
            line64 = (uint *)p;
            j = 0;
         }
      }

      if (j)
      {
         p = (char *)&line64[j];
         *p++ = '\r'; *p++ = '\n';
      }

      *p = '\0';
      *length = p - result;
   }

   return result;
}


static inline void decode4(uchar in[4], uchar out[3])
{
   static const unsigned dec256[256] =
   {
      0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
      0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
      0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x3E, 0x40, 0x40, 0x40, 0x3F,
      0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
      0x40, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E,
      0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x40, 0x40, 0x40, 0x40, 0x40,
      0x40, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
      0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0x40, 0x40, 0x40, 0x40, 0x40,
      0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
      0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
      0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
      0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
      0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
      0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
      0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
      0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40
   };

   unsigned din0 = dec256[in[0]];
   unsigned din1 = dec256[in[1]];
   unsigned din2 = dec256[in[2]];
   unsigned din3 = dec256[in[3]];
   out[0] = (uchar)(din0 << 2 | din1 >> 4);
   out[1] = (uchar)(din1 << 4 | din2 >> 2);
   out[2] = (uchar)(din2 << 6 | din3);
}


uint decodedLength(char *data, uint length, boolean crlfFlag)
{
   uint outLen = (length - (1 + crlfFlag)*(length/78 + (length%78 != 0)))*3/4;
   length -= 3;
   while (data[length--] == '=')
      outLen--;
   return outLen;
}

char *base64Decode(char *data, uint *length)
{
   boolean crlfFlag = true;
   uint    i, j, k, n, encLen = *length - (data[*length-1] == '\0');
   uint    outLen = decodedLength(data, encLen, crlfFlag);
   char    tail[3], *result;

   if ((result = allocate(outLen+1, default_align, false)) == NULL)
   {
      *length = 0;
      return NULL;
   }

   n = encLen - 6;
   for (i = 0, j = 0, k = 0; i < n; i += 4, k += 4)
   {
      if (k && k%76 == 0)
      {
         k = 0;
         if ((data[i] == '\r' || data[i] == '\n') && data[i+1] != '\n')
         {
            if (crlfFlag)
            {
               outLen = decodedLength(data, encLen, crlfFlag = false);
               if ((result = reallocate(result, outLen+1, false, true)) == NULL)
               {
                  *length = 0;
                  return NULL;
               }
               n = encLen - 5;
            }
            i++;
         }
         else    // only malformed base64 does not have crlf here
            i += 2;
      }
      decode4((uchar *)&data[i], (uchar *)&result[j]); j += 3;
   }
   decode4((uchar *)&data[i], (uchar *)tail);
   for (k = 0; k < 3 && j < outLen; k++, j++)
      result[j] = tail[k];

   *length = outLen;
   return result;
}


#if defined __APPLE__

   #include <uuid/uuid.h>

   char *generateUUID(void)
   {
      char *uuid_str = allocate(37, default_align, false);
      if (uuid_str)
      {
         uuid_t uuid_bin;
         uuid_generate_time(uuid_bin);
         uuid_unparse_lower(uuid_bin, uuid_str);
      }
      return uuid_str;
   }

#elif defined __FreeBSD__

   #include <uuid.h>

   char *generateUUID(void)
   {
      char    *uuid_str = NULL;
      uuid_t   uuid_bin;
      uint32_t rc;
      uuid_create(&uuid_bin, &rc);
      if (rc == uuid_s_ok && (uuid_str = allocate(37, default_align, false)))
         snprintf(uuid_str, 37, "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                  uuid_bin.time_low, uuid_bin.time_mid, uuid_bin.time_hi_and_version,
                  uuid_bin.clock_seq_hi_and_reserved, uuid_bin.clock_seq_low,
                  uuid_bin.node[0], uuid_bin.node[1], uuid_bin.node[2],
                  uuid_bin.node[3], uuid_bin.node[4], uuid_bin.node[5]);

      return uuid_str;
   }

#endif


static const uchar trailingBytesForUTF8[256] =
{
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
   2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
};

static const utf32 offsetsFromUTF8[6] = { 0x00000000, 0x00003080, 0x000E2080, 0x03C82080, 0xFA082080, 0x82082080 };
static const uchar firstByteMark[7]   = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };
static const utf32 byteMask = 0xBF;
static const utf32 byteMark = 0x80;

utf32 utf8to32(uchar **v)
{
   utf32 u32 = 0;
   uchar tl, *u = (*v)++;

   if (*u < 0x80)
      return *u;

   else switch (tl = trailingBytesForUTF8[*u])
   {
      default:
         return 0xFFFD;

      case 3: u32 += *u++; u32 <<= 6;
      case 2: u32 += *u++; u32 <<= 6;
      case 1: u32 += *u++; u32 <<= 6;
      case 0: u32 += *u++;
   }
   *v = u;

   return u32 - offsetsFromUTF8[tl];
}


utf8 utf32to8(utf32 u32)
{
   utf8  u8 = 0;
   uchar l;

   if      (u32 < 0x80)     l = 1;
   else if (u32 < 0x800)    l = 2;
   else if (u32 < 0x10000)  l = 3;
   else if (u32 < 0x110000) l = 4;
   else   { u32 = 0xFFFD;   l = 3; }

   uchar *u =(uchar *)&u8 + l;
   switch (l)
   {
      case 4: *--u = (uchar)((u32 | byteMark) & byteMask); u32 >>= 6;
      case 3: *--u = (uchar)((u32 | byteMark) & byteMask); u32 >>= 6;
      case 2: *--u = (uchar)((u32 | byteMark) & byteMask); u32 >>= 6;
      case 1: *--u = (uchar) (u32 | firstByteMark[l]);
   }

   return u8;
}


#pragma mark ••• URI encoding/decoding and HTML entity encoding •••

char *uriDecode(char *element)
{
   static const char hex[256] =
   {
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1,
      -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
   };

   if (element)
   {
      uchar c, *p, *q;
      char h1, h2;
      p = q = (uchar *)element;

      while (*p)
      {
         if ((c = *p++) == '%' && (h1 = hex[*p]) != -1 && (h2 = hex[*(p+1)]) != -1)
            p += 2, c = h2 + (uchar)((uchar)h1 << 4);
         *q++ = c;
      }
      *q = '\0';
   }

   return element;
}


char *uriEncode(char *element, char *buffer)       // if buffer is NULL, then the space for the encoded string will be
{                                                  // allocated, and it needs to be deallocated by the caller.
   if (element)
   {
      char *p = element;
      char *q = element = (buffer) ?: allocate(strvlen(p)*3 + 1, default_align, false);

      if (q)
      {
         char  c, h;
         while (c = *p++)
            switch (c)
            {
               case '0' ... '9':
               case 'A' ... 'Z':
               case 'a' ... 'z':
               case '/':
               case ':':
               case '-':
               case '_':
               case '.':
               case '~':
                  *q++ = c;
                  break;

               default:
                  *q++ = '%';
                  h = (c >> 4) & 0xF;
                  *q++ = (h <= 9) ? (h + '0') : (h + 'A' - 10);
                  h = c & 0xF;
                  *q++ = (h <= 9) ? (h + '0') : (h + 'A' - 10);
            }

         *q = '\0';
      }
   }

   return element;
}


char *entEncode(char *element, char *buffer)       // if buffer is NULL, then the space for the encoded string will be
{                                                  // allocated, and it needs to be deallocated by the caller.
   if (element)
   {
      char *p = element;
      char *q = element = (buffer) ?: allocate(strvlen(p)*6 + 1, default_align, false);

      if (q)
      {
         boolean b;
         int     k;
         char    h;
         utf8    c;
         utf32   u;

         while (c = *p)
            switch (c)
            {
               case '0' ... '9':
               case 'A' ... 'Z':
               case 'a' ... 'z':
                  *q++ = *p++;
                  break;

               default:
                  u = utf8to32((uchar **)&p);
                  cpy4(q, "&#x"), q += 3;
                  for (k = 28, b = false; k >= 8; k -= 4)
                     if ((h = (u >> k) & 0xF) || b)
                     {
                        *q++ = (h <= 9) ? (h + '0') : (h + 'A' - 10);
                        b = true;
                     }
                  h = (u >> 4) & 0xF;
                  *q++ = (h <= 9) ? (h + '0') : (h + 'A' - 10);
                  h = u & 0xF;
                  *q++ = (h <= 9) ? (h + '0') : (h + 'A' - 10);
                  *q++ = ';';
            }

         *q = '\0';
      }
   }

   return element;
}


#pragma mark ••• Number to String conversions int2str(), int2hex() and num2str() •••

int int2str(char *ist, llong i, int m, int width)
{
   if (i == 0)
      if (m > 1)
      {
         int k;
         for (k = 0; k < width-1 && k < m-2; k++)
            ist[k] = ' ';
         cpy2(ist+k, "0");
         return k+1;
      }
      else            // result won't fit
         return 0;
   else
   {
      boolean neg = (i < 0);
      if (neg)
         i = llabs(i);

      int n = intlg(i) + 1 + neg;
      if (n > m-1)    // result won't fit
         return 0;

      if (n < width && width < m)
         n = width;

      ist[n] = '\0';

      for (m = n-1; i; i /= 10, m--)
        ist[m] = '0' + i%10;

      if (neg)
         ist[m--] = '-';

      while (m >= 0)
         ist[m--] = ' ';

      return n;
   }
}


int int2hex(char *hex, llong i, int m, int width)
{
   union
   {
      llong l;
      uchar b[sizeof(llong)];
   } bin = {.l = (llong)MapInt64(i)};

   uchar c;
   int   j, k;

   cpy2(hex, "0x");
   for (j = 0, k = 2; j < sizeof(llong); j++)
   {
      if ((c = (bin.b[j] >> 4) & 0xF) || k > 2)
         hex[k++] = (c <= 9) ? (c + '0') : (c + 'a' - 10);

      if ((c = bin.b[j] & 0xF) || k > 2)
         hex[k++] = (c <= 9) ? (c + '0') : (c + 'a' - 10);
   }

   hex[k] = '\0';
   return k;
}


static inline long double pow10pl(int n)                 // ...pl => n must be positive
{
   long double z = 1.0L;

   if (n < 9)
      for (int i = 0; i < n; i++)                        // power base 10 by repeated multiplication
         z *= 10.0L;
   else
      for (long double b = 10.0L;; b *= b)               // power base 10 by repeated squaring
      {
         if (n & 1)
            z *= b;

         if (!(n >>= 1))
            break;
      }

   return z;
}

static inline long double pow11pl(int n)                 // ...pl => n must be positive
{
   long double z = 1e+100L; n -= 100;

   if (n < 9)
      for (int i = 0; i < n; i++)                        // power base 10 by repeated multiplication
         z *= 10.0L;
   else
      for (long double b = 10.0L;; b *= b)               // power base 10 by repeated squaring
      {
         if (n & 1)
            z *= b;

         if (!(n >>= 1))
            break;
      }

   return z;
}

static inline llong mgnround(long double x, int mgn)
{
   static long double pow10pi[100] =
   {
      1e+00L,1e+01L,1e+02L,1e+03L,1e+04L,1e+05L,1e+06L,1e+07L,1e+08L,1e+09L, 1e+10L,1e+11L,1e+12L,1e+13L,1e+14L,1e+15L,1e+16L,1e+17L,1e+18L,1e+19L,
      1e+20L,1e+21L,1e+22L,1e+23L,1e+24L,1e+25L,1e+26L,1e+27L,1e+28L,1e+29L, 1e+30L,1e+31L,1e+32L,1e+33L,1e+34L,1e+35L,1e+36L,1e+37L,1e+38L,1e+39L,
      1e+40L,1e+41L,1e+42L,1e+43L,1e+44L,1e+45L,1e+46L,1e+47L,1e+48L,1e+49L, 1e+50L,1e+51L,1e+52L,1e+53L,1e+54L,1e+55L,1e+56L,1e+57L,1e+58L,1e+59L,
      1e+60L,1e+61L,1e+62L,1e+63L,1e+64L,1e+65L,1e+66L,1e+67L,1e+68L,1e+69L, 1e+70L,1e+71L,1e+72L,1e+73L,1e+74L,1e+75L,1e+76L,1e+77L,1e+78L,1e+79L,
      1e+80L,1e+81L,1e+82L,1e+83L,1e+84L,1e+85L,1e+86L,1e+87L,1e+88L,1e+89L, 1e+90L,1e+91L,1e+92L,1e+93L,1e+94L,1e+95L,1e+96L,1e+97L,1e+98L,1e+99L
   };

   if (!mgn)
      return llroundl(x);
   else
   {
      long double z;
      boolean pos = (mgn > 0);
      int n = (pos) ? mgn : -mgn;

      if      (n <= 29)
         z = pow10pl(n);
      else if (n <= 99)
         z = pow10pi[n];
      else
         z = pow11pl(n);

      return llroundl(((pos) ? x*z : x/z));
   }
}

int num2str(char *dst, long double x, int m, int width, int digits, int formsel, char decsep)
{
   if (m < 2 || width < 0 || digits < 0)
      return 0;

   boolean minussign = signbit(x);
   boolean negzero   = (formsel&non_zero) == 0;
   boolean plussign  = (formsel&pls_sign) != 0;
   boolean exposign  = (formsel&noe_sign) == 0;
   boolean capitals  = (formsel&cap_litr) != 0;
   boolean nostrip0  = (formsel&alt_form) || (formsel&f_form) || (formsel&e_form);
   boolean dangleds  = (formsel&alt_form) &&!(formsel&d_form) &&!(formsel&nod_dsep);
   boolean dsspace   = (formsel&sup_dsep)?0:1;                    //space for the decimal separator -- 0 in case it shall be suppressed
   formsel &= b_mask;

   if (digits && (formsel&(d_form|g_form)))
      digits--;

   if (width > m-1)
      width = m-1;

   int k, l = 0;
   if (isfinite(x))
   {
      if (x != 0.0L)
      {
         int ilg = intlgl(x = fabsl(x));
         int xtd = digits;
         if (formsel != f_form)
         {
            if (digits > 17)
               digits = 17;
            digits -= ilg;
            xtd -= ilg;
         }
         else if (digits + ilg > 17)
            digits = 17 - ilg;
         else if (digits + ilg < -1)                              // -1 instead of 0 leaves space for a possible rounding up
            goto zero;                                            // in the exact f_form, the number of desired decimal digits is not sufficient for anything else than zero.
         xtd -= digits;

         if (formsel == g_form)
            formsel = (-4 <= ilg && 0 <= digits) ? f_form : e_form;

      // Magnitude rounding and BCD conversion
         llong v = (digits <= __LDBL_MAX_10_EXP__)
                 ? mgnround(x, digits)
                 : mgnround(x*1e100, digits-100);                 // in the case of numbers which are close to __LDBL_DENORM_MIN__, do the magnround() in 2 steps

         if (v == 0LL)
            goto zero;                                            // the value has been rounded to zero and we may skip the BCD stage

         int o, p, q, w;
         uchar bcd[32] __attribute__((aligned(16))) = {};         // size of 18 bytes would be sufficient, however, we want this to be aligned on a 16byte boundary anyway
         for (p = -1; v && p < 31; v /= 10)                       // at the end of the loop, p points to the position of the most significant non-zero byte in the BCD buffer
            bcd[++p] = v % 10;
         ilg = p - digits;                                        // rounding in the course of BCD conversion may have resulted in an incremented intlg

      // Digit extraction from the BCD buffer
         // Determine various characteristic indexes
         //  p - q: range of significant bytes in the reversed BCD buffer               |o    |p  |q
         //  o    : extension of zeros to the left of the siginificant digits - example 0.000054321
         //                                                   otherwise o = p - example 3.141593
         //                                                                             |o=p   |q
         //  k    : decimal separator + number of trailing zeros

         if (p < ilg+digits && formsel == e_form)
            ilg--, digits++, xtd++;

         o = (p > digits || formsel == e_form) ? p : digits;      // extension of zeros to the left of the siginificant digits

         if (0 < digits && digits <= o || formsel == e_form)      // does the number contain a fraction, or has a fraction by definition (e_form)
         {
            if (nostrip0)                                         // no stripping of non-significant zeros?
            {
               q = 0;
               if (formsel != e_form)
                  k = (dangleds || digits) ? dsspace : 0;         // reserve 1 byte for the decimal separator
               else
                  k = (dangleds || digits+ilg) ? dsspace : 0;     // reserve 1 byte for the decimal separator
            }

            else
            {
               xtd = 0;
               for (q = 0; q < p && bcd[q] == 0; q++);            // strip trailing non-significant zeros; q points to the position of the least significant non-zero digit
               if (formsel != e_form)
               {
                  if (p - q < ilg)
                     q = (p > ilg) ? p - ilg : 0;                 // don't strip off zeros before the decimal separator
                  k = (p - q > ilg || ilg < 0) ? dsspace : 0;     // strip the decicmal separator or not?
               }
               else
                  k = (p - q) ? dsspace : 0;                      // strip the decicmal separator or not?
            }
         }

         else                                                     // no fraction
         {
            q = 0;
            if (digits <= 0 && xtd)                               // very large integral numbers > 18 digits must
               xtd += digits,                                     // be extended by significant zeros to the right
               k = (xtd || dangleds) ? dsspace-digits : -digits;  // number of trailing zeros of an integral number + perhaps a dangling decimal separator
            else
               k = (dangleds) ? dsspace-digits : -digits;         // number of trailing zeros of an integral number + perhaps a dangling decimal separator
         }

      // Calcultate the length of the number and check it against the supplied buffer
         boolean kpow = (ilg < -999 || 999 < ilg);
         boolean hpow = kpow ||
                         (ilg < -99 || 99 < ilg);
         k += o - q + 1;                                          // decimal separator (k) + number of digits
         m -= w = ((minussign || plussign) ? 1 : 0) + k + xtd;    // actual width of the number
         if (formsel == e_form)
         {
            m -= (ilg < 0 || exposign) ? 4 : 3;
            if (kpow)
               m -= 2;
            else if (hpow)
               m--;
         }

         if (m < 1)
         {
            cpy2(dst, "!");                                       // the result won't fit into the supplied buffer
            return 1;
         }


      // Construct the actual number, by directly placing the parts into the supplied buffer
         // Left padding
         if (width)
            for (width -= w; l < width; l++)
               dst[l] = ' ';                                      // left-padding with spaces

         // Signs
         if (minussign)
            dst[l++] = '-';
         else if (plussign)
            dst[l++] = '+';

      // Digit extraction and placement of the decimal separator
         int dsm = l + k;                                         // decimals stop mark
         int dsp = l + ((formsel == e_form)?1:1+o-digits);        // decimal separator position
         for (; o && o > p; o--)
         {
            dst[l++] = '0';
            if (l == dsp && dsspace)
               dst[l++] = decsep;
         }

      // Transfer the BCD buffer
         for (; p >= q && l != dsm; p--)                          // add the significant digits
         {
            if (l == dsp && dsspace)
               dst[l++] = decsep;
            if (l != dsm)
               dst[l++] = '0' + bcd[p];
         }

         for (; l < dsm && l < dsp; l++)                          // add siginficant zeros after the digits and before the decimal separator
            dst[l] = '0';

         if (l == dsp && (dangleds || xtd) && dsspace)            // add a dangling decimal separator
            dst[l++] = decsep;
                                                                  //                                                   |<- xtd ->|
         while (xtd--)                                            // add the extended zero trail                       V         V
            dst[l++] = '0';                                       // 3141592653589793000000000000000000000000000000000.00000000000

      // Exponent
         if (formsel == e_form)
         {
            dst[l++] = (!capitals) ? 'e' : 'E';
            if (ilg < 0)
            {
               dst[l++] = '-';
               ilg = -ilg;
            }
            else if (exposign)
               dst[l++] = '+';

            if (kpow)
            {
              dst[l++] = '0'+(ilg/1000)%10;
              dst[l++] = '0'+(ilg/100)%10;
            }
            else if (hpow)
              dst[l++] = '0'+(ilg/100)%10;
            dst[l++] = '0'+(ilg/10)%10;
            dst[l++] = '0'+ ilg%10;
         }
      }

      else // (x == 0.0L)
      {
      zero:
         minussign = minussign && negzero;

         m -= k = ((formsel == e_form)?(exposign)?4:3:0)
                  + ((digits && nostrip0 || dangleds)?digits+1+dsspace:1)
                  + ((minussign || plussign)?1:0);
         if (m < 1)
            return 0;                                             // the result won't fit into the supplied buffer

         if (width)
            for (; l < width - k; l++)
               dst[l] = ' ';                                      // padding with spaces

         if (minussign)
            dst[l++] = '-';
         else if (plussign)
            dst[l++] = '+';

         if (digits && nostrip0 || dangleds)
         {
            dst[l++] = '0';
            if (dsspace)
               dst[l++] = decsep;
            for (int i = 0; i < digits; i++)
               dst[l++] = '0';
         }
         else
            dst[l++] = '0';

         if (formsel == e_form)
         {
            if (!capitals)
               if (!exposign)
                  cpy4(dst+l, "e00\0"), l += 3;
               else
                  cpy4(dst+l, "e+00" ), l += 4;
            else
               if (!exposign)
                  cpy4(dst+l, "E00\0"), l += 3;
               else
                  cpy4(dst+l, "E+00" ), l += 4;
         }
      }
   }

   else // (isinf(x) || isnan(x))
   {
      m -= k = ((minussign || plussign)?4:3);
      if (m < 1)
         return 0;                                                // the result won't fit into the supplied buffer

      if (width)
         for (; l < width - k; l++)
            dst[l] = ' ';                                         // padding with spaces

      if (isinf(x))
      {
         if (!capitals)
         {
            if (minussign)
               cpy4(dst+l, "-inf"), l += 4;
            else if (plussign)
               cpy4(dst+l, "+inf"), l += 4;
            else
               cpy4(dst+l,  "inf"), l += 3;
         }
         else
         {
            if (minussign)
               cpy4(dst+l, "-INF"), l += 4;
            else if (plussign)
               cpy4(dst+l, "+INF"), l += 4;
            else
               cpy4(dst+l,  "INF"), l += 3;
         }
      }

      else // isnan(x)
      {
         if (!capitals)
         {
            if (minussign)
               cpy4(dst+l, "-nan"), l += 4;
            else if (plussign)
               cpy4(dst+l, "+nan"), l += 4;
            else
               cpy4(dst+l,  "nan"), l += 3;
         }
         else
         {
            if (minussign)
               cpy4(dst+l, "-NAN"), l += 4;
            else if (plussign)
               cpy4(dst+l, "+NAN"), l += 4;
            else
               cpy4(dst+l,  "NAN"), l += 3;
         }
      }
   }

   dst[l] = '\0';
   return l;
}


#pragma mark ••• Fencing Memory Allocation Wrappers •••
// FEATURES
// -- optional clean-out in all stage, allocation, re-allocation and de-allocation
// -- specify explicit alignment
// -- check fence below the payload
// -- zero  fence above the payload
// -- deallocate via handle which places NULL into the pointer

ssize_t gAllocationTotal = 0;

static inline void countAllocation(ssize_t size)
{
   if (__atomic_add_fetch(&gAllocationTotal, size, __ATOMIC_RELAXED) < 0)
   {
      syslog(LOG_ERR, "Corruption of allocated memory detected by countAllocation().");
      exit(EXIT_FAILURE);
   }
}

static inline uint8_t padcalc(void *ptr, uint8_t align)
{
   if (align > 1)
   {
      uint8_t padis = ((uintptr_t)ptr%align);
      return (padis) ? align - padis : 0;
   }
   else
      return 0;
}

void *allocate(ssize_t size, uint8_t align, boolean cleanout)
{
   if (size >= 0)
   {
      allocation *a;

      if ((a = malloc(allocationMetaSize + align + size + sizeof(size_t))) == NULL)
         return NULL;

      if (cleanout)
         memset((void *)a, 0, allocationMetaSize + align + size + sizeof(size_t));
      else
         *(size_t *)((void *)a + allocationMetaSize + align + size) = 0;   // place a (size_t)0 just above the payload as the upper boundary of the allocation

      a->size  = size;
      a->check = (unsigned)(size | (size_t)a);
      a->fence = 'lf';     // lower fence
      a->align = align;
      a->padis = padcalc(a->payload, align);

      void *p = a->payload + a->padis;
      *(uint8_t *)(p-2) = a->align;
      *(uint8_t *)(p-1) = a->padis;

      countAllocation(size);
      return p;
   }

   else
      return NULL;
}

void *reallocate(void *p, ssize_t size, boolean cleanout, boolean free_on_error)
{
   if (size >= 0)
      if (p)
      {
         uint8_t align = *(uint8_t *)(p-2);
         uint8_t padis = *(uint8_t *)(p-1);
         allocation *a = p - allocationMetaSize - padis;

         if (a->check != (unsigned)(a->size | (size_t)a) || a->fence != 'lf' || a->align != align || a->padis != padis
          || *(ssize_t *)((void *)a + allocationMetaSize + align + a->size) != 0)
         {
            syslog(LOG_ERR, "Corruption of allocated memory detected by reallocate().");
            exit(EXIT_FAILURE);
         }

         allocation *b;
         if ((b = malloc(allocationMetaSize + align + size + sizeof(size_t))) == NULL)
         {
            if (free_on_error)
            {
               if (cleanout)
                  memset((void *)a, 0, allocationMetaSize + align + a->size + sizeof(size_t));
               countAllocation(-a->size);
               free(a);
            }

            return NULL;
         }

         else
         {
            if (cleanout)
               memset((void *)b, 0, allocationMetaSize + align + size + sizeof(size_t));
            else
               *(size_t *)((void *)b + allocationMetaSize + align + size) = 0;   // place a (size_t)0 just above the payload as the upper boundary of the allocation

            b->size  = size;
            b->check = (unsigned)(size | (size_t)b);
            b->fence = 'lf';     // lower fence
            b->align = align;
            b->padis = padcalc(b->payload, align);

            void *q = b->payload + b->padis;
            *(uint8_t *)(q-2) = b->align;
            *(uint8_t *)(q-1) = b->padis;
            memvcpy(q, p, (a->size <= size) ? a->size : size);

            if (cleanout)
               memset((void *)a, 0, allocationMetaSize + align + a->size + sizeof(size_t));
            countAllocation(size - a->size);
            free(a);
            return q;
         }
      }
      else
         return allocate(size, default_align, cleanout);

   return NULL;
}

void deallocate(void **p, boolean cleanout)
{
   if (p && *p)
   {
      uint8_t align = *(uint8_t *)(*p-2);
      uint8_t padis = *(uint8_t *)(*p-1);
      allocation *a = *p - allocationMetaSize - padis;
      *p = NULL;

      if (a->check != (unsigned)(a->size | (size_t)a) || a->fence != 'lf' || a->align != align || a->padis != padis
       || *(ssize_t *)((void *)a + allocationMetaSize + align + a->size) != 0)
      {
         syslog(LOG_ERR, "Corruption of allocated memory detected by deallocate().");
         exit(EXIT_FAILURE);
      }

      if (cleanout)
         memset((void *)a, 0, allocationMetaSize + align + a->size + sizeof(size_t));
      countAllocation(-a->size);
      free(a);
   }
}

void deallocate_batch(int cleanout, ...)
{
   void   **p;
   va_list  vl;
   va_start(vl, cleanout);

   while (p = va_arg(vl, void **))
      if (*p)
      {
         uint8_t align = *(uint8_t *)(*p-2);
         uint8_t padis = *(uint8_t *)(*p-1);
         allocation *a = *p - allocationMetaSize - padis;
         *p = NULL;

         if (a->check != (unsigned)(a->size | (size_t)a) || a->fence != 'lf' || a->align != align || a->padis != padis
          || *(ssize_t *)((void *)a + allocationMetaSize + align + a->size) != 0)
         {
            syslog(LOG_ERR, "Corruption of allocated memory detected by deallocate_batch().");
            exit(EXIT_FAILURE);
         }

         if (cleanout)
            memset((void *)a, 0, allocationMetaSize + align + a->size + sizeof(size_t));
         countAllocation(-a->size);
         free(a);
      }

   va_end(vl);
}

ssize_t allocsize(void *p)
{
   return (p)
      ? ((allocation *)(p - allocationMetaSize - *(uint8_t *)(p-1)))->size
      : 0;
}
