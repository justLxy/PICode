/*
* File:    bch3.c
* Title:   Encoder/decoder for binary BCH codes in C (Version 3.1)
* Author:  Robert Morelos-Zaragoza
* Date:    August 1994
* Revised: June 13, 1997
*
* ===============  Encoder/Decoder for binary BCH codes in C =================
*
* Version 1:   Original program. The user provides the generator polynomial
*              of the code (cumbersome!).
* Version 2:   Computes the generator polynomial of the code.
* Version 3:   No need to input the coefficients of a primitive polynomial of
*              degree m, used to construct the Galois Field GF(2**m). The
*              program now works for any binary BCH code of length such that:
*              2**(m-1) - 1 < length <= 2**m - 1
*
* Note:        You may have to change the size of the arrays to make it work.
*
* The encoding and decoding methods used in this program are based on the
* book "Error Control Coding: Fundamentals and Applications", by Lin and
* Costello, Prentice Hall, 1983.
*
* Thanks to Patrick Boyle (pboyle@era.com) for his observation that 'bch2.c'
* did not work for lengths other than 2**m-1 which led to this new version.
* Portions of this program are from 'rs.c', a Reed-Solomon encoder/decoder
* in C, written by Simon Rockliff (simon@augean.ua.oz.au) on 21/9/89. The
* previous version of the BCH encoder/decoder in C, 'bch2.c', was written by
* Robert Morelos-Zaragoza (robert@spectra.eng.hawaii.edu) on 5/19/92.
*
* NOTE:    
*          The author is not responsible for any malfunctioning of
*          this program, nor for any damage caused by it. Please include the
*          original program along with these comments in any redistribution.
*
*  For more information, suggestions, or other ideas on implementing error
*  correcting codes, please contact me at:
*
*                           Robert Morelos-Zaragoza
*                           5120 Woodway, Suite 7036
*                           Houston, Texas 77056
*
*                    email: r.morelos-zaragoza@ieee.org
*
* COPYRIGHT NOTICE: This computer program is free for non-commercial purposes.
* You may implement this program for any non-commercial application. You may 
* also implement this program for commercial purposes, provided that you
* obtain my written permission. Any modification of this program is covered
* by this copyright.
*
* == Copyright (c) 1994-7,  Robert Morelos-Zaragoza. All rights reserved.  ==
*
* m = order of the Galois field GF(2**m) 
* n = 2**m - 1 = size of the multiplicative group of GF(2**m)
* length = length of the BCH code
* t = error correcting capability (max. no. of errors the code corrects)
* d = 2*t + 1 = designed min. distance = no. of consecutive roots of g(x) + 1
* k = n - deg(g(x)) = dimension (no. of information bits/codeword) of the code
* p[] = coefficients of a primitive polynomial used to generate GF(2**m)
* g[] = coefficients of the generator polynomial, g(x)
* alpha_to [] = log table of GF(2**m) 
* index_of[] = antilog table of GF(2**m)
* data[] = information bits = coefficients of data polynomial, i(x)
* bb[] = coefficients of redundancy polynomial x^(length-k) i(x) modulo g(x)
* numerr = number of errors 
* errpos[] = error positions 
* recd[] = coefficients of the received polynomial 
* decerror = number of decoding errors (in _message_ positions) 
*
*/
/*
* File:			bchenc.c
* Modified by:	Alan Baojian ZHOU (Email: bzhouab@ust.hk)
* Last-modified: 2013-08-21 
* Note:		This is modified from the file bch3.c written by Robert Morelos-Zaragoza.
*/

#include <stdlib.h>
#include <string.h>

// Parameters
static int	m, length, t;
static int	n, k, d;
// Locally allocated memory; remember to delete
static int *p = NULL;
static int *alpha_to = NULL;
static int *index_of = NULL;
static int *g = NULL;
// Pointer passed in ; no need to delete; pay attention to the size
static unsigned char *data = NULL;
static unsigned char *bb = NULL;


