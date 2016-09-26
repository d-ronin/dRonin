// Routines to expand morse code for error pattern blinking, etc.
// MorseL (morse library)
//
// Copyright (c) 2016, dRonin
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <morsel.h>

/* Morse stuff is between 1 and 6 elements.
 * First 2 bits == 11 (punctuation), next 6 bits are character
 * First 3 bits == 000, 001, 010, 011, 100 for 1 to 5 elements,
 * following 5 bits are character.
 *
 * 101 reserved / unused
 *
 * 0 is dit, 1 is dah
 */

static const uint8_t morse_table[] = {
	/* **** ASCII, '(' to '@' **** (0..24) vs 40..64 */
	0B10010110,
	/*   -.--.	( */
	0B11101101,
	/*  -.--.-	) */
	0B11011010,
	/*  .--.-.	@ (should be *) */
	0B10001010,
	/*   .-.-.	+ */
	0B11110011,
	/*  --..--	, */
	0B11100001,
	/*  -....-	- */
	0B11010101,
	/*  .-.-.-	. */
	0B10010010,
	/*   -..-.	/ */
	0B10011111,
	/*   -----	0 */
	0B10001111,
	/*   .----	1 */
	0B10000111,
	/*   ..---	2 */
	0B10000011,
	/*   ...--	3 */
	0B10000001,
	/*   ....-	4 */
	0B10000000,
	/*   .....	5 */
	0B10010000,
	/*   -....	6 */
	0B10011000,
	/*   --...	7 */
	0B10011100,
	/*   ---..	8 */
	0B10011110,
	/*   ----.	9 */
	0B11111000,
	/*  ---...	: */
	0B11101010,
	/*  -.-.-.	; */
	0B10010110,
	/*   -.--.	[ (should be <) */
	0B10010001,
	/*   -...-	= */
	0B11101101,
	/*  -.--.-	] (should be >) */
	0B11001100,
	/*  ..--..	? */
	0B11011010,
	/*  .--.-.	@ (should be *) */

	/*   **** ASCII, A to Z **** (25..50) vs 65..90 and 97..122 */
	0B00101000,
	/*   .-		A */
	0B01110000,
	/*   -...	B */
	0B01110100,
	/*   -.-.	C */
	0B01010000,
	/*   -..	D */
	0B00000000,
	/*   .		E */
	0B01100100,
	/*   ..-.	F */
	0B01011000,
	/*   --.	G */
	0B01100000,
	/*   ....	H */
	0B00100000,
	/*   ..	I */
	0B01101110,
	/*   .---	J */
	0B01010100,
	/*   -.-	K */
	0B01101000,
	/*   .-..	L */
	0B00111000,
	/*   --		M */
	0B00110000,
	/*   -.		N */
	0B01011100,
	/*   ---	O */
	0B01101100,
	/*   .--.	P */
	0B01111010,
	/*   --.-	Q */
	0B01001000,
	/*   .-.	R */
	0B01000000,
	/*   ...	S */
	0B00010000,
	/*   -		T */
	0B01000100,
	/*   ..-	U */
	0B01100010,
	/*   ...-	V SHORT SHORT SHORT LONG */
	0B01001100,
	/*   .--	W */
	0B01110010,
	/*   -..-	X */
	0B01110110,
	/*   -.--	Y */
	0B01111000,
	/*   --..	Z */
};

static uint8_t morse_expand(uint8_t raw, uint8_t *len)
{
	/* Special 6 punctuation signifier */
	if ((raw & 0B11000000) == 0B11000000) {
		*len = 6;
		return raw << 2;
	}

	*len = (raw >> 5) + 1;

	return raw << 3;
}

static uint8_t morse_lookup(char c, uint8_t *len)
{
	*len = 0;

	if (c < 40) {
		/* Space is included in here, in which case we return a 0 len,
		 * and there's a total of 6 time units of silence */
		return 0;
	}

	if (c <= 90) { /* 40..90 */
		return morse_expand(morse_table[c - 40], len);
	}

	if (c < 97) { /* 91..96 */
		return 0;
	}

	if (c < 122) { /* 97..122 */
		return morse_expand(morse_table[c - 97 + 25], len);
	}

	return 0;
}

enum __attribute__ ((__packed__)) character_state {
	STATE_NEXTCHAR = 0,     // 1 time of nil, prime next character -> NEXTSYM/SPACEWAIT
	STATE_SPACEWAIT,        // 1 time of nil -> STATE_SPACEWAIT_B
	STATE_SPACEWAIT_B,      // 1 time of nil -> STATE_SPACEWAIT_C
	STATE_SPACEWAIT_C,      // 1 time of nil -> STATE_NEXTCHAR
	STATE_NEXTSYM,          // Prime next symbol, 1 time of nil -> NEXTCHAR/DOT/DASH
	STATE_DOT,              // Send a dot -> NEXTSYM
	STATE_DASH,             // Send a dash -> DASH_B
	STATE_DASH_B,           // Send a dash -> DASH_C
	STATE_DASH_C            // Send a dash -> NEXTSYM
};

union morsel_packed_state {
	uint32_t raw_state;
	struct {
		uint8_t len;
		uint8_t data;
		enum character_state char_state;
	} fields;
};

#define DONT_BUILD_IF(COND,MSG) typedef char static_assertion_##MSG[(COND)?-1:1]

DONT_BUILD_IF(sizeof(union morsel_packed_state) > sizeof(uint32_t),
		packedStateRep);

/* Returns 0 for off, 1 for on, -1 for completed */
int morse_send(const char **c, uint32_t *state)
{
	union morsel_packed_state *m_s = (void *) state;

	switch (m_s->fields.char_state) {
	case STATE_NEXTCHAR:
		if (!**c) {
			return -1;
		}

		m_s->fields.data = morse_lookup(**c, &m_s->fields.len);
		(*c)++;

		if (m_s->fields.len) {
			m_s->fields.char_state = STATE_NEXTSYM;
		} else {
			m_s->fields.char_state = STATE_SPACEWAIT;
		}

		return 0;

	case STATE_SPACEWAIT:
		m_s->fields.char_state = STATE_SPACEWAIT_B;

		return 0;

	case STATE_SPACEWAIT_B:
		m_s->fields.char_state = STATE_SPACEWAIT_C;

		return 0;

	case STATE_SPACEWAIT_C:
		m_s->fields.char_state = STATE_NEXTCHAR;

		return 0;

	case STATE_NEXTSYM:
		if (!m_s->fields.len) {
			m_s->fields.char_state = STATE_NEXTCHAR;

			return 0;
		}

		if (m_s->fields.data & 0B10000000) {
			m_s->fields.char_state = STATE_DASH;
		} else {
			m_s->fields.char_state = STATE_DOT;
		}

		m_s->fields.data <<= 1;

		m_s->fields.len--;

		return 0;

	case STATE_DASH:
		m_s->fields.char_state = STATE_DASH_B;
		return 1;

	case STATE_DASH_B:
		m_s->fields.char_state = STATE_DASH_C;
		return 1;

	case STATE_DOT:
	case STATE_DASH_C:
		m_s->fields.char_state = STATE_NEXTSYM;
		return 1;

	default:                        /* illegal state */
		return 1;
	}
}
