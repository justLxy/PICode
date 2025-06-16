#include <iostream>
#include "PiCodeDecoder.h"

// The C-style wrapper function from PiCodeDecoder.h
extern "C" string decode_picode(char* filename);

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <input_image_path>" << std::endl;
        return 1;
    }

    char* filename = argv[1];

    // The function returns the decoded string.
    // If it fails, it returns a specific error message.
    std::string decoded_message = decode_picode(filename);
    
    std::cout << decoded_message << std::endl;

    return 0;
} 