void 
	gf_poly()
	/*
	*	Read m, the degree of a primitive polynomial p(x) used to compute the
	*	Galois field GF(2**m). Get precomputed coefficients p[] of p(x). Read
	*	the code length.
	*/
{
	int ninf;
	int i ;
	// Check if m is valid (2 to 20)
	if ( !(m>1) || !(m<21) )
	{
		exit(1);
	}

	// Check if length is valid (2^(m-1) - 1 to 2^m - 1)
	n = (1 << m) - 1;
	ninf = (1 << (m - 1)) - 1;
	if ( !(length > ninf) || !(length <= n) )
	{
		exit(2);
	}

	// Set primitive polynomial for GF
	for (i = 1; i < m; i++)
		p[i] = 0;
	p[0] = p[m] = 1;
	if (m == 2)			p[1] = 1;
	else if (m == 3)	p[1] = 1;
	else if (m == 4)	p[1] = 1;
	else if (m == 5)	p[2] = 1;
	else if (m == 6)	p[1] = 1;
	else if (m == 7)	p[1] = 1;
	else if (m == 8)	p[4] = p[5] = p[6] = 1;
	else if (m == 9)	p[4] = 1;
	else if (m == 10)	p[3] = 1;
	else if (m == 11)	p[2] = 1;
	else if (m == 12)	p[3] = p[4] = p[7] = 1;
	else if (m == 13)	p[1] = p[3] = p[4] = 1;
	else if (m == 14)	p[1] = p[11] = p[12] = 1;
	else if (m == 15)	p[1] = 1;
	else if (m == 16)	p[2] = p[3] = p[5] = 1;
	else if (m == 17)	p[3] = 1;
	else if (m == 18)	p[7] = 1;
	else if (m == 19)	p[1] = p[5] = p[6] = 1;
	else if (m == 20)	p[3] = 1;
}


void 
	generate_gf()
	/*
	* Generate field GF(2**m) from the irreducible polynomial p(X) with
	* coefficients in p[0]..p[m].
	*
	* Lookup tables:
	*   index->polynomial form: alpha_to[] contains j=alpha^i;
	*   polynomial form -> index form:	index_of[j=alpha^i] = i
	*
	* alpha=2 is the primitive element of GF(2**m) 
	*/
{
	register int    i, mask;

	mask = 1;
	alpha_to[m] = 0;
	for (i = 0; i < m; i++) {
		alpha_to[i] = mask;
		index_of[alpha_to[i]] = i;
		if (p[i] != 0)
			alpha_to[m] ^= mask;
		mask <<= 1;
	}
	index_of[alpha_to[m]] = m;
	mask >>= 1;
	for (i = m + 1; i < n; i++) {
		if (alpha_to[i - 1] >= mask)
			alpha_to[i] = alpha_to[m] ^ ((alpha_to[i - 1] ^ mask) << 1);
		else
			alpha_to[i] = alpha_to[i - 1] << 1;
		index_of[alpha_to[i]] = i;
	}
	index_of[0] = -1;
}


