#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <algorithm>
//#include <android/log.h>

#include "PiCodeEncoder.h"
#include "BitArray.h"
#include "reedsol.h"
#include "bchenc.h"
#include "otsuthresh.h"

//#include "CImg.h"
//using namespace cimg_library;
using namespace std;

PiCodeEncoder::PiCodeEncoder(void)
{
	version = 0;

	moduleSize = 4;
	moduleNumData = 29;
	moduleNumFP = 1;
	moduleNumQZ = 2;

	numStart = 104;
	numEnd = 126;
	charStart = 32;

	RSGFPoly = 285; // Primitive polynomial = D^8+D^4+D^3+D^2+1 (285 decimal)
	codeRateMax = 5.0f/6.0f;

	quality = 40.0f / 255;
}

PiCodeEncoder::~PiCodeEncoder(void)
{
}

/*
Interface function. Alan ZHOU 20140223
generate and save a picode to the current directory
Return:
0, successful; 1, failure caused by message being too short or too long; otherwise, other failures.
*/
int PiCodeEncoder::GeneratePiCode(char msg[], char filenameLogo[], char filenamePiCode[])
{
	// PiCode configuration.
	unsigned int WHITE = 255;
	unsigned int dataRegionSize = moduleSize * moduleNumData;
	unsigned int picodeSize = moduleSize * (2 * (moduleNumQZ + moduleNumFP) + moduleNumData);

	// Intialize output PiCode as all white.
	CImg<unsigned int> imgPiCode(picodeSize, picodeSize, 1, 3, WHITE);
	// debug: Be careful when handling unsigned int subtraction; remember to cast result to int.
	// imgPiCode.display("GeneratePiCode-imgPiCode", true); // debug

	// Load and resize input image.
	CImg<unsigned int> imgLogo;
	if (strlen(filenameLogo) == 0) {
		// If no logo is provided, create a blank white image.
		imgLogo.assign(dataRegionSize, dataRegionSize, 1, 3, WHITE);
	} else {
		imgLogo.load(filenameLogo);
		imgLogo.resize(dataRegionSize, dataRegionSize, 1, 3);
	}
	// imgLogo.display("GeneratePiCode-imgLogo", true); // debug

	// Do coding for input message.
	unsigned int moduleNumData2 = moduleNumData * moduleNumData;
	char *data = new char[moduleNumData2];// Remember to delete
	memset(data, 0, sizeof(char)*moduleNumData2); // Set all data to 0.
	int flag = Encode(msg, data);
	if(1 == flag){
		return 1; // Msg too long or too short
	}

	// Load logo and embed
	Embed(data, imgLogo, imgPiCode);

	// Save PiCode as an image file.
	imgPiCode.save(filenamePiCode);

	// Release resource and return.
	delete[] data;
	return 0; // Success
} //fxn

/*
Return:
0, successful; 1, failure caused by message being too short or too long; otherwise, other failures.
*/
int PiCodeEncoder::GeneratePiCode(char *msg, int *logoPixels, int *picodePixels)
{
	// Initialize PiCode pixels
	unsigned int picodeSize = moduleSize * (2 * (moduleNumQZ + moduleNumFP) + moduleNumData);
	memset(picodePixels, 255, sizeof(int) * picodeSize * picodeSize);  // Important: 255

	unsigned int moduleNumData2 = moduleNumData * moduleNumData;
	char *data = new char[moduleNumData2];// Remember to delete
	memset(data, 0, sizeof(char)*moduleNumData2); // Set all data to 0

	// Do coding for input message
	int flag = Encode(msg, data);
	if(1 == flag){
		return 1; // Msg too long or too short
	}

	// Load logo and embed
	Embed(data, logoPixels, picodePixels);

	delete[] data;
	return 0; // Success
}


/*
Description: Encode string input message into bits, with source coding, RS coding and interleaver.
Remark:
1. Default: data should be of size moduleNumData * moduleNumData = 29 * 29 = 841.
2. Each char in the char array, data, stores one bit.
*/
int PiCodeEncoder::Encode(char *msg, char *data)
{

	// Source coding
	unsigned char * msgBytes = NULL; // To be initialized; Remember to delete!!!
	unsigned int msgBytesLength = 0;
	SourceCoding(msg, msgBytes, msgBytesLength);

	// RS coding
	unsigned char *codeBytes = NULL; // To be initialized; Remember to delete!!!
	unsigned int codeBytesLength = 0;
	int flag = RSCoding(msgBytes, msgBytesLength, codeBytes, codeBytesLength);
	if(1 == flag){
		return 1; // msg too long or too short
	}

	AssembleData(msgBytesLength, codeBytes, codeBytesLength, data);
	// Interleave the coded bits
	Interleave(data);

	// Release resources
	delete[] msgBytes;
	delete[] codeBytes;

	return 0; // Success
}


