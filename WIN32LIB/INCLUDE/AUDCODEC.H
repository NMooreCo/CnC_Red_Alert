/*
**	Command & Conquer Red Alert(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/***********************************************************************************************
 *                 Project Name : Westwood 32 bit Library                                      *
 *                    File Name : AUDCODEC.H                                                   *
 *                                                                                             *
 *   C++ versions of the audio decompressors that used to be assembly:                         *
 *                                                                                             *
 *     SOS_Decompress   -- the HMI "SOS" 4:1 IMA ADPCM decoder (SOSCODEC.ASM, OLSOSDEC.ASM     *
 *                         and the VQA player's copy). Used for .AUD files and VQA movies with *
 *                         compression type 99.                                                *
 *     WW_Audio_Unzap   -- Westwood's own 2/4 bit delta decoder (AUDUNCMP.ASM's               *
 *                         Decompress_Frame and the VQA player's AudioUnzap).                   *
 *                                                                                             *
 *   The SOS decoder is a template because the library and the VQA player each declare their   *
 *   own _SOS_COMPRESS_INFO structure, with the same members in a different order.            *
 *---------------------------------------------------------------------------------------------*/

#ifndef AUDCODEC_H
#define AUDCODEC_H

/*
**	Step index adjustment per 4 bit code, and the step table (standard IMA ADPCM).
*/
static const short SOSIndexTable[16] = {
	-1, -1, -1, -1, 2, 4, 6, 8,
	-1, -1, -1, -1, 2, 4, 6, 8
};

static const unsigned short SOSStepTable[89] = {
	7,     8,     9,     10,    11,    12,    13,    14,    16,    17,
	19,    21,    23,    25,    28,    31,    34,    37,    41,    45,
	50,    55,    60,    66,    73,    80,    88,    97,    107,   118,
	130,   143,   157,   173,   190,   209,   230,   253,   279,   307,
	337,   371,   408,   449,   494,   544,   598,   658,   724,   796,
	876,   963,   1060,  1166,  1282,  1411,  1552,  1707,  1878,  2066,
	2272,  2499,  2749,  3024,  3327,  3660,  4026,  4428,  4871,  5358,
	5894,  6484,  7132,  7845,  8630,  9493,  10442, 11487, 12635, 13899,
	15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};


/***********************************************************************************************
 * SOS_Decode_Nibble -- Decodes one 4 bit ADPCM code, updating the channel state.              *
 *=============================================================================================*/
static inline int SOS_Decode_Nibble(int code, int & predicted, short & index, short & step)
{
	int s = (unsigned short)step;
	int diff = s >> 3;
	if (code & 4) diff += s;
	if (code & 2) diff += s >> 1;
	if (code & 1) diff += s >> 2;
	if (code & 8) diff = -diff;

	int sample = predicted + diff;
	if (sample > 32767) sample = 32767;
	if (sample < -32768) sample = -32768;
	predicted = sample;

	int i = index + SOSIndexTable[code];
	if (i < 0) i = 0;
	if (i > 88) i = 88;
	index = (short)i;
	step = (short)SOSStepTable[i];
	return(sample);
}


/***********************************************************************************************
 * SOS_Init_Stream -- Prepares a compression info structure for a new stream.                  *
 *=============================================================================================*/
template<class INFO>
static inline void SOS_Init_Stream(INFO * info)
{
	info->wIndex = 0;
	info->wStep = 7;
	info->dwPredicted = 0;
	info->dwSampleIndex = 0;
	info->wIndex2 = 0;
	info->wStep2 = 7;
	info->dwPredicted2 = 0;
	info->dwSampleIndex2 = 0;
}


/***********************************************************************************************
 * SOS_Decompress -- Decompresses ADPCM data.                                                  *
 *                                                                                             *
 *    The source is info->lpSource, the output goes to info->lpDest. "bytes" is the number of   *
 *    bytes to produce. 16 bit or 8 bit (unsigned) samples, mono or stereo, are produced       *
 *    according to info->wBitSize and info->wChannels. Stereo data has the channels in         *
 *    alternate source bytes, each byte holding two samples of its channel (low nibble first). *
 *    Each call starts with the low nibble of a source byte, as the original did.             *
 *                                                                                             *
 * OUTPUT:  Returns with the number of bytes requested.                                        *
 *=============================================================================================*/
