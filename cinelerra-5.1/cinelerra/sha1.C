// see copyright notice in sha1.h
#include "sha1.h"

void SHA1::addBytes(const uint8_t* input, size_t length)
{
	m_totalBytes += length;
	while (length > 0) {
#if 1
// allow unaliged access
		uint64_t *buf = (uint64_t*)&m_buffer[m_cursor];
		const uint64_t *inp = (const uint64_t*)input;
		while (length >= sizeof(uint64_t) && m_cursor < 64) {
			*buf++ = *inp++;
			m_cursor += sizeof(uint64_t);
			length -= sizeof(uint64_t);
		}
		input = (const uint8_t *)inp;
#endif
		while (length > 0 && m_cursor < 64) {
			m_buffer[m_cursor++] = *input++;
			--length;
		}
		if( m_cursor >= 64 ) processBlock();
	}
}

void SHA1::computeHash(uint8_t *digest)
{
	finalize();
	memset(digest, 0, 20);
	for (size_t i = 0; i < 5; ++i) {
		// Treat hashValue as a big-endian value.
		uint32_t hashValue = m_hash[i];
		for (int j = 0; j < 4; ++j) {
			digest[4*i + (3-j)] = hashValue & 0xFF;
			hashValue >>= 8;
		}
	}

	reset();
}

void SHA1::finalize()
{
	m_buffer[m_cursor++] = 0x80;
	if (m_cursor > 56) {
		while (m_cursor < 64) m_buffer[m_cursor++] = 0x00;
		processBlock();
	}

	for (size_t i = m_cursor; i < 56; ++i)
		m_buffer[i] = 0x00;

	uint64_t bits = m_totalBytes*8;
	for (int i = 0; i < 8; ++i) {
		m_buffer[56 + (7-i)] = bits & 0xFF;
		bits >>= 8;
	}
	m_cursor = 64;
	processBlock();
}

void SHA1::processBlock()
{
	uint32_t w[80] = { 0 };
	for (int t = 0; t < 16; ++t)
		w[t] =	(m_buffer[t*4] << 24) |
		(m_buffer[t*4 + 1] << 16) |
		(m_buffer[t*4 + 2] << 8) |
		(m_buffer[t*4 + 3]);
	for (int t = 16; t < 80; ++t)
		w[t] = rotateLeft(1, w[t-3] ^ w[t-8] ^ w[t-14] ^ w[t-16]);

	uint32_t a = m_hash[0], b = m_hash[1];
	uint32_t c = m_hash[2], d = m_hash[3];
	uint32_t e = m_hash[4];

	for (int t = 0; t < 80; ++t) {
		uint32_t temp = rotateLeft(5, a) + f(t, b, c, d) + e + w[t] + k(t);
		e = d;  d = c;
		c = rotateLeft(30, b);
		b = a;  a = temp;
	}

	m_hash[0] += a;  m_hash[1] += b;
	m_hash[2] += c;  m_hash[3] += d;
	m_hash[4] += e;

	m_cursor = 0;
}

void SHA1::reset()
{
	m_cursor = 0;
	m_totalBytes = 0;
	m_hash[0] = 0x67452301;  m_hash[1] = 0xefcdab89;
	m_hash[2] = 0x98badcfe;  m_hash[3] = 0x10325476;
	m_hash[4] = 0xc3d2e1f0;
	memset(m_buffer, 0, sizeof(m_buffer));
}

#ifdef TEST
static void expectSHA1(const char *input, int repeat, const char *expected)
{
	SHA1 sha1;
	const uint8_t *inp = (const uint8_t *)input;
	int len = strlen(input);
	for (int i = 0; i < repeat; ++i) sha1.addBytes(inp, len);
	uint8_t digest[20];  sha1.computeHash(digest);
	char actual[64], *buffer = actual;
	for (size_t i = 0; i < 20; ++i) {
		snprintf(buffer, 3, "%02X", digest[i]);
		buffer += 2;
	}
	if( !strcmp(actual, expected) ) return;
	printf("input: %s, repeat: %d, actual: %s, expected: %s\n",
		input, repeat, actual, expected);
}

int main(int ac, char **av)
{
	// Examples taken from sample code in RFC 3174.
	expectSHA1("abc", 1, "A9993E364706816ABA3E25717850C26C9CD0D89D");
	expectSHA1("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
		1, "84983E441C3BD26EBAAE4AA1F95129E5E54670F1");
	expectSHA1("a",
		1000000, "34AA973CD4C4DAA4F61EEB2BDBAD27316534016F");
	expectSHA1("0123456701234567012345670123456701234567012345670123456701234567",
		10, "DEA356A2CDDD90C7A7ECEDC5EBB563934F460452");
	return 0;
}
#endif

