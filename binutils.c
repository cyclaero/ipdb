//  binutils.c
//  ipdb / ipup / geod
//
//  Created by Dr. Rolf Jansen on 2016-08-13.
//  Copyright (c) 2016 projectworld.net. All rights reserved.
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
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS
//  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
//  AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
//  OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
//  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
//  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
//  THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <syslog.h>

#include "binutils.h"

#pragma mark ••• Fencing Memory Allocation Wrappers •••

ssize_t gAllocationTotal = 0;

static inline void countAllocation(ssize_t size)
{
   if (__sync_add_and_fetch(&gAllocationTotal, size) < 0)
   {
      syslog(LOG_ERR, "Corruption of allocated memory detected by countAllocation().");
      exit(EXIT_FAILURE);
   }
}

void *allocate(ssize_t size, bool cleanout)
{
   if (size >= 0)
   {
      allocation *a;

      if (cleanout)
      {
         if ((a = calloc(1, allocationMetaSize + size + sizeof(size_t))) == NULL)
            return NULL;
      }

      else
      {
         if ((a = malloc(allocationMetaSize + size + sizeof(size_t))) == NULL)
            return NULL;

         *(size_t *)((void *)a + allocationMetaSize + size) = 0;   // place a (size_t)0 just above the payload as the upper boundary of the allocation
      }

      countAllocation(size);
      a->size  = size;
      a->check = size | (size_t)a;
      return &a->payload;
   }
   else
      return NULL;
}

void *reallocate(void *p, ssize_t size, bool cleanout, bool free_on_error)
{
   if (size >= 0)
      if (p)
      {
         allocation *a = p - allocationMetaSize;

         if (a->check == (a->size | (size_t)a) && *(ssize_t *)((void *)a + allocationMetaSize + a->size) == 0)
            a->check = 0;
         else
         {
            syslog(LOG_ERR, "Corruption of allocated memory detected by reallocate().");
            exit(EXIT_FAILURE);
         }

         if ((p = realloc(a, allocationMetaSize + size + sizeof(size_t))) == NULL)
         {
            if (free_on_error)
            {
               countAllocation(-a->size);
               free(a);
            }
            return NULL;
         }
         else
            a = p;

         if (cleanout)
            bzero((void *)a + allocationMetaSize + a->size, size - a->size + sizeof(size_t));
         else
            *(ssize_t *)((void *)a + allocationMetaSize + size) = 0;   // place a (size_t)0 just above the payload as the upper boundary of the allocation

         countAllocation(size - a->size);
         a->size  = size;
         a->check = size | (size_t)a;
         return &a->payload;
      }
      else
         return allocate(size, cleanout);

   return NULL;
}

void deallocate(void **p, bool cleanout)
{
   if (p && *p)
   {
      allocation *a = *p - allocationMetaSize;
      *p = NULL;

      if (a->check == (a->size | (size_t)a) && *(ssize_t *)((void *)a + allocationMetaSize + a->size) == 0)
         a->check = 0;
      else
      {
         syslog(LOG_ERR, "Corruption of allocated memory detected by deallocate().");
         exit(EXIT_FAILURE);
      }

      countAllocation(-a->size);
      if (cleanout)
         bzero((void *)a, allocationMetaSize + a->size + sizeof(size_t));
      free(a);
   }
}

void deallocate_batch(unsigned cleanout, ...)
{
   void   **p;
   va_list  vl;
   va_start(vl, cleanout);

   while (p = va_arg(vl, void **))
      if (*p)
      {
         allocation *a = *p - allocationMetaSize;
         *p = NULL;

         if (a->check == (a->size | (size_t)a) && *(ssize_t *)((void *)a + allocationMetaSize + a->size) == 0)
            a->check = 0;
         else
         {
            syslog(LOG_ERR, "Corruption of allocated memory detected by deallocate_batch().");
            exit(EXIT_FAILURE);
         }

         countAllocation(-a->size);
         if (cleanout)
            bzero((void *)a, allocationMetaSize + a->size + sizeof(size_t));
         free(a);
      }

   va_end(vl);
}


#pragma mark ••• uint128 Arithmetic •••

