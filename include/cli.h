/****************
 * Author: Sinan Demir
 * File: cli.h
 * Date: 11/19/2025
 * Purpose: Header file for CLI module.
 *****************/

#ifndef ORBIT_SIM_CLI_H
#define ORBIT_SIM_CLI_H

#include <string>
#include <iostream>
#include <stdexcept>

/***********************
 * struct CLIOptions
 * @brief: Holds all command-line arguments for orbit-sim.
 *
 * Supported commands:
 *    - run
 *    - list
 *    - info
 *    - fetch
 ***********************/
struct CLIOptions {
    std::string command;       ///< Command name
    std::string systemFile;    ///< JSON file (for run/info)
    int steps = -1;            ///< Number of simulation steps
    double dt = -1;            ///< Timestep (seconds)
    std::string output = "orbit.csv"; ///< Output path

    // ----- FETCH OPTIONS -----
    std::string fetchBody;     ///< e.g. "499", "Mars"
    std::string fetchCenter;   ///< e.g. "0", "@ssb"
    std::string fetchStart;    ///< Start time (string)
    std::string fetchStop;     ///< Stop time
    std::string fetchStep;     ///< Step size ("1 d", "1 h", etc.)
};

CLIOptions parseCLI(int argc, char** argv);

#endif  // ORBIT_SIM_CLI_H
