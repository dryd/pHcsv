#include "pHcsv.h"

#include <chrono>
#include <iostream>
#include <sstream>

void logPerf(const std::string& label, std::chrono::time_point<std::chrono::high_resolution_clock> start) {
    std::cout << label << ": " << std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start).count() << " ms" << std::endl;
}

struct SSO {
    std::string solution_id;
    std::string source_id;
    std::string observation_id;
    size_t number_mp;
    double epoch;
    double epoch_err;
    double epoch_utc;
    double ra;
    double dec;
    double ra_error_systematic;
    double dec_error_systematic;
    double ra_dec_correlation_systematic;
    double ra_error_random;
    double dec_error_random;
    double ra_dec_correlation_random;
    double g_mag;
    double g_flux;
    double g_flux_error;
    double x_gaia;
    double y_gaia;
    double z_gaia;
    double vx_gaia;
    double vy_gaia;
    double vz_gaia;
    double position_angle_scan;
    double level_of_confidence;
};

SSO convertFromDynamic(const pHcsv::mapped_row& row) {
    SSO sso;
    sso.solution_id = row.at("solution_id");
    sso.source_id = row.at("source_id");
    sso.observation_id = row.at("observation_id");
    sso.number_mp = row.get<size_t>("number_mp");
    sso.epoch = row.get<double>("epoch");
    sso.epoch_err = row.get<double>("epoch_err");
    sso.epoch_utc = row.get<double>("epoch_utc");
    sso.ra = row.get<double>("ra");
    sso.dec = row.get<double>("dec");
    sso.ra_error_systematic = row.get<double>("ra_error_systematic");
    sso.dec_error_systematic = row.get<double>("dec_error_systematic");
    sso.ra_dec_correlation_systematic = row.get<double>("ra_dec_correlation_systematic");
    sso.ra_error_random = row.get<double>("ra_error_random");
    sso.dec_error_random = row.get<double>("dec_error_random");
    sso.ra_dec_correlation_random = row.get<double>("ra_dec_correlation_random");
    sso.g_mag = row.at("g_mag").empty() ? 0.0 : row.get<double>("g_mag");
    sso.g_flux = row.at("g_flux").empty() ? 0.0 : row.get<double>("g_flux");
    sso.g_flux_error = row.at("g_flux_error").empty() ? 0.0 : row.get<double>("g_flux_error");
    sso.x_gaia = row.get<double>("x_gaia");
    sso.y_gaia = row.get<double>("y_gaia");
    sso.z_gaia = row.get<double>("z_gaia");
    sso.vx_gaia = row.get<double>("vx_gaia");
    sso.vy_gaia = row.get<double>("vy_gaia");
    sso.vz_gaia = row.get<double>("vz_gaia");
    sso.position_angle_scan = row.get<double>("position_angle_scan");
    sso.level_of_confidence = row.get<double>("level_of_confidence");
    return sso;
}