// Using reference is crucial here
bool PiCodeEncoder::SourceCoding(char* msg, unsigned char * &msgBytes, unsigned int &msgBytesLength)
{
	unsigned int msgLength = strlen(msg);

	// Do source coding to the msg to get a bit stream/array...
	BitArray msgBits; // msgBits without header

	unsigned int idxChar = 0; // Index of current char position of message
	bool successFlag = false;// if one trial of encoding is successful;...
	// this can be used to help separate one function into several functions	
	for(idxChar = 0; idxChar < msgLength; ){
		successFlag = false;

		// 1. consider high frequency strings
		unsigned int idxHFString = 0;
		unsigned int hfStringLength = 0;
		for(idxHFString = 0; idxHFString < hfNum; idxHFString++){
			hfStringLength = strlen(hfStringArray[idxHFString]);
			if(msgLength - idxChar >= hfStringLength){ // msg not encoded yet longer than hfStringLength
				char msgHeaderString[100];
				memcpy(msgHeaderString, (msg + idxChar), sizeof(char)*hfStringLength);
				msgHeaderString[hfStringLength] = '\0';
				if( 0 == strcmp(msgHeaderString, hfStringArray[idxHFString]) ){
					msgBits.AppendBits((int)(hfByteArray[idxHFString]), 7); // code the hfString to 7 bits
					idxChar += hfStringLength; //rontex! important
					// Rontex! When successfully encoded, restart encoding;
					// Remember this is of the highest priority!
					successFlag = true; //restart encoding
				}//if equal; if not equal, do nothing
			}//if leftMsgLength
		}//for idxHFString
		// Rontex! When successfully encoded, restart encoding;
		if(true == successFlag){
			continue;
		}

		// 2. consider successive numbers
		unsigned int idxCharTmp = idxChar; // Index where number characters end
		// Check length of successive numbers, max length 25 = (126-103) + 2 since 104 means 3 numbers
		// Special encoding is needed only for more than successive 3 numbers
		if(idxChar + 2 < msgLength){ // Only do this when there are at least 3 characters remaining uncoded
			for(idxCharTmp = idxChar; idxCharTmp < msgLength && (unsigned int)(idxCharTmp - idxChar) < (unsigned int)(numEnd - numStart) + 3; idxCharTmp++){
				if(msg[idxCharTmp] < '0' || msg[idxCharTmp] > '9'){
					break;
				}//if meet character that is not '0'-'9', or max length reached, break checking loop
			}//for idxCharTmp, idxCharTmp finally stops at the first non-number character or msgLength/overflow
		}//if idxChar + 2 < msgLength

		// Do special coding for successive numbers, if no less than 3 numbers found
		if(idxCharTmp - idxChar >= 3){	//no less than 3 numbers, then do special coding
			// Add number length indicator to the bit stream
			msgBits.AppendBits((idxCharTmp - idxChar) - 3 + numStart, 7);

			// Encode numbers
			while(idxChar < idxCharTmp){
				if(idxCharTmp - idxChar >= 3){
					// case 1): more than 3 numbers left, then encode the first 3 numbers into 10 bits
					msgBits.AppendBits( (msg[idxChar] - '0')*100 
						+ (msg[idxChar+1] - '0')*10
						+ (msg[idxChar+2] - '0'), 10);
					idxChar += 3;
				}//number length >= 3
				else if(idxCharTmp - idxChar == 2){
					// case 2): if 2 numbers left, encode them into 7 bits
					msgBits.AppendBits( (msg[idxChar] - '0')*10 
						+ (msg[idxChar+1] - '0'), 7);
					idxChar += 2;
				}//number length == 2
				else if(idxCharTmp - idxChar == 1){
					// case 3): if 1 number left, encode it into 4 bits
					msgBits.AppendBits((msg[idxChar] - '0'), 4);
					idxChar++;
				}//number length == 1
			}//while idxChar < idxCharTmp

			successFlag = true; //restart encoding		
		}//if idxCharTmp - idxChar >= 3
		// Rontex! When successfully encoded, restart encoding;
		if(true == successFlag){
			continue;
		}

		// 3. ordinary cases
		msgBits.AppendBits(msg[idxChar] - charStart, 7); // Support from space character, ' '
		idxChar++;
		//continue;

	}// for idxChar

	// Determine if msgBits terminator (7 bit-1s) is needed; terminator does not increase the msgBytesLength
	if(1 == msgBits.GetLength() % 8){
		msgBits.AppendBits(127, 7);
	}
	msgBytesLength = (msgBits.GetLength() + 7) >> 3; // Length of msgBits, in bytes

	// Convert bit stream to bytes (unsigned chars) for RS coding input
	msgBytes = new unsigned char[msgBytesLength]; // Remember to delete
	if(msgBytes != NULL){
		return msgBits.GetBytes(msgBytes, msgBytesLength);
	} else{
		return false;
	}
}// fxn SourceCoding()


