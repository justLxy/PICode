#include "BitMatrix.h"
#include "BitArray.h"
#include "Counted.h"
#include "ReedSolomonDecoder.h"
#include "Exception.h"
#include "zxdecoderlib.h"
#include "zxblocks.h"
#include <sstream>
#include "bch.h"

using namespace std;
using zxing::BitMatrix;
	using zxing::BitArray;
	using zxing::ReedSolomonDecoder;
	using zxing::GenericGF;
	using zxing::ArrayRef;
	using zxing::Exception;
//The decode function that perform the whole decode process
string decode(unsigned char** bitmap, zxing::BitMatrix* bma, int bmsize, int dimension, int modules)
{

	//Define constant
//	int	modules = 29;


	int transitLength = modules*2 + 1;
	int NumOfRecord = transitLength + 4;

	//Define namespace
	
	

	//Start of decoding (Basically the same implementation as in Java version)

	//Examine border
	int* BorderRecorderX = examineBorder(true, transitLength, dimension, modules, bma, bmsize, bmsize);
	int* BorderRecorderY = examineBorder(false, transitLength, dimension, modules, bma, bmsize, bmsize);

	//Terminate if null border is returned
	if(BorderRecorderX == 0 || BorderRecorderY == 0)
		{
		throw string("ERROR");}

	//Create blocks object
	Blocks* block = new Blocks(bitmap, modules, bmsize);

	//Count header of barcode

    int usablebits=modules*modules-53;
    int numofBytes =usablebits/8;
    int numofCodewords=ceil(double(numofBytes)/255);


	int numDataCw = countHeader(block, BorderRecorderX, BorderRecorderY);
	if(numDataCw <= 0)
	{throw string("ERROR");}

    int* informationbytes = new int[numDataCw];

    int TotalDataLength=numofBytes*8;

	if(TotalDataLength == 0)
		throw string("ERROR");


	int totalByte = TotalDataLength >> 3;

	block->totalByte = totalByte;
	block->erasureLimit = ((totalByte - (numDataCw)) >> 1) - 2;
	//Decode each block
	BitArray FrankResultBa(TotalDataLength);
	int* FrankResultByte=new int [totalByte];

	int row_num=0;
	int col_num=0;
	for(int i = 1; i <= 53;i++)
		{


			if (i % 2==1)
				row_num += 2;
			else
			{
				row_num = row_num - 2;
				col_num += 2;
			}
		}
	for(int i=1+53;i<=TotalDataLength+53;i++)
	{
		if(block->DecodeBlock(BorderRecorderX[col_num], BorderRecorderX[col_num+1], BorderRecorderY[row_num], BorderRecorderY[row_num+1], 999))
			FrankResultBa.set(i-1-53);

		if(i<=modules*(modules-1))
		{
			if (i % (2*modules) ==0)
				row_num += 2;

			if (i % (4*modules) <= 2*modules-1 && i % (4*modules) >0)
			{
				if (i % 2==1)
					row_num += 2;
				else
				{
					row_num =row_num - 2;
					col_num += 2;
				}
			}

			if (i % (4*modules) >=2*modules+1)
			{
				if (i % 2==1)
					row_num += 2;
				else
				{
					row_num =row_num - 2;
					col_num =col_num - 2;
				}
			}
		}
		else
		{	col_num +=2;}
	}

	//Get raw decode result
	for (int i=0;i<totalByte;i++)
	{
		int k=0;

		if (FrankResultBa.get(i*8+0))
			k=k+ 128;
		if (FrankResultBa.get(i*8+1))
			k=k+ 64;
		if (FrankResultBa.get(i*8+2))
			k=k+ 32;
		if (FrankResultBa.get(i*8+3))
			k=k+ 16;
		if (FrankResultBa.get(i*8+4))
			k=k+ 8;
		if (FrankResultBa.get(i*8+5))
			k=k+ 4;
		if (FrankResultBa.get(i*8+6))
			k=k+ 2;
		if (FrankResultBa.get(i*8+7))
			k=k+ 1;

		FrankResultByte[i] =(int) k;
	}


	//Error correction with RS decoder
	ReedSolomonDecoder rsDecoder(GenericGF::QR_CODE_FIELD_256);

	//Convert int[] to ArraRef<int> for compatibility with C++ Zxing RS decoder
	int currentdatabyte=0;
    int currenttotalbyte=0;

	for (int m=0;m<numofCodewords && currentdatabyte<numDataCw;m++)
	{
	int temptotalbytes;
    if(m<numofCodewords-1)
    {temptotalbytes=255;}
    else
    {temptotalbytes=totalByte-m*255;}


    int tempinforbytes=ceil(double(temptotalbytes)*double(numDataCw)/double(totalByte));
    if(tempinforbytes<3)
    {
    	tempinforbytes=3;
    }
    if ((temptotalbytes-tempinforbytes)%2==1)
    {   if(m<numofCodewords-1)
    	tempinforbytes++;
        else if(m==numofCodewords-1)
        temptotalbytes--;
    }

   /* if(m=numofCodewords-1)
    {
     tempinforbytes=numDataCw-currentdatabyte;
     if (tempinforbytes%2 !=1)
     {temptotalbytes--;}

    }*/

	ArrayRef<int> FrankResultAR(temptotalbytes);
	for (int i=0; i<temptotalbytes; i++)
		FrankResultAR[i]=FrankResultByte[i+currenttotalbyte];

	try {
		rsDecoder.decode(FrankResultAR, temptotalbytes-tempinforbytes);
	}
	catch (...)
	{
		throw string("ERROR");
	}

	for(int j=0;j<tempinforbytes;j++)
		informationbytes[currentdatabyte+j]=FrankResultAR[j];



    currentdatabyte= currentdatabyte +tempinforbytes;
    currenttotalbyte= currenttotalbyte +temptotalbytes;
	}


	//Retrieve coding bits
	int* codingbits=new int[numDataCw*8];

	for (int tmp=0; tmp < numDataCw; tmp++)
		 for (int tmp1=7;tmp1>=0;tmp1--)
			 codingbits[tmp*8+tmp1]=(informationbytes[tmp] >>(7-tmp1))%2;


	//Get the message inside
	int bitspos=0;
	string msg="";

	while(bitspos<=8*numDataCw-7)
	{
		int sevenbits=0;

		for (int i=bitspos; i<bitspos+7;i++)
			if (codingbits[i]==1)
				sevenbits += pow(2.0,bitspos+6-i);

		bitspos=bitspos+7;

		if (sevenbits < 95)
			msg=msg+ (char) (sevenbits+32);
		else if (sevenbits ==95)
			msg=msg+"ust.hk";
		else if (sevenbits ==96)
			msg=msg+"www.";
		else if (sevenbits ==97)
			msg=msg+".com";
		else if (sevenbits ==98)
			msg=msg+".org";
		else if (sevenbits ==99)
			msg=msg+"http://";
		else if (sevenbits ==100)
			msg=msg+"https://";
		else if (sevenbits ==101)
			msg=msg+"facebook";
		else if (sevenbits ==102)
			msg=msg+"renren";
		else if (sevenbits ==103)
			msg=msg+"eewhmow";
		else if (sevenbits ==127)
			break;
		else
		{
			int numlength=sevenbits-101;

			if ((numlength/3)*10+bitspos >= numDataCw*8)
				throw string("ERROR");

			for (int tmp=1;tmp<=floor((double)numlength/(double)3);tmp++)
			{
				int tenbits=0;

				for (int i=bitspos; i<bitspos+10;i++)
					if (codingbits[i]==1)
						tenbits += pow(2.0,bitspos+9-i);

				bitspos=bitspos+10;

				if (tenbits>=100)
					msg=msg+toString(tenbits);
				else if (tenbits>=10)
				{
					msg=msg+"0";
					msg=msg+toString(tenbits);
				}
				else
				{
					msg=msg+"00";
					msg=msg+toString(tenbits);
				}
			}

			if (numlength %3==2)
			{
				if (7+bitspos >= numDataCw*8)
					throw string("ERROR");

				int tenbits=0;
				for (int i=bitspos; i<bitspos+7;i++)
					if (codingbits[i]==1)
						tenbits += pow(2.0,bitspos+6-i);

				bitspos=bitspos+7;

				if (tenbits>=10)
					msg=msg+toString(tenbits);
				else
				{
					msg=msg+"0";
					msg=msg+toString(tenbits);
				}
			}
			else if (numlength%3==1)
			{
				if (4+bitspos >= numDataCw*8)
					throw string("ERROR");

				int tenbits=0;
				for (int i=bitspos; i<bitspos+4;i++)
					if (codingbits[i]==1)
						tenbits += pow(2.0,bitspos+3-i);
				bitspos=bitspos+4;
				msg=msg+toString(tenbits);
			}
		}
	}

	msg = msg + '\0';

	//Memory release
	delete block;
	delete[] BorderRecorderX;
	delete[] BorderRecorderY;


	//Return result
	return msg;
}

