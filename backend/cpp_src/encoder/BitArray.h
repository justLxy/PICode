/*Author: ZHOU Baojian 
**Created: 2013/08/08
**Last-Modified: 2013/08/08
**Remark: Just for temporary use
*/

#ifndef _BIT_ARRAY_H
#define _BIT_ARRAY_H

class BitArray
{
public:
	BitArray(void);
	~BitArray(void);
private:
	int length; // Current bit array length (length that being used)
	int size; // Bit array size
	int incrementSize; // Incremental size when capacity is not enough
	char * bitArray; // Bit array storing data
private:
	bool IncreaseSize();// Increase the size of the array
public:
	int GetLength(); // Get the current length used
	bool AppendBits(int data, int numBits);// Append new bits to the end of the used part of array
	bool Set(int idx, int data); // Set the value of the idx element in the bitArray
	// Set the value of numBits consecutive elements in the bitArray, starting from the idx element
	bool Set(int idx, int data, int numBits);
	char Get(int idx); // Get value of the idx element
	bool Get(char* bitArray); // Copy the values in this->bitArray to bitArray
	bool GetBytes(unsigned char * byteArray, int byteArrayLength); // Convert to byte array and copy to byteArray
};

#endif
