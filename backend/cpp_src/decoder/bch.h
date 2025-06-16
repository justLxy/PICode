#ifndef __BCH_H__
#define __BCH_H__

#include <math.h>
#include <stdio.h>





int decode_bch();
void encode_bch();
void gen_poly();
void generate_gf();
void read_p();


int decodebch(int bchcodeword[]);

#endif 