//Refer to Java version for details
int* examineBorder(bool Scanway, int transitLength, int dimension, int modules, BitMatrix* BinImg, int imgWidth, int imgHeight)
{
	bool way = Scanway;
	int NumOfRecord = transitLength + 4;	//Maximum no. of Record Data per axis
	int EndOfData = transitLength - 1;
	int* Border = new int[NumOfRecord];
	int CheckLimit = dimension - 1;		//Limit for main axis to check
	int CheckRange = (dimension / modules) * 3;	//Range for re-check the border(No range limit? ArrayOutOfIndexBoundary!)
	int x, y;
	bool cur_color, old_color;

	for(int i=0;i<NumOfRecord;i++)
		Border[i] = 0;

	if(way)
	{	//true, Scan X transition
		x = 1;	y = 1;
		cur_color = true;	old_color = true;

		while(BinImg->get(x, y) != cur_color)
		{
			x += 1;
			if (x >= (imgWidth >> 1))
				return 0;
		}
		while(Border[EndOfData] == 0)
		{
			for(int i=0;i<NumOfRecord;i+=2)
			{
				while(x < CheckLimit)
				{
					cur_color = BinImg->get(x, y);
					if(cur_color != old_color)
					{
						old_color = cur_color;

						if(i != 0)
						{
							Border[i] = x;
							Border[i - 1] = x - 1;
						}
						else
							Border[i] = x;

						break;
					}
					else
						x += 1;
				}
			}

			//check any wrong detection
			if(Border[EndOfData] == 0 || Border[EndOfData+1] != 0)
			{	//error presence
				x = 1;	y += 1;	cur_color = true;	old_color = true;

				delete[] Border;
				Border = new int[NumOfRecord];

				for(int i=0;i<NumOfRecord;i++)
					Border[i] = 0;

				if(y > CheckRange)	//Probably the Source BitMatrix is shifted...
					return 0;
			}
		}
	}
	else
	{	//false, Scan Y transition
		x = dimension - 1;	y = 1;
		cur_color = true;	old_color = true;

		while(BinImg->get(x, y) != cur_color)
		{
			y += 1;
			if(y >= (imgHeight >> 1))
					return 0;
		}
		while(Border[EndOfData] == 0)
		{
			for(int i=0;i<NumOfRecord;i+=2)
			{
				while(y < CheckLimit)
				{
					cur_color = BinImg->get(x, y);

					if(cur_color != old_color)
					{
						old_color = cur_color;

						if(i != 0)
						{
							Border[i] = y;
							Border[i - 1] = y - 1;
						}
						else
							Border[i] = y;

						break;
					}
					else
						y += 1;
				}
			}

			//check any wrong detection
			if(Border[EndOfData] == 0 || Border[EndOfData+1] != 0)
			{	//error presence
				x -= 1;	y = 1;
				cur_color = true;	old_color = true;

				delete[] Border;
				Border = new int[NumOfRecord];

				for(int i=0;i<NumOfRecord;i++)
					Border[i] = 0;

				if(x < dimension - CheckRange)	//Probably the Source BitMatrix is shifted...
					return 0;
			}
		}
	}

	return Border;
}