// Using reference is crucial here
int PiCodeEncoder::RSCoding(unsigned char *msgBytes, unsigned int msgBytesLength, unsigned char * &codeBytes, unsigned int &codeBytesLength)
{
	// Do RS coding, in byte units, for the bit stream containing the header
	// Error correction capacity = (n - k)/2; thus (n - k) must be an even integer
	// codeBytesLength n at most 2^8-1 = 255
	unsigned int moduleNumRSCode2 = moduleNumData * moduleNumData - headerLength;
	codeBytesLength = moduleNumRSCode2 / 8; // Default: (29*29 - 53)/8 = 98, support n = 98 or 97; max 255 or 254
	unsigned int parityBytesLength = codeBytesLength - msgBytesLength;
	float codeRate = (float)msgBytesLength / codeBytesLength; // Ideal code rate

	// Check if message is too long or too short for the PiCode, i.e., code rate is too high or too low
	if(codeRate > codeRateMax || codeRate == 0 || parityBytesLength < 1)
	{
		return 1; // Message too long or too short
	}

	codeBytes = new unsigned char[codeBytesLength]; // Remember to delete
	// memset(codeBytes, 0, sizeof(unsigned char) * codeBytesLength);
	// Modified 20130909
	// Randomly set the codeBytes[] to [0, 255]
	srand((unsigned int)time(NULL));
	for(unsigned int idxCodeBytes = 0; idxCodeBytes < codeBytesLength; idxCodeBytes++)
	{
		codeBytes[idxCodeBytes] = rand() % 255;
	}
	unsigned char * parityBytes = new unsigned char[parityBytesLength];
	memset(parityBytes, 0, sizeof(unsigned char) * parityBytesLength);

	// Attention: multiple-codeword
	unsigned int codeLengthStep = 255; // n = 255 for all codewords except the last one (if there exists one)
	unsigned int codeNum = (codeBytesLength - 1) / codeLengthStep + 1; // Ceil; If barcode is too large, there can be more than 1 RS codeword
	unsigned int msgLengthStep = (int)(codeLengthStep * codeRate) + 1; // Ceil
	if(msgLengthStep<3)
		msgLengthStep=3;
	msgLengthStep = msgLengthStep + (1 - msgLengthStep % 2); // Make msgLengthStep odd to form a RS(255, msgLengthStep) code
	unsigned int parityLengthStep = codeLengthStep - msgLengthStep;

	// Temporary pointers
	unsigned char *codeBytesPos = codeBytes;
	unsigned char *msgBytesPos = msgBytes;
	unsigned char *parityBytesPos = parityBytes;
	rs_init_gf(RSGFPoly); // [8,4,3,2,1]=0x11D=285, same as default in MATLAB.
	for(unsigned int idxCode = 0; idxCode < codeNum; idxCode++)
	{	
		// Determine if the encoding is done
		if(msgBytesPos >= msgBytes + msgBytesLength)
		{
			break;
		}

		// Special setting for last code
		// The order of the following statements is important: first check msgBytesPos, then check codeBytesPos
		// Reason: codeLengthStep should be trimed according to msgLengthStep
		if(msgBytesPos + msgLengthStep > msgBytes + msgBytesLength)// remaining msg shorter than msgLengthStep
		{
			msgLengthStep = msgBytes + msgBytesLength - msgBytesPos;
			if(msgLengthStep<3)
				msgLengthStep=3;
		}
		if(codeBytesPos + codeLengthStep > codeBytes + codeBytesLength)// remaining codeword space shorter than codeLengthStep
		{
			codeLengthStep = codeBytes + codeBytesLength - codeBytesPos;
			codeLengthStep = codeLengthStep - (codeLengthStep - msgLengthStep) % 2; // Make sure (n - k) is even
		}
		parityLengthStep = codeLengthStep - msgLengthStep;

		// Normal RS coding process
		rs_init_code(parityLengthStep, 1); // Attention: pass (n - k) as 1st parameter
		rs_encode(msgLengthStep, msgBytesPos, parityBytesPos); // ParityBytes only contains the added symbols, in *INVERTED* order
		// Copy encoded data
		memcpy(codeBytesPos, msgBytesPos, msgLengthStep);
		for(unsigned int idxByte = 0; idxByte < parityLengthStep; idxByte++)
		{
			codeBytesPos[codeLengthStep - 1 - idxByte] = parityBytesPos[idxByte]; // Inversion and copy the parity bytes
		}

		// Update temporary pointer
		codeBytesPos += codeLengthStep;
		msgBytesPos += msgLengthStep;
		parityBytesPos += parityLengthStep;
	} // for idxCode

	delete[] parityBytes;
	return 0; // Success

} // fxn RSCoding()