void 
	gen_poly()
	/*
	* Compute the generator polynomial of a binary BCH code. Fist generate the
	* cycle sets modulo 2**m - 1, cycle[][] =  (i, 2*i, 4*i, ..., 2^l*i). Then
	* determine those cycle sets that contain integers in the set of (d-1)
	* consecutive integers {1..(d-1)}. The generator polynomial is calculated
	* as the product of linear factors of the form (x+alpha^i), for every i in
	* the above cycle sets.
	*/
{
	register int	ii, jj, ll, kaux;
	register int	test, aux, nocycles, root, noterms, rdncy;
	int             cycle[1024][21], size[1024], min[1024], zeros[1024];

	/* Generate cycle sets modulo n, n = 2**m - 1 */
	cycle[0][0] = 0;
	size[0] = 1;
	cycle[1][0] = 1;
	size[1] = 1;
	jj = 1;			/* cycle set index */

	do {
		/* Generate the jj-th cycle set */
		ii = 0;
		do {
			ii++;
			cycle[jj][ii] = (cycle[jj][ii - 1] * 2) % n;
			size[jj]++;
			aux = (cycle[jj][ii] * 2) % n;
		} while (aux != cycle[jj][0]);
		/* Next cycle set representative */
		ll = 0;
		do {
			ll++;
			test = 0;
			for (ii = 1; ((ii <= jj) && (!test)); ii++)	
				/* Examine previous cycle sets */
					for (kaux = 0; ((kaux < size[ii]) && (!test)); kaux++)
						if (ll == cycle[ii][kaux])
							test = 1;
		} while ((test) && (ll < (n - 1)));
		if (!(test)) {
			jj++;	/* next cycle set index */
			cycle[jj][0] = ll;
			size[jj] = 1;
		}
	} while (ll < (n - 1));
	nocycles = jj;		/* number of cycle sets modulo n */

	d = 2 * t + 1; // Mimum distance

	/* Search for roots 1, 2, ..., d-1 in cycle sets */
	kaux = 0;
	rdncy = 0;
	for (ii = 1; ii <= nocycles; ii++) {
		min[kaux] = 0;
		test = 0;
		for (jj = 0; ((jj < size[ii]) && (!test)); jj++)
			for (root = 1; ((root < d) && (!test)); root++)
				if (root == cycle[ii][jj])  {
					test = 1;
					min[kaux] = ii;
				}
				if (min[kaux]) {
					rdncy += size[min[kaux]];
					kaux++;
				}
	}
	noterms = kaux;
	kaux = 1;
	for (ii = 0; ii < noterms; ii++)
		for (jj = 0; jj < size[min[ii]]; jj++) {
			zeros[kaux] = cycle[min[ii]][jj];
			kaux++;
		}

		k = length - rdncy;

		if (k<0)
		{
			exit(3);
		}

		/* Compute the generator polynomial */
		g[0] = alpha_to[zeros[1]];
		g[1] = 1;		/* g(x) = (X + zeros[1]) initially */
		for (ii = 2; ii <= rdncy; ii++) {
			g[ii] = 1;
			for (jj = ii - 1; jj > 0; jj--)
				if (g[jj] != 0)
					g[jj] = g[jj - 1] ^ alpha_to[(index_of[g[jj]] + zeros[ii]) % n];
				else
					g[jj] = g[jj - 1];
			g[0] = alpha_to[(index_of[g[0]] + zeros[ii]) % n];
		}

}


void 
	encode()
	/*
	* Compute redundacy bb[], the coefficients of b(x). The redundancy
	* polynomial b(x) is the remainder after dividing x^(length-k)*data(x)
	* by the generator polynomial g(x).
	*/
{
	register int    i, j;
	register unsigned char  feedback;

	for (i = 0; i < length - k; i++)
		bb[i] = 0;
	for (i = k - 1; i >= 0; i--) {
		feedback = data[i] ^ bb[length - k - 1];
		if (feedback != 0) {
			for (j = length - k - 1; j > 0; j--)
				if (g[j] != 0)
					bb[j] = bb[j - 1] ^ feedback;
				else
					bb[j] = bb[j - 1];
			bb[0] = g[0] && feedback;
		} else {
			for (j = length - k - 1; j > 0; j--)
				bb[j] = bb[j - 1];
			bb[0] = 0;
		}
	}
}


void bchenc(int m_in, int length_in, int t_in, unsigned char *data_in)
{
	// Parameters
	m = m_in; // m: 2 to 20
	length = length_in; // length: 2^(m/2)-1 to 2^m-1
	t = t_in; // t cannot be arbitrarily choosen; refer to matlab please

	// Memory allocation
	p = (int*)malloc(sizeof(int)*(m + 1));
	alpha_to = (int*)malloc(sizeof(int)*(1<<m));
	index_of = (int*)malloc(sizeof(int)*(1<<m));
	g = (int*)malloc(sizeof(int)*(1<<m));

	gf_poly();					// Set the primitive polynomial of the Galois Field GF(2^m) 
	generate_gf();			// Construct the Galois Field GF(2^m)
	gen_poly();             // Compute the generator polynomial of BCH code

	// Data storage
	data = data_in;		// Data pointer; encoded data will still be stored here 
	bb = data + k; // k is set after gen_poly()
	encode();					// Encode data

	// Memory release
	free(p);
	free(alpha_to);
	free(index_of);
	free(g);
} // fxn bchenc()