SSO convertFromVec(const std::vector<std::string>& row) {
    SSO sso;
    sso.solution_id = row.at(0);
    sso.source_id = row.at(1);
    sso.observation_id = row.at(2);
    sso.number_mp = pHcsv::detail::convert<size_t>(row.at(3));
    sso.epoch = pHcsv::detail::convert<double>(row.at(4));
    sso.epoch_err = pHcsv::detail::convert<double>(row.at(5));
    sso.epoch_utc = pHcsv::detail::convert<double>(row.at(6));
    sso.ra = pHcsv::detail::convert<double>(row.at(7));
    sso.dec = pHcsv::detail::convert<double>(row.at(8));
    sso.ra_error_systematic = pHcsv::detail::convert<double>(row.at(9));
    sso.dec_error_systematic = pHcsv::detail::convert<double>(row.at(10));
    sso.ra_dec_correlation_systematic = pHcsv::detail::convert<double>(row.at(11));
    sso.ra_error_random = pHcsv::detail::convert<double>(row.at(12));
    sso.dec_error_random = pHcsv::detail::convert<double>(row.at(13));
    sso.ra_dec_correlation_random = pHcsv::detail::convert<double>(row.at(14));
    sso.g_mag = row.at(15).empty() ? 0.0 : pHcsv::detail::convert<double>(row.at(15));
    sso.g_flux = row.at(16).empty() ? 0.0 : pHcsv::detail::convert<double>(row.at(16));
    sso.g_flux_error = row.at(17).empty() ? 0.0 : pHcsv::detail::convert<double>(row.at(17));
    sso.x_gaia = pHcsv::detail::convert<double>(row.at(18));
    sso.y_gaia = pHcsv::detail::convert<double>(row.at(19));
    sso.z_gaia = pHcsv::detail::convert<double>(row.at(20));
    sso.vx_gaia = pHcsv::detail::convert<double>(row.at(21));
    sso.vy_gaia = pHcsv::detail::convert<double>(row.at(22));
    sso.vz_gaia = pHcsv::detail::convert<double>(row.at(23));
    sso.position_angle_scan = pHcsv::detail::convert<double>(row.at(24));
    sso.level_of_confidence = pHcsv::detail::convert<double>(row.at(25));
    return sso;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        throw std::runtime_error("No mode specified");
    }
    int mode = atoi(argv[1]);
    if (mode == 0) {
        auto start = std::chrono::high_resolution_clock::now();
        pHcsv::mapped data("test_data/SsoObservation.csv");
        logPerf("pHcsv::mapped", start);

        start = std::chrono::high_resolution_clock::now();
        double avg = 0.0;
        for (size_t i = 0; i < data.size(); i++) {
            avg += data.get<double>(i, "x_gaia");
        }
        logPerf("pHcsv::mapped avg method 0 (" + std::to_string(avg/static_cast<double>(data.size())) + ")", start);

        start = std::chrono::high_resolution_clock::now();
        avg = 0.0;
        size_t x_gaia_index = data.headerIndex("x_gaia");
        for (size_t i = 0; i < data.size(); i++) {
            avg += data.get<double>(i, x_gaia_index);
        }
        logPerf("pHcsv::mapped avg method 1 (" + std::to_string(avg / static_cast<double>(data.size())) + ")", start);
    }
    if (mode == 1) {
        auto start = std::chrono::high_resolution_clock::now();
        pHcsv::flat data("test_data/SsoObservation_no_header.csv");
        logPerf("pHcsv::flat", start);
    }
    if (mode == 2) {
        auto start = std::chrono::high_resolution_clock::now();
        std::vector<SSO> data;
        pHcsv::streamRows("test_data/SsoObservation.csv", [&data] (const pHcsv::mapped_row& row) {
            data.push_back(convertFromDynamic(row));
        });
        logPerf("pHcsv::streamRows", start);

        start = std::chrono::high_resolution_clock::now();
        double avg = 0.0;
        for (const SSO& sso : data) {
            avg += sso.x_gaia;
        }
        logPerf("pHcsv::streamRows avg (" + std::to_string(avg/static_cast<double>(data.size())) + ")", start);
    }
    if (mode == 3) {
        auto start = std::chrono::high_resolution_clock::now();
        std::vector<SSO> data;
        pHcsv::streamRows("test_data/SsoObservation_no_header.csv", [&data] (const std::vector<std::string>& row) {
            data.push_back(convertFromVec(row));
        });
        logPerf("pHcsv::streamRows (no_header)", start);
    }
    if (mode == -1) {
        // Directly from stringstream
        std::ifstream t("test_data/SsoObservation.csv");
        std::stringstream strStream;
        strStream << t.rdbuf();//read the file

        auto start = std::chrono::high_resolution_clock::now();
        pHcsv::mapped data(strStream);
        logPerf("From stringstream", start);
    }
}