bool PiCodeEncoder::AssembleData(unsigned int msgBytesLength, unsigned char *codeBytes, unsigned int codeBytesLength, char *data)
{
	// BCH coded version and RS message-length header[]
	// header[]: LSB-msgLength-version-truncated 0's-MSB; reversed when put into PiCode
	unsigned char header[headerBCHLength]; // Header array

	unsigned int idxHeader;
	unsigned int idxBit;
	// Convert msgLength to bits
	idxHeader = 0;
	for(idxBit = 0; idxBit < msgLength; idxBit++)
	{
		header[idxHeader] = (msgBytesLength>>idxBit) % 2;
		idxHeader++;
	}// for idxBit
	// Convert version to bits
	idxHeader = msgLength;
	for(idxBit = 0; idxBit < versionLength; idxBit++)
	{
		header[idxHeader] = (version>>idxBit) % 2;
		idxHeader++;
	}// for idxBit
	// Truncated BCH: set first 10 bits (MSB) of the uncoded header to 0's
	for(idxHeader = msgLength + versionLength; idxHeader < msgLength + versionLength + headerTruncatedLength; idxHeader++)
	{
		header[idxHeader] = 0;
	}

	// BCH coding
	bchenc(headerBCHm, headerBCHLength, headerBCHt, header); 

	// Copy BCH coded header to data array; Attention: the reverse order
	unsigned int idxData;
	idxHeader = msgLength + versionLength - 1;
	for(idxData = 0; idxData < msgLength + versionLength; idxData++)
	{
		data[idxData] = header[idxHeader];
		idxHeader--;
	}
	idxHeader = headerBCHLength - 1;
	for(idxData = msgLength + versionLength; idxData < headerLength; idxData++)
	{
		data[idxData] = header[idxHeader];
		idxHeader--;
	}

	// Convert the RS encoded data of bytes to bit array and store it in data array
	idxData = headerLength;
	for(unsigned int idxByte = 0; idxByte < codeBytesLength; idxByte++)
	{
		// MSB first
		for(idxBit = 7; (int)idxBit >= 0; idxBit--)
		{
			data[idxData] = (codeBytes[idxByte]>>idxBit) % 2;
			idxData++;
		}// for idxBit
	}

	// Modified 20130909
	// Randomly set the remaining data[] bits to 0 or 1
	srand((unsigned int)time(NULL));
	for(; idxData < moduleNumData * moduleNumData; idxData++)
	{
		data[idxData] = rand() % 2;
	}

	//debug
	//for(int idxData = 0; idxData < moduleNumData * moduleNumData; idxData++)
	//{
	//	if(data[idxData] !=0 && data[idxData] != 1)
	//		printf("data: %d  Location: %d", data[idxData], idxData);
	//}

	return true;
}// fxn AssembleData


// Use interleaver to re-arrange the bits
bool PiCodeEncoder::Interleave(char *data)
{
	// First, make a copy the original data
	unsigned int dataLength = moduleNumData * moduleNumData;
	char *dataOrig = new char[dataLength];
	memcpy(dataOrig, data, sizeof(char)*dataLength);

	unsigned int posX = 0;
	unsigned int posY = 0;
	unsigned int idxOld = 0;
	unsigned int idxNew = 0;
	for(idxOld = 0; idxOld < dataLength; idxOld++){
		idxNew = posY * moduleNumData + posX;
		data[idxNew] = dataOrig[idxOld];

		//Update next (posX, posY) for calculating idxNew
		if(idxOld >= (moduleNumData - 1) * moduleNumData){//last row
			if((idxOld / (moduleNumData * 2)) % 2 == 0){
				posX++;	
			}
			else{
				posX--;
			}
		}
		else if((idxOld + 1) % (moduleNumData * 2) == 0){//switch from a double-row to another double-row
			posY++;
		}
		else if((idxOld / (moduleNumData * 2)) % 2 == 0){//even double-row
			posX += (idxOld % 2);
			if(idxOld % 2 == 0){
				posY++;
			}
			else if(idxOld % 2 == 1){
				posY--;
			}
		}
		else if((idxOld / (moduleNumData * 2)) % 2 == 1){//odd double-row
			posX -= (idxOld % 2);
			if(idxOld % 2 == 0){
				posY++;
			}
			else if(idxOld % 2 == 1){
				posY--;
			}
		}
	}//for idxOld

	delete[]  dataOrig;
	return true;
}// fxn Interleave()


bool PiCodeEncoder::Embed(char *data, CImg<unsigned int> &imgLogo, CImg<unsigned int> &imgPiCode)
{
	// Convert input image from RGB to YUV.
	CImg<float> imgLogoYUV = imgLogo.get_RGBtoYUV();
	// imgLogoYUV.display("Embed:imgLogoYUV", true); // debug

	// Do modulation.
	float *logoPixelsY = imgLogoYUV.data(0, 0, 0, 0);
	EmbedData(data, logoPixelsY);

	// Convert modulated image from YUV back to RGB.
	imgLogo = imgLogoYUV.get_YUVtoRGB();
	// imgLogo.display("Embed:imgLogo", true); // debug

	// Attach finder pattern to and copy data region (modulated image) into PiCode.
	AttachFinderPattern(imgPiCode);
	FillDataRegion(imgLogo, imgPiCode);

	return true;
} // fxn Embed