template<class INFO>
static inline unsigned int SOS_Decompress(INFO * info, unsigned int bytes)
{
	unsigned char const * source = (unsigned char const *)info->lpSource;
	unsigned char * dest = (unsigned char *)info->lpDest;
	bool sixteen = (info->wBitSize == 16);
	int channels = (info->wChannels == 2) ? 2 : 1;
	unsigned int samples = sixteen ? (bytes >> 1) : bytes;		// Total, all channels.

	for (int channel = 0; channel < channels; channel++) {
		int & predicted = channel ? info->dwPredicted2 : info->dwPredicted;
		short & index = channel ? info->wIndex2 : info->wIndex;
		short & step = channel ? info->wStep2 : info->wStep;
		unsigned char const * src = source + channel;
		unsigned int count = samples / channels;

		for (unsigned int n = 0; n < count; n++) {
			int token = src[(n >> 1) * channels];
			int code = (n & 1) ? (token >> 4) : (token & 0x0F);
			int sample = SOS_Decode_Nibble(code, predicted, index, step);
			unsigned int out = n * channels + channel;
			if (sixteen) {
				dest[out * 2] = (unsigned char)(sample & 0xFF);
				dest[out * 2 + 1] = (unsigned char)((sample >> 8) & 0xFF);
			} else {
				dest[out] = (unsigned char)(((sample >> 8) & 0xFF) ^ 0x80);
			}
		}
	}
	info->dwSampleIndex = 0;
	info->dwSampleIndex2 = 0;
	return(bytes);
}


/***********************************************************************************************
 * WW_Audio_Unzap -- Decompresses a Westwood compressed (2/4 bit delta) audio block.           *
 *                                                                                             *
 *    Output is 8 bit unsigned samples. Each code byte holds a 2 bit command in the top bits:  *
 *    0 = 2 bit deltas, 1 = 4 bit deltas (count+1 bytes follow), 2 = raw samples or a 5 bit   *
 *    signed delta, 3 = repeat the last sample count+1 times.                                 *
 *                                                                                             *
 * INPUT:   source   -- Pointer to the compressed data.                                        *
 *          dest     -- Pointer to the output buffer.                                          *
 *          size     -- Number of samples (bytes) to produce.                                  *
 *                                                                                             *
 * OUTPUT:  Returns with the number of source bytes consumed.                                  *
 *=============================================================================================*/
static inline int WW_Audio_Unzap(void const * source, void * dest, int size)
{
	static const signed char decode2[4] = {-2, -1, 0, 1};
	static const signed char decode4[16] = {-9, -8, -6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 8};

	unsigned char const * src = (unsigned char const *)source;
	unsigned char * dst = (unsigned char *)dest;
	int count = size;
	int sample = 0x80;

	if (source == NULL || dest == NULL || size == 0) return(0);

	while (count > 0) {
		int code = *src++;
		int arg = code & 0x3F;

		switch (code >> 6) {
			case 2:
				if (arg & 0x20) {
					/*
					**	A 5 bit signed delta (no clamping, as in the original).
					*/
					int delta = (arg & 0x1F) | ((arg & 0x10) ? ~0x1F : 0);
					sample = (sample + delta) & 0xFF;
					*dst++ = (unsigned char)sample;
					count--;
				} else {
					/*
					**	Raw samples follow.
					*/
					int n = arg + 1;
					memcpy(dst, src, n);
					src += n;
					dst += n;
					count -= n;
					sample = dst[-1];
				}
				break;

			case 1:
				for (int n = arg + 1; n > 0; n--) {
					int b = *src++;
					sample += decode4[b & 0x0F];
					if (sample < 0) sample = 0;
					if (sample > 255) sample = 255;
					*dst++ = (unsigned char)sample;
					sample += decode4[b >> 4];
					if (sample < 0) sample = 0;
					if (sample > 255) sample = 255;
					*dst++ = (unsigned char)sample;
					count -= 2;
				}
				break;

			case 0:
				for (int n = arg + 1; n > 0; n--) {
					int b = *src++;
					for (int shift = 0; shift < 8; shift += 2) {
						sample += decode2[(b >> shift) & 3];
						if (sample < 0) sample = 0;
						if (sample > 255) sample = 255;
						*dst++ = (unsigned char)sample;
					}
					count -= 4;
				}
				break;

			default:
				memset(dst, sample, arg + 1);
				dst += arg + 1;
				count -= arg + 1;
				break;
		}
	}
	return((int)(src - (unsigned char const *)source));
}

#endif
