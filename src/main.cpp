/********************
 * Author: Sinan Demir
 * File: main.cpp
 * Date: 11/18/2025
 * Purpose:
 *    Entry point for orbit-sim CLI application.
 *    Supports:
 *      - Running N-body simulations from JSON system files
 *      - Printing basic system info
 *      - Listing available system definitions
 *********************/

#include "main.h"

/********************
 * printSystemInfo
 * @brief: Prints details of bodies loaded from JSON
 *********************/
void printSystemInfo(const std::string& path) {
    try {
        auto bodies = loadSystemFromJSON(path);
        std::cout << "System file: " << path << "\n";
        std::cout << "Bodies:\n";

        for (const auto& b : bodies) {
            std::cout << " - " << b.name
                      << " | mass=" << b.mass
                      << " | pos=(" << b.position << ")"
                      << " | vel=(" << b.velocity << ")"
                      << "\n";
        }
    }
    catch (const std::exception& e) {
        std::cerr << "❌ Error loading system: " << e.what() << "\n";
    }
}

/********************
 * listSystems
 * @brief: Lists JSON files inside the "systems" directory.
 *********************/
void listSystems(const std::string& dir = "systems") {
    std::cout << "Available systems in \"" << dir << "\":\n";

    if (!std::filesystem::exists(dir)) {
        std::cout << "(No systems directory found)\n";
        return;
    }

    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (entry.path().extension() == ".json") {
            std::cout << " - " << entry.path().string() << "\n";
        }
    }
}

/********************
 * main
 * @brief: Parses CLI arguments and performs actions.
 *********************/
int main(int argc, char** argv) {

    CLIOptions opt = parseCLI(argc, argv);

    if (opt.command == "list") {
        listSystems();
        return 0;
    }

    if (opt.command == "info") {
        if (opt.systemFile.empty()) {
            std::cerr << "❌ Must specify --system <file.json>\n";
            return 1;
        }
        printSystemInfo(opt.systemFile);
        return 0;
    }

    if (opt.command == "run") {
        if (opt.systemFile.empty()) {
            std::cerr << "❌ Must specify --system <file.json>\n";
            return 1;
        }

        try {
            // Load system from JSON
            auto bodies = loadSystemFromJSON(opt.systemFile);

            // Determine simulation steps and dt
            int steps = (opt.steps > 0 ? opt.steps : 8766);
            double dt = (opt.dt > 0 ? opt.dt : 3600.0);  // default: 1 hour

            std::cout << "Running simulation:\n"
                      << " - System: " << opt.systemFile << "\n"
                      << " - Steps:  " << steps << "\n"
                      << " - dt:     " << dt << " seconds\n"
                      << " - Output: " << opt.output << "\n";

            // Run simulation
            runSimulation(bodies, steps, dt, opt.output);
        }
        catch (const std::exception& e) {
            std::cerr << "❌ Simulation failed: " << e.what() << "\n";
            return 1;
        }

        return 0;
    }

    // Command not recognized
    std::cerr << "❌ Unknown command: " << opt.command << "\n";
    std::cerr << "Valid commands are:\n"
              << "  orbit-sim list\n"
              << "  orbit-sim info --system <file.json>\n"
              << "  orbit-sim run  --system <file.json> --steps N --dt T\n";

    return 1;
}
