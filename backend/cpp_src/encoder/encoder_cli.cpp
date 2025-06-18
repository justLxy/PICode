#include <iostream>
#include <string> // Required for std::stoi

#include "PiCodeEncoder.h"

// The function is declared in PiCodeEncoder.h
// extern "C" int generate_picode(char msg[], char filenameLogo[], char filenamePiCode[], unsigned int moduleNumData, unsigned int moduleSize);

int main(int argc, char* argv[]) {
    if (argc != 7) {
        std::cerr << "Usage: " << argv[0] << " <message> <logo_file_path> <output_file_path> <module_num_data> <module_size> <quality>" << std::endl;
        return 1;
    }

    char* msg = argv[1];
    char* filenameLogo = argv[2];
    char* filenamePiCode = argv[3];
    unsigned int moduleNumData = std::stoi(argv[4]);
    unsigned int moduleSize = std::stoi(argv[5]);
    unsigned int quality = std::stoi(argv[6]);

    int result = generate_picode(msg, filenameLogo, filenamePiCode, moduleNumData, moduleSize, quality);

    if (result == 0) {
        // The output path is what we need in the backend, so let's just print that to stdout
        std::cout << filenamePiCode;
        return 0;
    } else {
        std::cerr << "PiCode generation failed with error code: " << result << std::endl;
        // Corresponding error messages from test file:
        // 1: Message too long!
        // other: PiCode file generation failed
        if (result == 1) {
            std::cerr << "Message too long!" << std::endl;
        } else {
            std::cerr << "PiCode file generation failed." << std::endl;
        }
        return 1;
    }
} 