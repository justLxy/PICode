#ifndef blocks
#define blocks

class Blocks{
	public:
		//Image
		unsigned char**		DecodeBm;
		int 				imgWidth;
		int					imgHeight;

		//Block's variables
		int					blockDiff;
		int					blockCenter;
		int					blockEdge;
		int					totalByte;
		int					totalModules;
		const static int	edgeThreshold = 25;

		//Erasure Marking variable
		int* 				allBlkDiff;
		int* 				allBlkOrder;
		int* 				erasurePosition;
		int					numErasure;
		int					erasureLimit;

		Blocks (unsigned char**, int, int);
		~Blocks();
		bool DecodeBlock(int, int, int, int, int);

	private:
		//Block's variables
		const static int scaledSize = 8;

		unsigned char** getBlockFromBm(int, int, int, int);
		bool decodeScaledBlock(unsigned char**, int);
//		int demodule(int, int, int, int);
		unsigned char** resize(unsigned char**, int, int, int, int);
};

#endif
