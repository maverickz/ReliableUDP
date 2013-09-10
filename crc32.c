#include <stdio.h>                  
#include <stdlib.h>                 

#define POLYNOMIAL 0x04c11db7L      // Standard CRC-32 ppolynomial

static unsigned int crc_table[256]; // Table of 8-bit remainders

void crcTableInit(void);
unsigned int update_crc(unsigned char *data, unsigned int dataSize);

void crcTableInit(void)
{
	unsigned short int i, j;
	unsigned int crc_accum;

	for (i=0;  i<256;  i++)
	{
		crc_accum = ( (unsigned int) i << 24 );
		for ( j = 0;  j < 8;  j++ )
		{
			if ( crc_accum & 0x80000000L ){
				crc_accum = (crc_accum << 1) ^ POLYNOMIAL;
			}
			else {
				crc_accum = (crc_accum << 1);
			}
		}
		crc_table[i] = crc_accum;
	}
}


unsigned int update_crc(unsigned char *data, unsigned int dataSize)
{
	unsigned int i, j;
	unsigned int crc_accum = -1;

	for (j=0; j<dataSize; j++)
	{
		i = ((int) (crc_accum >> 24) ^ *data++) & 0xFF;
		crc_accum = (crc_accum << 8) ^ crc_table[i];
	}
	crc_accum = ~crc_accum;

	return crc_accum;
}


