/*
*Copyright (C) 2011 Google Inc. All rights reserved.
 *
*Redistribution and use in source and binary forms, with or without
*modification, are permitted provided that the following conditions are
*met:
 *
** Redistributions of source code must retain the above copyright
*notice, this list of conditions and the following disclaimer.
** Redistributions in binary form must reproduce the above
*copyright notice, this list of conditions and the following disclaimer
*in the documentation and/or other materials provided with the
*distribution.
** Neither the name of Google Inc. nor the names of its
*contributors may be used to endorse or promote products derived from
*this software without specific prior written permission.
 *
*THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
*OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
*SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
*LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
*(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
*OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// A straightforward SHA-1 implementation based on RFC 3174.
// http://www.ietf.org/rfc/rfc3174.txt
// The names of functions and variables (such as "a", "b", and "f")
// follow notations in RFC 3174.

#ifndef __SHA1_H__
#define __SHA1_H__

#include<stdio.h>
#include<stdint.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>

class SHA1 {
	void finalize();
	void processBlock();
	void reset();

	uint8_t m_buffer[64];
	size_t m_cursor;
	uint64_t m_totalBytes;
	uint32_t m_hash[5];
	static inline uint32_t f(int t, uint32_t b, uint32_t c, uint32_t d) {
		if (t < 20) return (b & c) | ((~b) & d);
		if (t < 40) return b ^ c ^ d;
		if (t < 60) return (b & c) | (b & d) | (c & d);
		return b ^ c ^ d;
	}
	static inline uint32_t k(int t) {
		if (t < 20) return 0x5a827999;
		if (t < 40) return 0x6ed9eba1;
		if (t < 60) return 0x8f1bbcdc;
		return 0xca62c1d6;
	}
	static inline uint32_t rotateLeft(int n, uint32_t x) {
		return (x << n) | (x >> (32-n));
	}
public:
	SHA1() { reset(); }
	void addBytes(const uint8_t* input, size_t length);
	void computeHash(uint8_t *digest);

};

#endif