//Refer to Java version for details
int countHeader (Blocks* block, int* BorderRecorderX, int* BorderRecorderY)
{
	int version = 0;
	int numDataCw = 0;
	int row_num=0;
	int col_num=0;


	/*for(int i = 1; i <= 3;i++)
	{
		if(block->DecodeBlock(BorderRecorderX[col_num], BorderRecorderX[col_num+1], BorderRecorderY[row_num], BorderRecorderY[row_num+1], 999))
			version |= (1 << (3-i));

		if (i % 2==1)
			row_num += 2;
		else
		{
			row_num = row_num - 2;
			col_num += 2;
		}
	}

	for(int i = 4; i <= 14;i++)
		{
			if(block->DecodeBlock(BorderRecorderX[col_num], BorderRecorderX[col_num+1], BorderRecorderY[row_num], BorderRecorderY[row_num+1], 999))
				numDataCw |= (1 << (11-(i-3)));

			if (i % 2==1)
				row_num += 2;
			else
			{
				row_num = row_num - 2;
				col_num += 2;
			}
		}*/
	int headerdata[53];
	for(int i = 1; i <= 53;i++)
		{
			if(block->DecodeBlock(BorderRecorderX[col_num], BorderRecorderX[col_num+1], BorderRecorderY[row_num], BorderRecorderY[row_num+1], 999))
				headerdata[i-1] = 1;
			else
				headerdata[i-1] = 0;

			if (i % 2==1)
				row_num += 2;
			else
			{
				row_num = row_num - 2;
				col_num += 2;
			}
		}

	int bchcodeword[63];
	for (int i=0;i<53;i++)
	{
        bchcodeword[i]=headerdata[52-i];
	}
    for (int i=53;i<63;i++)
    {
    	bchcodeword[i]=0;
    }

    numDataCw=decodebch(bchcodeword);

	return numDataCw;
}

//Refer to Java version for details





//A helper function to turn number into string
string toString (int number)
{
	ostringstream ss;
	ss << number;
	return ss.str();
}