/*
Description: Embed data into the logo to generate the PiCode.
Parameters:
data - pointer to char array of bits to be embedded; length moduleNumData * moduleNumData
logoPixels - pointer to int array of logo pixels
picodePixels - pointer to int array of PiCode pixels
Return:
true if success; otherwise, false.
*/
bool PiCodeEncoder::Embed(char *data, int *logoPixels, int *picodePixels)
{
	// Load int pixel values into CImg object.
	unsigned int dataRegionSize = moduleSize * moduleNumData; // i.e., logoPixels size along one dimension
	unsigned int numPixels = dataRegionSize * dataRegionSize;
	unsigned int *logoPixelsRGB = new unsigned int[3 * numPixels];
	unsigned int *r = logoPixelsRGB;
	unsigned int *g = r + numPixels;
	unsigned int *b = g + numPixels;
	for(unsigned int i = 0; i < numPixels; ++i)
	{
		r[i] = ((unsigned int)logoPixels[i] & 0x00ff0000) >> 16;
		g[i] = ((unsigned int)logoPixels[i] & 0x0000ff00) >> 8;
		b[i] = ((unsigned int)logoPixels[i] & 0x000000ff);
	}
	CImg<unsigned int> imgLogo(logoPixelsRGB, dataRegionSize, dataRegionSize, 1, 3);
	delete[] logoPixelsRGB;

	// Convert from RGB format to YUV format.
	CImg<float> imgLogoYUV = imgLogo.get_RGBtoYUV();

	// Get the Y component and do modulation.
	// Attention: data storage format
	float *logoPixelsY = imgLogoYUV.data(0, 0, 0, 0);

	// Lewis' adaptive modulation for the Y component.
	EmbedData(data, logoPixelsY);

	// Convert back to RGB format.
	imgLogo = imgLogoYUV.get_YUVtoRGB();

	// Convert RGB pixels into int and copy back into array.
	r = imgLogo.data(0, 0, 0, 0);
	g = imgLogo.data(0, 0, 0, 1);
	b = imgLogo.data(0, 0, 0, 2);
	for (unsigned int i = 0; i < numPixels; ++i)
	{
		logoPixels[i] = (int)(0xff000000 | r[i] << 16 | g[i] << 8 | b[i]);
	}

	// Attach finder pattern and copy the data region to PiCode pixelss.
	AttachFinderPattern(picodePixels);
	FillDataRegion(logoPixels, picodePixels);

	return true;
}// fxn

