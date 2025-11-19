/****************
 * Author: Sinan Demir
 * File: cli.h
 * Date: 11/18/2025
 * Purpose: header file for CLI module.
 *****************/


#ifndef ORBIT_SIM_CLI_H
#define ORBIT_SIM_CLI_H

#include <string>
#include <iostream>
#include <stdexcept>

struct CLIOptions {
    std::string command;       // "run", "list", "info"
    std::string systemFile;    // path to JSON
    int steps = -1;
    double dt = -1;
    std::string output = "orbit.csv";
};

CLIOptions parseCLI(int argc, char** argv);

#endif  // ORBIT_SIM_CLI_H
