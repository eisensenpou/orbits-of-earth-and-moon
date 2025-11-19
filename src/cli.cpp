/****************
 * Author: Sinan Demir
 * File: cli.cpp
 * Date: 11/18/2025
 * Purpose: Implementation file for CLI module.
 *****************/

#include "cli.h"


CLIOptions parseCLI(int argc, char** argv) {
/**********************
 * parseCLI
 * @brief: Parses command-line arguments into a CLIOptions struct.
 * @param: argc - argument count
 * @param: argv - argument vector
 * @return: CLIOptions struct with parsed values
 * @exception: exits on invalid usage
 * @note: Supports commands: run, list, info
 **********************/
    CLIOptions opt;

    if (argc < 2) {
        std::cerr << "Usage: orbit-sim <command> [options]\n";
        exit(1);
    }

    opt.command = argv[1];

    for (int i = 2; i < argc; ++i) {
        std::string a = argv[i];

        if (a == "--system" && i + 1 < argc) {
            opt.systemFile = argv[++i];
        }
        else if (a == "--steps" && i + 1 < argc) {
            opt.steps = std::stoi(argv[++i]);
        }
        else if (a == "--dt" && i + 1 < argc) {
            opt.dt = std::stod(argv[++i]);
        }
        else if (a == "--output" && i + 1 < argc) {
            opt.output = argv[++i];
        }
        else {
            std::cerr << "Unknown option: " << a << "\n";
            exit(1);
        }
    }

    return opt;
}  // end parseCLI
