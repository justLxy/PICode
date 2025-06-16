
#include "BitArray.h"
#include <string.h>
#include <stdlib.h>
//#include <android/log.h>

BitArray::BitArray(void)
{
	length = 0;
	size = 1024;
	incrementSize = 1024;
	bitArray = (char*)malloc(sizeof(char)*size);
}


BitArray::~BitArray(void)
{
	delete bitArray;
}

// Increase the size of the array
bool BitArray::IncreaseSize()
{
	// Old bitArray memory will be deleted by realloc
	bitArray = (char*)realloc(bitArray, sizeof(char)*(size+incrementSize));

	if(bitArray == NULL) {return false;}
	else{return true;}
}

// Get the length
int BitArray::GetLength()
{
	return length;
}

/*
Function: AppendBits(int data, int numBits)
Discription: Append new bits to the end of the used part of array
Input:	data - int, to be converted to bits
			numBits - numbers of bits to be added; consider from LSB of data
Output:
Return: true if success; else false
*/
bool BitArray::AppendBits(int data, int numBits)
{
	// numBits < sizeof(int)*8 is not checked
	// Check if capacity is still enough
	if(size < numBits + length){IncreaseSize();}

	// Convert and store data
	// Attention: MSB first or LSB first? Control idxBit to control the order
	// LSB first: for(int idxBit = 0; idxBit < numBits; idxBit++)
	// MSB first: for(int idxBit = numBits - 1; idxBit >= 0; idxBit--)
	for(int idxBit = numBits - 1; idxBit >= 0; idxBit--)
	{
		bitArray[length] = (data>>idxBit) % 2;
		length++;
	}// for idxBit

	return true;
}

// Set the value of the idx element in the bitArray
bool BitArray::Set(int idx, int data) 
{
	// Attention: idx is not checked
	bitArray[idx] = data;

	return true;
}

// Set the value of numBits consecutive elements in the bitArray, starting from the idx element
bool BitArray::Set(int idx, int data, int numBits) 
{
	// Attention: idx is not checked
	// Convert and store data
	// Attention: MSB first or LSB first? Control idxBit to control the order
	for(int idxBit = numBits - 1; idxBit >= 0; idxBit--)
	{
		bitArray[idx] = (data>>idxBit) % 2;
		idx++;
	}// for idxBit

	return true;
}

// Copy the values in this->bitArray to bitArray
bool BitArray::Get(char* bitArray) 
{
	// Check length of bitArray
	if(sizeof(bitArray) / sizeof(bitArray[0]) < length) {return false;}

	// Copy data
	for(int idx = 0; idx < length; idx++)
	{
		bitArray[idx] = this->bitArray[idx];
	}
	
	return true;
}

// Convert to byte array and copy to byteArray
// Padding bit-0s at the end
bool BitArray::GetBytes(unsigned char * byteArray, int byteArrayLength)
{
	int lengthByte = (length + 7) >> 3;
	if(byteArrayLength < lengthByte) {
		return false;
	} // Check storage requirement
	memset(byteArray, 0, sizeof(unsigned char) * byteArrayLength); // Important

	for(int idxByte = 0; idxByte < lengthByte; idxByte++)
	{
		for(int idxBit = 0; idxBit < 8; idxBit++)
		{
			int idx = (idxByte * 8 + idxBit);
			if(idx < length)
			{
				byteArray[idxByte] += (unsigned char)(bitArray[idx] << (7 - idxBit));
			}
		}// for idxBit
	}// for idxByte
	
	return true;
}// fxn

// Get value of the idx element
char BitArray::Get(int idx)
{
	return bitArray[idx];
}
