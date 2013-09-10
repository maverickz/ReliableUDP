#include <netdb.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <math.h>
#define	CRC_LEN		16

int polynomial[17]={1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1}; /*  Polynomial - X^16 + X^2 + 1  */
void extractBinary(char charArray[],int charArraySize,int binaryArray[], int paddingArray[]);
void decimalToBinary(int decimalNumber, int binaryArray[]);
int positionOfOne(int array[], int arrayLength);
int binaryToDec(int array[],int crcLength);


int update_crc(char *inputBuffer, int inputBufferSize)
{
	FILE *fp1;
	int *input;//8016 + 160
	int *remainder;
	int *padding;
	int i,j;
	int position;
	int bufferSize = inputBufferSize;
	int tcpHeaderSize = sizeof (struct tcpheader);
	int inputLength;
	int crc;

	inputLength = bufferSize*8 + tcpHeaderSize*8;
	input = (int *)malloc((inputLength + CRC_LEN)*sizeof(int));
	remainder = (int *)malloc(CRC_LEN * sizeof(int));
	padding = (int *)calloc(CRC_LEN, sizeof(int));
	
	extractBinary(inputBuffer,inputBufferSize,input,padding);
	
	position = positionOfOne(input,inputLength+CRC_LEN);
	while(position >=0 && position < inputLength) {
		for(i=position,j=0; i<position+CRC_LEN+1; i++,j++) {
			input[i] = input[i] ^ polynomial[j];
		}
		position = positionOfOne(input,inputLength+CRC_LEN);
	}
	
	for(i=inputLength, j=0; i<inputLength+CRC_LEN,j<CRC_LEN; i++,j++) {
		remainder[j] = input[i];
		//printf("%d ",input[i]);
	}
	crc = binaryToDec(remainder,CRC_LEN);
	return crc;
}


int positionOfOne(int array[], int arrayLength)
{
	int i;
	for(i = 0; i<arrayLength; i++) {
		if(array[i] == 1)
		return i;
	}
	return -1;
}

void extractBinary(char charArray[], int charArraySize, int binaryArray[], int paddingArray[])
{
	int i=0;
	int j=0;
	char charAti;
	for(i=0;i<charArraySize;i++)
	{		
		charAti=charArray[i];
		binaryArray[j+0]=(charAti&0x80)?1:0;
		binaryArray[j+1]=(charAti&0x40)?1:0;
		binaryArray[j+2]=(charAti&0x20)?1:0;
		binaryArray[j+3]=(charAti&0x10)?1:0;
		binaryArray[j+4]=(charAti&0x08)?1:0;
		binaryArray[j+5]=(charAti&0x04)?1:0;
		binaryArray[j+6]=(charAti&0x02)?1:0;
		binaryArray[j+7]=(charAti&0x01)?1:0;	
		j+=8;	
	}
	printf("Val of j: %d\n",j);
	for(i=0;i<CRC_LEN;i++)
	{
		binaryArray[charArraySize*8+i] = paddingArray[i];
	}	
}


int binaryToDec(int array[],int crcLength)
{
	int i,j;
	int returnVal = 0;	
	for(i=0,j=crcLength-1;i < crcLength;i++,j--)
	{
		returnVal += array[i]*pow(2,j);
	}
	return returnVal;
}