/*
Description: Embed data into the Y component of the logo to generate
the Y component of the PiCode data region pixels.
Parameters:
data - array with length moduleNumData * moduleNumData that stores the data
logoPixelsY - array of the Y component (range: [0, 1]) of the logo
Return:
true if successful; otherwise, false.
Remark:
1. Tiling pattern
bit-0:
255,255,255,255
255,  0,  0,255
255,  0,  0,255
255,255,255,255
bit-1:
  0,  0,  0,  0
  0,255,255,  0
  0,255,255,  0
  0,  0,  0,  0
2. Adaptive modulation scheme
For pixel with tiling pattern value 255, increase the pixel value;
for pixel with tiling pattern value 0, decrease the pixel value.
If the increased/decreased value exceeds 255/0, record the overflowed value,
and substract the overflowed value from the pixel values in the 0/255 region.
Updates:
20150325
- Update the modulation scheme as the adaptive one proposed by Lewis Chen.
*/
bool PiCodeEncoder::EmbedData(char *data, float *logoPixelsY)
{
	// Library expires at 2015.01.01
	/*
	time_t t= time(0);
	struct tm *now =localtime(& t);
	if (now->tm_year+1900 > 2015)
		return 0;
	*/

	// Get tiling pattern
	unsigned char *tilingPattern[2];
	tilingPattern[0] = new unsigned char[moduleSize * moduleSize]; // Tiling pattern for bit-0
	tilingPattern[1] = new unsigned char[moduleSize * moduleSize]; // Tiling pattern for bit-1
	GetTilingPattern(tilingPattern[0], 0);
	GetTilingPattern(tilingPattern[1], 1);
	// Data region specifications
	unsigned int dataRegionSize = moduleSize * moduleNumData;
	unsigned int moduleNumData2 = moduleNumData * moduleNumData; // Number of modules in data[]

	// Adaptive modulation proposed by Lewis Chen.
	unsigned int idxModule = 0; // Index of module (left to right, top to bottom)
	unsigned int idxModuleX = 0; // X index of module (left to right)
	unsigned int idxModuleY = 0; // Y index of module (top to bottom)
	unsigned int idx = 0; // Index of pixel
	unsigned int idxBase = 0; // Temporary variable for efficiency
	unsigned int idxLocal = 0; // Local index (index in module) of pixel
	unsigned int x = 0; // X index of pixel
	unsigned int xStart = 0; // Start of x index of pixel
	unsigned int xEnd = 0; // End of x index of pixel
	unsigned int y = 0; // Y index of pixel
	unsigned int yStart = 0; // Start of y index of pixel
	unsigned int yEnd = 0; // End of y index of pixel
	// Do the modulation module by module.
	for (idxModule = 0; idxModule < moduleNumData2; ++idxModule)
	{
		// debug
		/*if (idxModule == 34)
		{
			printf("\nEmbedData:idxModule: %d\n");
		}*/

		// Calculate the parameters.
		float energyRatio = CalculateEnergyRatio(logoPixelsY, idxModule);

		// Determine the indices.
		idxModuleX = idxModule % moduleNumData;
		idxModuleY = idxModule / moduleNumData;
		xStart = idxModuleX * moduleSize;
		xEnd = xStart + moduleSize;
		yStart = idxModuleY * moduleSize;
		yEnd = yStart + moduleSize;

		// Modulation algorithm by Lewis
		float brightExtra = 0;	// Extra pixel value greater than 255
		float darkExtra = 0;	// Extra pixel value less than 0
		if (data[idxModule] == 1)
		{
			// Calculate extra bright and dark value.
			idxLocal = 0;
			for (y = yStart; y < yEnd; ++y)
			{
				idxBase = y * dataRegionSize;
				for (x = xStart; x < xEnd; ++x)
				{
					idx = x + idxBase;
					
					if (tilingPattern[1][idxLocal] == 255)
					{
						brightExtra += max(0.0f, logoPixelsY[idx] + energyRatio * quality - 1.0f);
					}
					else
					{
						darkExtra += max(0.0f, quality / 3 - logoPixelsY[idx]);
					}

					++idxLocal;
				} // for x
			} // for y

			// Do modulation.
			float bright = energyRatio * quality + darkExtra / (moduleSize * moduleSize * 3 / 4);
			float dark = quality / 3 + brightExtra / (moduleSize * moduleSize / 4);
			idxLocal = 0;
			for (y = yStart; y < yEnd; ++y)
			{
				idxBase = y * dataRegionSize;
				for (x = xStart; x < xEnd; ++x)
				{
					idx = x + idxBase;

					if (tilingPattern[1][idxLocal] == 255)
					{
						logoPixelsY[idx] = min(1.0f, logoPixelsY[idx] + bright);
					}
					else
					{
						logoPixelsY[idx] = max(0.0f, logoPixelsY[idx] - dark);
					}

					++idxLocal;
				} // for x
			} // for y
		} // data[] == 1
		else
		{
			// Calculate extra bright and dark value.
			idxLocal = 0;
			for (y = yStart; y < yEnd; ++y)
			{
				idxBase = y * dataRegionSize;
				for (x = xStart; x < xEnd; ++x)
				{
					idx = x + idxBase;

					if (tilingPattern[0][idxLocal] == 255)
					{
						brightExtra += max(0.0f, logoPixelsY[idx] + quality - 1.0f);
					}
					else
					{
						darkExtra += max(0.0f, quality / 3 - logoPixelsY[idx]);
					}

					++idxLocal;
				} // for x
			} // for y

			// Do modulation.
			float bright = quality + darkExtra / (moduleSize * moduleSize / 4);
			float dark = quality / 3 + brightExtra / (moduleSize * moduleSize * 3 / 4);
			idxLocal = 0;
			for (y = yStart; y < yEnd; ++y)
			{
				idxBase = y * dataRegionSize;
				for (x = xStart; x < xEnd; ++x)
				{
					idx = x + idxBase;

					if (tilingPattern[0][idxLocal] == 255)
					{
						logoPixelsY[idx] = min(1.0f, logoPixelsY[idx] + bright);
					}
					else
					{
						logoPixelsY[idx] = max(0.0f, logoPixelsY[idx] - dark);
					}

					++idxLocal;
				} // for x
			} // for y
		} // data[] == 0

	} // for idxModule

	// release pointers
	delete[] tilingPattern[0];
	delete[] tilingPattern[1];
	return true;
}// fxn EmbedData()

/*
Calculate the energy ratio (defined by Lewis) of a module.
Parameters:
logoPixelsY - float array of Y component of the logo pixels
idxModule - index of module (left to right, top to bottom)
Return:
The energy ratio of this module
*/
float PiCodeEncoder::CalculateEnergyRatio(float *logoPixelsY, unsigned int idxModule)
{
	// Determine the indices.
	unsigned int idxModuleX = idxModule % moduleNumData; // X index of module (left to right)
	unsigned int idxModuleY = idxModule / moduleNumData; // Y index of module (top to bottom)
	unsigned int xStart = idxModuleX * moduleSize; // Start of x index of pixel
	unsigned int xEnd = xStart + moduleSize; // End of x index of pixel
	unsigned int yStart = idxModuleY * moduleSize; // Start of y index of pixel
	unsigned int yEnd = yStart + moduleSize; // End of y index of pixel

	// Convert pixel values from float to unsigned int.
	unsigned int i = 0; // Index of pixel in destination
	unsigned int numPixel = moduleSize * moduleSize;
	unsigned int *image = new unsigned int[numPixel];
	unsigned int idx = 0; // Index of pixel in source
	unsigned int idxBase = 0; // Temporary variable for efficiency
	unsigned int x = 0; // X index of pixel
	unsigned int y = 0; // Y index of pixel
	unsigned int width = moduleNumData * moduleSize;
	for (i = 0, y = yStart; y < yEnd; ++y)
	{
		idxBase = y * width;
		for (x = xStart; x < xEnd; ++x, ++i)
		{
			idx = x + idxBase;
			image[i] = (unsigned int)floor(logoPixelsY[idx] * 255);
		} // for x
	} // for y

	// Construct the histogram.
	unsigned int numBin = 256; // Maximum grayscale value
	unsigned int *histogram = new unsigned int[numBin];
	GetHistogram(histogram, numBin, image, numPixel);

	// Release memory
	delete[] image;

	// Determine the threshold using Otsu's method.
	ThresholdResult tr = GetThreshold(histogram, numBin);
	
	// Release memory.
	delete[] histogram;

	// Calculate the energy ratio parameter for this module according to Lewis' defintion.
	float energyRatio = sqrt((tr.meanForeground - tr.meanBackground) / (quality * 4 * 255));
	if (energyRatio < 1 ||
		(float)tr.numBackground / tr.num < 0.25 ||
		(float)tr.numForeground / tr.num < 0.25)
	{
		energyRatio = 1;
	}

	return energyRatio;
} // fxn CalculateEnergyRatio

