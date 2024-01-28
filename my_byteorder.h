#ifndef MY_BYTEORDER_INCLUDED
#define MY_BYTEORDER_INCLUDED

/* Copyright (c) 2001, 2023, Oracle and/or its affiliates.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License, version 2.0,
   as published by the Free Software Foundation.

   This program is also distributed with certain software (including
   but not limited to OpenSSL) that is licensed under separate terms,
   as designated in a particular file or component or in included license
   documentation.  The authors of MySQL hereby grant you an additional
   permission to link the program and your derivative works with the
   separately licensed software that they have included with MySQL.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License, version 2.0, for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA */

/**
  @file include/my_byteorder.h
  Functions for reading and storing in machine-independent format.
  The little-endian variants are 'korr' (assume 'corrector') variants
  for integer types, but 'get' (assume 'getter') for floating point types.
*/

#include <string.h>
#include <sys/types.h>

#include <arpa/inet.h>

#if defined(_MSC_VER)
#include <stdlib.h>
#endif

#if defined(_WIN32) && defined(WIN32_LEAN_AND_MEAN)
#include <winsock2.h>
#endif

#ifdef WORDS_BIGENDIAN
#include "big_endian.h"  // IWYU pragma: export
#else
#include "little_endian.h"  // IWYU pragma: export
#endif

// #ifdef __cplusplus
// #include "template_utils.h"
// #endif

static inline int32_t sint3korr(const unsigned char *A) {
  return ((int32_t)(((A[2]) & 128)
                      ? (((uint32_t)255L << 24) | (((uint32_t)A[2]) << 16) |
                         (((uint32_t)A[1]) << 8) | ((uint32_t)A[0]))
                      : (((uint32_t)A[2]) << 16) | (((uint32_t)A[1]) << 8) |
                            ((uint32_t)A[0])));
}

static inline uint32_t uint3korr(const unsigned char *A) {
  return (uint32_t)(((uint32_t)(A[0])) + (((uint32_t)(A[1])) << 8) +
                  (((uint32_t)(A[2])) << 16));
}

static inline uint64_t uint5korr(const unsigned char *A) {
  return ((uint64_t)(((uint32_t)(A[0])) + (((uint32_t)(A[1])) << 8) +
                      (((uint32_t)(A[2])) << 16) + (((uint32_t)(A[3])) << 24)) +
          (((uint64_t)(A[4])) << 32));
}

static inline uint64_t uint6korr(const unsigned char *A) {
  return ((uint64_t)(((uint32_t)(A[0])) + (((uint32_t)(A[1])) << 8) +
                      (((uint32_t)(A[2])) << 16) + (((uint32_t)(A[3])) << 24)) +
          (((uint64_t)(A[4])) << 32) + (((uint64_t)(A[5])) << 40));
}

/**
  int3store

  Stores an unsigned integer in a platform independent way

  @param T  The destination buffer. Must be at least 3 bytes long
  @param A  The integer to store.

  _Example:_
  A @ref a_protocol_type_int3 "int \<3\>" with the value 1 is stored as:
  ~~~~~~~~~~~~~~~~~~~~~
  01 00 00
  ~~~~~~~~~~~~~~~~~~~~~
*/
static inline void int3store(unsigned char *T, uint32_t A) {
  *(T) = (unsigned char)(A);
  *(T + 1) = (unsigned char)(A >> 8);
  *(T + 2) = (unsigned char)(A >> 16);
}

static inline void int5store(unsigned char *T, uint64_t A) {
  *(T) = (unsigned char)(A);
  *(T + 1) = (unsigned char)(A >> 8);
  *(T + 2) = (unsigned char)(A >> 16);
  *(T + 3) = (unsigned char)(A >> 24);
  *(T + 4) = (unsigned char)(A >> 32);
}

static inline void int6store(unsigned char *T, uint64_t A) {
  *(T) = (unsigned char)(A);
  *(T + 1) = (unsigned char)(A >> 8);
  *(T + 2) = (unsigned char)(A >> 16);
  *(T + 3) = (unsigned char)(A >> 24);
  *(T + 4) = (unsigned char)(A >> 32);
  *(T + 5) = (unsigned char)(A >> 40);
}

#ifdef __cplusplus

inline int16_t sint2korr(const char *pT) {
  return sint2korr(static_cast<const unsigned char *>
      (static_cast<const void *>(pT)));
}

inline uint16_t uint2korr(const char *pT) {
  return uint2korr(static_cast<const unsigned char *>
      (static_cast<const void *>(pT)));
}

inline uint32_t uint3korr(const char *pT) {
  return uint3korr(static_cast<const unsigned char *>
      (static_cast<const void *>(pT)));
}

inline int32_t sint3korr(const char *pT) {
  return sint3korr(static_cast<const unsigned char *>
      (static_cast<const void *>(pT)));
}

inline uint32_t uint4korr(const char *pT) {
  return uint4korr(static_cast<const unsigned char *>
      (static_cast<const void *>(pT)));
}

inline int32_t sint4korr(const char *pT) {
  return sint4korr(static_cast<const unsigned char *>
      (static_cast<const void *>(pT)));
}

inline uint64_t uint6korr(const char *pT) {
  return uint6korr(static_cast<const unsigned char *>
      (static_cast<const void *>(pT)));
}

inline uint64_t uint8korr(const char *pT) {
  return uint8korr(static_cast<const unsigned char *>
      (static_cast<const void *>(pT)));
}

