
#include <iostream>
using namespace std;



#ifndef _PICODE_DECODER_H
#define _PICODE_DECODER_H



class PiCodeDecoder
{


public:
	string picode (unsigned char* grey, int width, int height);
	string picodedecode (char* filename);
unsigned char* bilinearresize(unsigned char in[],
    int src_width, int src_height, int dest_width, int dest_height);

};


extern "C" string decode_picode(char* filename);

#endif