bool PiCodeEncoder::AttachFinderPattern(CImg<unsigned int> &imgPiCode)
{
	// PiCode specification
	unsigned int BLACK = 0;
	unsigned int fpSize = moduleSize * moduleNumFP; // Finder pattern size
	unsigned int picodeSize = moduleSize * (2 * (moduleNumQZ + moduleNumFP) + moduleNumData);
	unsigned int pos1 = moduleSize * moduleNumQZ;
	unsigned int pos2 = moduleSize * (moduleNumQZ + moduleNumFP);
	unsigned int pos3 = moduleSize * (moduleNumQZ + moduleNumFP + moduleNumData);
	unsigned int pos4 = moduleSize * (moduleNumQZ + moduleNumFP + moduleNumData + moduleNumFP);
	unsigned int *r = imgPiCode.data(0, 0, 0, 0);
	unsigned int *g = imgPiCode.data(0, 0, 0, 1);
	unsigned int *b = imgPiCode.data(0, 0, 0, 2);

	unsigned int x, y, idx, idxBase;
	// Left vertical line
	for(y = pos1; y < pos4; ++y)
	{
		idxBase = y * picodeSize;
		for(x = pos1; x < pos2; ++x)
		{
			idx = x + idxBase;
			r[idx] = BLACK;
			g[idx] = BLACK;
			b[idx] = BLACK;
		}
	}
	// Bottom horizontal line
	for(y = pos3; y < pos4; ++y)
	{
		idxBase = y * picodeSize;
		for(x = pos1; x < pos4; ++x)
		{
			idx = x + idxBase;
			r[idx] = BLACK;
			g[idx] = BLACK;
			b[idx] = BLACK;
		}
	}
	// Top horizontal dots
	for(y = pos1; y < pos2; ++y)
	{
		idxBase = y * picodeSize;
		for(x = pos1; x < pos4;)
		{
			idx = x + idxBase;
			r[idx] = BLACK;
			g[idx] = BLACK;
			b[idx] = BLACK;

			++x;
			if(x % fpSize == 0)
			{
				x += fpSize;
			}
		}
	}
	// Right vertical dots
	for(y = pos1; y < pos4;)
	{
		idxBase = y * picodeSize;
		for(x = pos3; x < pos4; ++x)
		{
			idx = x + idxBase;
			r[idx] = BLACK;
			g[idx] = BLACK;
			b[idx] = BLACK;
		}

		++y;
		if(y % fpSize == 0)
		{
			y += fpSize;
		}
	}

	return true;
}// fxn AttachFinderPattern()

bool PiCodeEncoder::AttachFinderPattern(int *picodePixels)
{
	// PiCode specification
	int BLACK = 0xff000000;
	unsigned int fpSize = moduleSize * moduleNumFP; // Finder pattern size
	unsigned int picodeSize = moduleSize * (2 * (moduleNumQZ + moduleNumFP) + moduleNumData);
	unsigned int pos1 = moduleSize * moduleNumQZ;
	unsigned int pos2 = moduleSize * (moduleNumQZ + moduleNumFP);
	unsigned int pos3 = moduleSize * (moduleNumQZ + moduleNumFP + moduleNumData);
	unsigned int pos4 = moduleSize * (moduleNumQZ + moduleNumFP + moduleNumData + moduleNumFP);

	unsigned int x, y;
	// Left vertical line
	for(x = pos1; x < pos2; ++x)
	{
		for(y = pos1; y < pos4; ++y)
		{
			picodePixels[y * picodeSize + x] = BLACK;
		}
	}
	// Bottom horizontal line
	for(x = pos1; x < pos4; ++x)
	{
		for(y = pos3; y < pos4; ++y)
		{
			picodePixels[y * picodeSize + x] = BLACK;
		}
	}
	// Top horizontal dots
	for(x = pos1; x < pos4;)
	{
		for(y = pos1; y < pos2; ++y)
		{
			picodePixels[y * picodeSize + x] = BLACK;
		}

		++x;
		if(x % fpSize == 0)
		{
			x += fpSize;
		}
	}
	// Right vertical dots
	for(x = pos3; x < pos4; ++x)
	{
		for(y = pos1; y < pos4;)
		{
			picodePixels[y * picodeSize + x] = BLACK;

			++y;
			if(y % fpSize == 0)
			{
				y += fpSize;
			}
		}
	}

	return true;
}// fxn AttachFinderPattern()

