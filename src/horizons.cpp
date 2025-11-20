/********************
 * Author: Sinan Demir
 * File: horizons.cpp
 * Date: 11/19/2025
 * Purpose:
 *    Implementation of NASA/JPL HORIZONS File API wrapper using libcurl.
 *********************/

#include "horizons.h"

#include <curl/curl.h>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <sstream>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

// libcurl write callback: append received data to std::string buffer
static size_t writeToStringCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total = size * nmemb;
    auto* buffer = static_cast<std::string*>(userp);
    buffer->append(static_cast<char*>(contents), total);
    return total;
}

bool fetchHorizonsEphemeris(const HorizonsFetchOptions& opts,
                            const std::string& outputPath)
{
    const std::string baseUrl = "https://ssd-api.jpl.nasa.gov/horizons_file.api";

    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "❌ Failed to initialize libcurl\n";
        return false;
    }

    // Fix dates: add " 00:00" if user provided only a yyyy-mm-dd
    auto fixDate = [](const std::string& s) {
        if (s.find(':') == std::string::npos) {
            return s + " 00:00";
        }
        return s;
    };

    std::string fixed_start = fixDate(opts.start_time);
    std::string fixed_stop  = fixDate(opts.stop_time);

    // Encode parameters EXCEPT CENTER (@ must NOT be escaped)
    char* esc_cmd   = curl_easy_escape(curl, opts.command.c_str(),     0);
    char* esc_start = curl_easy_escape(curl, fixed_start.c_str(),      0);
    char* esc_stop  = curl_easy_escape(curl, fixed_stop.c_str(),       0);
    char* esc_step  = curl_easy_escape(curl, opts.step_size.c_str(),   0);

    if (!esc_cmd || !esc_start || !esc_stop || !esc_step) {
        std::cerr << "❌ curl_easy_escape returned NULL\n";
        curl_easy_cleanup(curl);
        return false;
    }

    // Construct URL exactly in Horizons format
    std::ostringstream url;
    url << baseUrl
        << "?format=json"
        << "&COMMAND='"    << esc_cmd      << "'"
        << "&CENTER='"     << opts.center  << "'"        // NOT escaped
        << "&EPHEM_TYPE=VECTORS"
        << "&START_TIME='" << esc_start    << "'"
        << "&STOP_TIME='"  << esc_stop     << "'"
        << "&STEP_SIZE='"  << esc_step     << "'"
        << "&MAKE_EPHEM=YES";

    curl_free(esc_cmd);
    curl_free(esc_start);
    curl_free(esc_stop);
    curl_free(esc_step);

    std::string response;

    curl_easy_setopt(curl, CURLOPT_URL, url.str().c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToStringCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "orbit-sim/1.0");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    std::cout << "DEBUG URL = " << url.str() << "\n\n";

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "❌ curl_easy_perform() failed: "
                  << curl_easy_strerror(res) << "\n";
        curl_easy_cleanup(curl);
        return false;
    }

    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_easy_cleanup(curl);

    if (httpCode != 200) {
        std::cerr << "❌ HORIZONS HTTP error code: " << httpCode << "\n";
        return false;
    }

    // Parse JSON response
    json j;
    try {
        j = json::parse(response);
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to parse HORIZONS JSON: " << e.what() << "\n";
        return false;
    }

    if (j.contains("error")) {
        std::cerr << "❌ HORIZONS API returned error: "
                  << j["error"].get<std::string>() << "\n";
        return false;
    }

    if (!j.contains("result")) {
        std::cerr << "❌ HORIZONS JSON missing 'result' field\n";
        return false;
    }

    std::string ephemText = j["result"].get<std::string>();

    // Write output
    std::ofstream out(outputPath);
    if (!out) {
        std::cerr << "❌ Could not open output file: " << outputPath << "\n";
        return false;
    }

    out << ephemText;
    out.close();

    std::cout << "✅ HORIZONS ephemeris saved to: " << outputPath << "\n";
    return true;
}