#if !defined(__x86_64__) && !defined(__arm64__) || defined(UInt128_Testing)

   uint128t mul_u128(uint128t a, uint128t b)
   {
      if (a.quad[b2_0] == 0 && a.quad[b2_1] == 0 || b.quad[b2_0] == 0 && b.quad[b2_1] == 0)
         return (uint128t){0, 0};

      else if (a.quad[b2_0] == 0 && a.quad[b2_1] == 1)
         return b;

      else if (b.quad[b2_0] == 0 && b.quad[b2_1] == 1)
         return a;

      else
      {
         uint64_t k, t, w1, w2, w3,
                  u0 = a.quad[b2_0] >> 32,
                  u1 = a.quad[b2_0] & 0xFFFFFFFF,
                  v0 = b.quad[b2_0] >> 32,
                  v1 = b.quad[b2_0] & 0xFFFFFFFF;

         t  = u1*v1;
         w3 = t & 0xFFFFFFFF;
         k  = t >> 32;

         t  = u0*v1 + k;
         w2 = t & 0xFFFFFFFF;
         w1 = t >> 32;

         t  = u1*v0 + w2;
         k  = t >> 32;

         u1 = u0*v0 + w1 + k;
         u0 = (t << 32) + w3;

         a.quad[b2_1] = u1 + a.quad[b2_0]*b.quad[b2_1] + a.quad[b2_1]*b.quad[b2_0];
         a.quad[b2_0] = u0;

         return a;
      }
   }


   static inline int32_t normalize(uint64_t u)
   {
      int32_t m = 0;
      if (u <= 0x00000000FFFFFFFF) m += 32, u <<= 32;
      if (u <= 0x0000FFFFFFFFFFFF) m += 16, u <<= 16;
      if (u <= 0x00FFFFFFFFFFFFFF) m +=  8, u <<=  8;
      if (u <= 0x0FFFFFFFFFFFFFFF) m +=  4, u <<=  4;
      if (u <= 0x3FFFFFFFFFFFFFFF) m +=  2, u <<=  2;
      if (u <= 0x7FFFFFFFFFFFFFFF) m +=  1;
      return m;
   }

   static void div_u128_64(uint128t u, uint64_t v, uint64_t *q, uint64_t *r)
   {
      if (!v || u.quad[b2_1] >= v)
      {
         if (q) *q = 0xFFFFFFFFFFFFFFFF;
         if (r) *r = 0xFFFFFFFFFFFFFFFF;
         return;
      }

      static const uint64_t b = 0x100000000;

      uint64_t un1, un0,
               vn1, vn0,
               q1, q0,
               un32, un21, un10,
               rhat;

      int64_t  s = normalize(v);

      v <<= s;
      vn1 = v >> 32;
      vn0 = v & 0xFFFFFFFF;

      un32 = (u.quad[b2_1] << s) | (u.quad[b2_0] >> (64 - s)) & (-s >> 63);
      un10 = u.quad[b2_0] << s;

      un1 = un10 >> 32;
      un0 = un10 & 0xFFFFFFFF;

      q1 = un32/vn1;
      rhat = un32 - q1*vn1;

   again1:
      if (q1 >= b || q1*vn0 > b*rhat + un1)
      {
         q1--;
         rhat += vn1;
         if (rhat < b)
            goto again1;
      }

      un21 = un32*b + un1 - q1*v;

      q0 = un21/vn1;
      rhat = un21 - q0*vn1;

   again2:
      if (q0 >= b || q0*vn0 > b*rhat + un0)
      {
         q0--;
         rhat = rhat + vn1;
         if (rhat < b)
            goto again2;
      }

      if (q) *q = q1*b + q0;
      if (r) *r = (un21*b + un0 - q0*v) >> s;
   }


   void divrem_u128(uint128t a, uint128t b, uint128t *q, uint128t *r)
   {
      if (b.quad[b2_1] == 0)
      {
         if (a.quad[b2_1] < b.quad[b2_0])
         {
            div_u128_64(a, b.quad[b2_0], (q)?&q->quad[b2_0] : NULL, (r)?&r->quad[b2_0]:NULL);
            if (q) q->quad[b2_1] = 0;
            if (r) r->quad[b2_1] = 0;
         }

         else
         {
            if (q) q->quad[b2_1] = a.quad[b2_1]/b.quad[b2_0];
            a.quad[b2_1] = a.quad[b2_1]%b.quad[b2_0];
            div_u128_64(a, b.quad[b2_0], (q)?&q->quad[b2_0] : NULL, (r)?&r->quad[b2_0]:NULL);
            if (r) r->quad[b2_1] = 0;
         }
      }

      else
      {
         int32_t n  = normalize(b.quad[b2_1]);
         uint128t v1 = shl_u128(b, n);
         uint128t u1 = shr_u128(a, 1);
         uint128t q1, r1;
         div_u128_64(u1, v1.quad[b2_1], &q1.quad[b2_0], NULL);
         q1.quad[b2_1] = 0;
         q1 = shr_u128(q1, 63 - n);

         if (q1.quad[b2_1] || q1.quad[b2_0])
            dec_u128(&q1);

         r1 = sub_u128(a, mul_u128(q1, b));
         if (ge_u128(r1, b))
         {
            inc_u128(&q1);
            r1 = sub_u128(r1, b);
         }

         if (q) *q = q1;
         if (r) *r = r1;
      }
   }

#endif