// Copy data region into imgPiCode
bool PiCodeEncoder::FillDataRegion(CImg<unsigned int> &imgLogo, CImg<unsigned int> &imgPiCode)
{
	unsigned int *rSrc = imgLogo.data(0, 0, 0, 0);
	unsigned int *gSrc = imgLogo.data(0, 0, 0, 1);
	unsigned int *bSrc = imgLogo.data(0, 0, 0, 2);
	unsigned int *rDst = imgPiCode.data(0, 0, 0, 0);
	unsigned int *gDst = imgPiCode.data(0, 0, 0, 1);
	unsigned int *bDst = imgPiCode.data(0, 0, 0, 2);
	unsigned int dataRegionSize = moduleSize * moduleNumData;
	unsigned int picodeSize = moduleSize * (2 * (moduleNumQZ + moduleNumFP) + moduleNumData);

	unsigned int x, y, idxSrc, idxSrcBase, idxDst, idxDstBase;
	unsigned int posOffset = moduleSize * (moduleNumQZ + moduleNumFP);
	for(y = 0; y < dataRegionSize; ++y)
	{
		idxSrcBase = y * dataRegionSize;
		idxDstBase = (posOffset + y) * picodeSize + posOffset;
		for(x = 0; x < dataRegionSize; ++x)
		{
			idxSrc = x + idxSrcBase;
			idxDst = x + idxDstBase;
			rDst[idxDst] = rSrc[idxSrc];
			gDst[idxDst] = gSrc[idxSrc];
			bDst[idxDst] = bSrc[idxSrc];
		}
	}

	return true;
} // fxn FillDataRegion()

// Copy data region into imgPiCode
bool PiCodeEncoder::FillDataRegion(int *logoPixels, int *picodePixels)
{
	unsigned int dataRegionSize = moduleSize * moduleNumData;
	unsigned int picodeSize = moduleSize * (2 * (moduleNumQZ + moduleNumFP) + moduleNumData);

	unsigned int x, y;
	unsigned int posOffset = moduleSize * (moduleNumQZ + moduleNumFP);
	for(y = 0; y < dataRegionSize; ++y)
	{
		for(x = 0; x < dataRegionSize; ++x)
		{
			picodePixels[(posOffset + y) * picodeSize + (posOffset + x)] = logoPixels[y * dataRegionSize + x];
		}
	}

	return true;
} // fxn FillDataRegion()

/*
Get the tiling pattern for the given bit value (and module size).
Parameters:
tilingPattern - pointer to unsigned char array that stores the tiling pattern
moduleValue - bit value, 0 or 1, that this tiling pattern represents
Return:
true if success; otherwise, false.
Remark:
1. Tiling pattern
bit-0:
255,255,255,255
255,  0,  0,255
255,  0,  0,255
255,255,255,255
bit-1:
  0,  0,  0,  0
  0,255,255,  0
  0,255,255,  0
  0,  0,  0,  0
*/
bool PiCodeEncoder::GetTilingPattern(unsigned char *tilingPattern, char moduleValue)
{
	for(unsigned int y = 0; y < moduleSize; ++y)
	{
		for(unsigned int x = 0; x < moduleSize; ++x)
		{
			if(y >= (moduleSize >> 2) && y <= (moduleSize - (moduleSize >> 2)) - 1
				&& x >= (moduleSize >> 2) && x <= (moduleSize - (moduleSize >> 2)) - 1)
			{// Inner part
				if(moduleValue == 1){tilingPattern[y * moduleSize + x] = 255;} // Value 1
				else{tilingPattern[y * moduleSize + x] = 0;} // Value 0
			}
			else{ // Outer part
				if(moduleValue == 1){tilingPattern[y * moduleSize + x] = 0;} // Value 1
				else{tilingPattern[y * moduleSize + x] = 255;} // Value 0
			}// if x,y
		}// for x
	}// for y

	return true;
}// fxn


// Interface function with C symbols for PHP.
// Generate and save a picode to the current directory
int generate_picode(char msg[], char filenameLogo[], char filenamePiCode[], unsigned int moduleNumData, unsigned int moduleSize)
{
	PiCodeEncoder pc;
	pc.moduleNumData = moduleNumData;
	pc.moduleSize = moduleSize;
	int flag = pc.GeneratePiCode(msg, filenameLogo, filenamePiCode);
	
	return flag; //0: successful; 1: message too long/short; otherwise: failure.
}// fxn

