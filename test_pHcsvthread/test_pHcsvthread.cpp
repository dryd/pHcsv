#include "../pHcsvthread.h"

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

SSO convertFromMapped(const pH::csv::mapped& data, size_t i) {
    SSO sso;
    sso.solution_id = data.at(i, "solution_id");
    sso.source_id = data.at(i, "source_id");
    sso.observation_id = data.at(i, "observation_id");
    sso.number_mp = data.get<size_t>(i, "number_mp");
    sso.epoch = data.get<double>(i, "epoch");
    sso.epoch_err = data.get<double>(i, "epoch_err");
    sso.epoch_utc = data.get<double>(i, "epoch_utc");
    sso.ra = data.get<double>(i, "ra");
    sso.dec = data.get<double>(i, "dec");
    sso.ra_error_systematic = data.get<double>(i, "ra_error_systematic");
    sso.dec_error_systematic = data.get<double>(i, "dec_error_systematic");
    sso.ra_dec_correlation_systematic = data.get<double>(i, "ra_dec_correlation_systematic");
    sso.ra_error_random = data.get<double>(i, "ra_error_random");
    sso.dec_error_random = data.get<double>(i, "dec_error_random");
    sso.ra_dec_correlation_random = data.get<double>(i, "ra_dec_correlation_random");
    sso.g_mag = data.at(i, "g_mag").empty() ? 0.0 : data.get<double>(i, "g_mag");
    sso.g_flux = data.at(i, "g_flux").empty() ? 0.0 : data.get<double>(i, "g_flux");
    sso.g_flux_error = data.at(i, "g_flux_error").empty() ? 0.0 : data.get<double>(i, "g_flux_error");
    sso.x_gaia = data.get<double>(i, "x_gaia");
    sso.y_gaia = data.get<double>(i, "y_gaia");
    sso.z_gaia = data.get<double>(i, "z_gaia");
    sso.vx_gaia = data.get<double>(i, "vx_gaia");
    sso.vy_gaia = data.get<double>(i, "vy_gaia");
    sso.vz_gaia = data.get<double>(i, "vz_gaia");
    sso.position_angle_scan = data.get<double>(i, "position_angle_scan");
    sso.level_of_confidence = data.get<double>(i, "level_of_confidence");
    return sso;
}