inline int64_t sint8korr(const char *pT) {
  return sint8korr(static_cast<const unsigned char *>
      (static_cast<const void *>(pT)));
}

inline void int2store(char *pT, uint16_t A) {
  int2store(static_cast<unsigned char *>(static_cast<void *>(pT)), A);
}

inline void int3store(char *pT, uint32_t A) {
  int3store(static_cast<unsigned char *>(static_cast<void *>(pT)), A);
}

inline void int4store(char *pT, uint32_t A) {
  int4store(static_cast<unsigned char *>(static_cast<void *>(pT)), A);
}

inline void int5store(char *pT, uint64_t A) {
  int5store(static_cast<unsigned char *>(static_cast<void *>(pT)), A);
}

inline void int6store(char *pT, uint64_t A) {
  int6store(static_cast<unsigned char *>(static_cast<void *>(pT)), A);
}

inline void int8store(char *pT, uint64_t A) {
  int8store(static_cast<unsigned char *>(static_cast<void *>(pT)), A);
}

/*
  Functions for reading and storing in machine format from/to
  short/long to/from some place in memory V should be a variable
  and M a pointer to byte.
*/

inline void float4store(char *V, float M) {
  float4store(static_cast<unsigned char *>(static_cast<void *>(V)), M);
}

inline double float8get(const char *M) {
  return float8get(static_cast<const unsigned char *>
      (static_cast<const void *>(M)));
}

inline void float8store(char *V, double M) {
  float8store(static_cast<unsigned char *>(static_cast<void *>(V)), M);
}

/*
 Functions that have the same behavior on little- and big-endian.
*/

inline float floatget(const unsigned char *ptr) {
  float val;
  memcpy(&val, ptr, sizeof(val));
  return val;
}

inline void floatstore(unsigned char *ptr, float val) {
  memcpy(ptr, &val, sizeof(val));
}

inline double doubleget(const unsigned char *ptr) {
  double val;
  memcpy(&val, ptr, sizeof(val));
  return val;
}

inline void doublestore(unsigned char *ptr, double val) {
  memcpy(ptr, &val, sizeof(val));
}

inline uint16_t ushortget(const unsigned char *ptr) {
  uint16_t val;
  memcpy(&val, ptr, sizeof(val));
  return val;
}

inline int16_t shortget(const unsigned char *ptr) {
  int16_t val;
  memcpy(&val, ptr, sizeof(val));
  return val;
}

inline void shortstore(unsigned char *ptr, int16_t val) {
  memcpy(ptr, &val, sizeof(val));
}

inline int32_t longget(const unsigned char *ptr) {
  int32_t val;
  memcpy(&val, ptr, sizeof(val));
  return val;
}

inline void longstore(unsigned char *ptr, int32_t val) {
  memcpy(ptr, &val, sizeof(val));
}

inline uint32_t ulongget(const unsigned char *ptr) {
  uint32_t val;
  memcpy(&val, ptr, sizeof(val));
  return val;
}

inline int64_t longlongget(const unsigned char *ptr) {
  int64_t val;
  memcpy(&val, ptr, sizeof(val));
  return val;
}

inline void longlongstore(unsigned char *ptr, int64_t val) {
  memcpy(ptr, &val, sizeof(val));
}

/*
 Functions for big-endian loads and stores. These are safe to use
 no matter what the compiler, CPU or alignment, and also with -fstrict-aliasing.

 The stores return a pointer just past the value that was written.
*/

inline uint16_t load16be(const char *ptr) {
  uint16_t val;
  memcpy(&val, ptr, sizeof(val));
  return ntohs(val);
}

inline uint32_t load32be(const char *ptr) {
  uint32_t val;
  memcpy(&val, ptr, sizeof(val));
  return ntohl(val);
}

inline char *store16be(char *ptr, uint16_t val) {
#if defined(_MSC_VER)
  // _byteswap_ushort is an intrinsic on MSVC, but htons is not.
  val = _byteswap_ushort(val);
#else
  val = htons(val);
#endif
  memcpy(ptr, &val, sizeof(val));
  return ptr + sizeof(val);
}

inline char *store32be(char *ptr, uint32_t val) {
  val = htonl(val);
  memcpy(ptr, &val, sizeof(val));
  return ptr + sizeof(val);
}

/**
  Casts from one pointer type, to another, without using
  reinterpret_cast or C-style cast:
    foo *f; bar *b= pointer_cast<bar*>(f);
  This avoids having to do:
    foo *f; bar *b= static_cast<bar*>(static_cast<void*>(f));
 */
template <typename T>
inline T pointer_cast(void *p) {
  return static_cast<T>(p);
}

template <typename T>
inline const T pointer_cast(const void *p) {
  return static_cast<T>(p);
}

// Adapters for using unsigned char * instead of char *.

inline uint16_t load16be(const unsigned char *ptr) {
  return load16be(pointer_cast<const char *>(ptr));
}

inline uint32_t load32be(const unsigned char *ptr) {
  return load32be(pointer_cast<const char *>(ptr));
}

inline unsigned char *store16be(unsigned char *ptr, uint16_t val) {
  return pointer_cast<unsigned char *>(
      store16be(pointer_cast<char *>(ptr), val));
}

inline unsigned char *store32be(unsigned char *ptr, uint32_t val) {
  return pointer_cast<unsigned char *>(
      store32be(pointer_cast<char *>(ptr), val));
}

#endif /* __cplusplus */

#endif /* MY_BYTEORDER_INCLUDED */