SSO convertFromDynamic(const pH::csv::mapped_row& row) {
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

SSO convertFromFlat(const pH::csv::flat& data, size_t i) {
    SSO sso;
    sso.solution_id = data.at(i, 0);
    sso.source_id = data.at(i, 1);
    sso.observation_id = data.at(i, 2);
    sso.number_mp = pH::csv::detail::convert<size_t>(data.at(i, 3));
    sso.epoch = pH::csv::detail::convert<double>(data.at(i, 4));
    sso.epoch_err = pH::csv::detail::convert<double>(data.at(i, 5));
    sso.epoch_utc = pH::csv::detail::convert<double>(data.at(i, 6));
    sso.ra = pH::csv::detail::convert<double>(data.at(i, 7));
    sso.dec = pH::csv::detail::convert<double>(data.at(i, 8));
    sso.ra_error_systematic = pH::csv::detail::convert<double>(data.at(i, 9));
    sso.dec_error_systematic = pH::csv::detail::convert<double>(data.at(i, 10));
    sso.ra_dec_correlation_systematic = pH::csv::detail::convert<double>(data.at(i, 11));
    sso.ra_error_random = pH::csv::detail::convert<double>(data.at(i, 12));
    sso.dec_error_random = pH::csv::detail::convert<double>(data.at(i, 13));
    sso.ra_dec_correlation_random = pH::csv::detail::convert<double>(data.at(i, 14));
    sso.g_mag = data.at(i, 15).empty() ? 0.0 : pH::csv::detail::convert<double>(data.at(i, 15));
    sso.g_flux = data.at(i, 16).empty() ? 0.0 : pH::csv::detail::convert<double>(data.at(i, 16));
    sso.g_flux_error = data.at(i, 17).empty() ? 0.0 : pH::csv::detail::convert<double>(data.at(i, 17));
    sso.x_gaia = pH::csv::detail::convert<double>(data.at(i, 18));
    sso.y_gaia = pH::csv::detail::convert<double>(data.at(i, 19));
    sso.z_gaia = pH::csv::detail::convert<double>(data.at(i, 20));
    sso.vx_gaia = pH::csv::detail::convert<double>(data.at(i, 21));
    sso.vy_gaia = pH::csv::detail::convert<double>(data.at(i, 22));
    sso.vz_gaia = pH::csv::detail::convert<double>(data.at(i, 23));
    sso.position_angle_scan = pH::csv::detail::convert<double>(data.at(i, 24));
    sso.level_of_confidence = pH::csv::detail::convert<double>(data.at(i, 25));
    return sso;
}

SSO convertFromVec(const std::vector<std::string>& row) {
    SSO sso;
    sso.solution_id = row.at(0);
    sso.source_id = row.at(1);
    sso.observation_id = row.at(2);
    sso.number_mp = pH::csv::detail::convert<size_t>(row.at(3));
    sso.epoch = pH::csv::detail::convert<double>(row.at(4));
    sso.epoch_err = pH::csv::detail::convert<double>(row.at(5));
    sso.epoch_utc = pH::csv::detail::convert<double>(row.at(6));
    sso.ra = pH::csv::detail::convert<double>(row.at(7));
    sso.dec = pH::csv::detail::convert<double>(row.at(8));
    sso.ra_error_systematic = pH::csv::detail::convert<double>(row.at(9));
    sso.dec_error_systematic = pH::csv::detail::convert<double>(row.at(10));
    sso.ra_dec_correlation_systematic = pH::csv::detail::convert<double>(row.at(11));
    sso.ra_error_random = pH::csv::detail::convert<double>(row.at(12));
    sso.dec_error_random = pH::csv::detail::convert<double>(row.at(13));
    sso.ra_dec_correlation_random = pH::csv::detail::convert<double>(row.at(14));
    sso.g_mag = row.at(15).empty() ? 0.0 : pH::csv::detail::convert<double>(row.at(15));
    sso.g_flux = row.at(16).empty() ? 0.0 : pH::csv::detail::convert<double>(row.at(16));
    sso.g_flux_error = row.at(17).empty() ? 0.0 : pH::csv::detail::convert<double>(row.at(17));
    sso.x_gaia = pH::csv::detail::convert<double>(row.at(18));
    sso.y_gaia = pH::csv::detail::convert<double>(row.at(19));
    sso.z_gaia = pH::csv::detail::convert<double>(row.at(20));
    sso.vx_gaia = pH::csv::detail::convert<double>(row.at(21));
    sso.vy_gaia = pH::csv::detail::convert<double>(row.at(22));
    sso.vz_gaia = pH::csv::detail::convert<double>(row.at(23));
    sso.position_angle_scan = pH::csv::detail::convert<double>(row.at(24));
    sso.level_of_confidence = pH::csv::detail::convert<double>(row.at(25));
    return sso;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        throw std::runtime_error("No mode specified");
    }
    std::vector<SSO> ssos;
    int mode = std::stoi(argv[1]);
    if (mode == 0 || mode == -1) {
        auto start = std::chrono::high_resolution_clock::now();
        pH::csv::mapped data("test_data/SsoObservation.csv");
        for (size_t row = 0; row < data.rows(); row++) {
          ssos.push_back(convertFromMapped(data, row));
        }
        logPerf("pH::csv::mapped", start);
    }
    if (mode == 1 || mode == -1) {
        auto start = std::chrono::high_resolution_clock::now();
        pH::csv::flat data("test_data/SsoObservation_no_header.csv");
        for (size_t row = 0; row < data.rows(); row++) {
          ssos.push_back(convertFromFlat(data, row));
        }
        logPerf("pH::csv::flat", start);
    }
    if (mode == 2 || mode == -1) {
        auto start = std::chrono::high_resolution_clock::now();
        pH::csv::streamRows("test_data/SsoObservation.csv", [&ssos] (const pH::csv::mapped_row& row) {
            ssos.push_back(convertFromDynamic(row));
        });
        logPerf("pH::csv::streamRows", start);
    }
    if (mode == 3 || mode == -1) {
        auto start = std::chrono::high_resolution_clock::now();
        pH::csv::streamRows("test_data/SsoObservation_no_header.csv", [&ssos] (const std::vector<std::string>& row) {
            ssos.push_back(convertFromVec(row));
        });
        logPerf("pH::csv::streamRows (no_header)", start);
    }
    if (mode == 4 || mode == -1) {
        auto start = std::chrono::high_resolution_clock::now();
        std::mutex mut;
        pH::csv::streamRowsThreaded("test_data/SsoObservation.csv", 3, [&ssos, &mut] (const pH::csv::mapped_row& row) {
          auto sso = convertFromDynamic(row);
          std::lock_guard<std::mutex> lock(mut);
          ssos.push_back(std::move(sso));
        });
        logPerf("pH::csv::streamRowsThreaded", start);
    }
    if (mode == 5 || mode == -1) {
        auto start = std::chrono::high_resolution_clock::now();
        std::mutex mut;
        pH::csv::streamRowsThreaded("test_data/SsoObservation_no_header.csv", 1, [&ssos, &mut] (const std::vector<std::string>& row) {
          auto sso = convertFromVec(row);
          std::lock_guard<std::mutex> lock(mut);
          ssos.push_back(std::move(sso));
        });
        logPerf("pH::csv::streamRowsThreaded (no_header)", start);
    }
}
