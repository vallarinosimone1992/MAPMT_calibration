#include "mapmt/DetectorConfig.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <stdexcept>
#include <string>

namespace mapmt {

std::string trim(std::string value) {
  auto isSpace = [](unsigned char c) { return std::isspace(c) != 0; };
  value.erase(value.begin(), std::find_if(value.begin(), value.end(), [&](unsigned char c) {
                return !isSpace(c);
              }));
  value.erase(std::find_if(value.rbegin(), value.rend(), [&](unsigned char c) {
                return !isSpace(c);
              }).base(),
              value.end());
  return value;
}

namespace {

void setInt(const std::string& key, const std::string& value, DetectorConfig& config) {
  const int parsed = std::stoi(value);
  if (key == "max_slots") config.maxSlots = parsed;
  else if (key == "max_fibers") config.maxFibers = parsed;
  else if (key == "n_asics") config.nAsics = parsed;
  else if (key == "n_pixels") config.nPixels = parsed;
  else if (key == "setup_header_lines") config.setupHeaderLines = parsed;
  else if (key == "slot_base") config.slotBase = parsed;
  else if (key == "default_threshold") config.defaultThreshold = parsed;
  else if (key == "default_gain") config.defaultGain = parsed;
  else if (key == "threshold_offset") config.thresholdOffset = parsed;
  else if (key == "threshold_min") config.thresholdMin = parsed;
  else if (key == "threshold_max") config.thresholdMax = parsed;
  else if (key == "flat_rate_min") config.flatRateMin = parsed;
  else if (key == "flat_rate_max") config.flatRateMax = parsed;
  else if (key == "shoulder_range") config.shoulderRange = parsed;
  else if (key == "min_pedestal_channels_per_asic") config.minPedestalChannelsPerAsic = parsed;
  else throw std::runtime_error("unknown integer config key: " + key);
}

void setDouble(const std::string& key, const std::string& value, DetectorConfig& config) {
  const double parsed = std::stod(value);
  if (key == "clock_hz") config.clockHz = parsed;
  else if (key == "pedestal_mean_min") config.pedestalMeanMin = parsed;
  else if (key == "pedestal_mean_max") config.pedestalMeanMax = parsed;
  else if (key == "noisy_pedestal_rms") config.noisyPedestalRms = parsed;
  else if (key == "hot_rate_cps") config.hotRateCps = parsed;
  else throw std::runtime_error("unknown floating config key: " + key);
}

}  // namespace

DetectorConfig loadDetectorConfig(const std::filesystem::path& path, DetectorConfig base) {
  if (path.empty()) return base;

  std::ifstream input(path);
  if (!input) {
    throw std::runtime_error("cannot open detector config: " + path.string());
  }

  std::string line;
  int lineNumber = 0;
  while (std::getline(input, line)) {
    ++lineNumber;
    const auto comment = line.find('#');
    if (comment != std::string::npos) line.erase(comment);
    line = trim(line);
    if (line.empty()) continue;

    const auto equal = line.find('=');
    if (equal == std::string::npos) {
      throw std::runtime_error("bad config line " + std::to_string(lineNumber) + ": " + line);
    }
    std::string key = trim(line.substr(0, equal));
    std::string value = trim(line.substr(equal + 1));

    std::replace(key.begin(), key.end(), '-', '_');
    if (key == "clock_hz" || key == "pedestal_mean_min" || key == "pedestal_mean_max" ||
        key == "noisy_pedestal_rms" || key == "hot_rate_cps") {
      setDouble(key, value, base);
    } else {
      setInt(key, value, base);
    }
  }

  if (base.nPixels != 64) {
    throw std::runtime_error("only 64-channel MAPMT/MAROC mapping is currently supported");
  }
  if (base.thresholdMax <= base.thresholdMin) {
    throw std::runtime_error("threshold_max must be larger than threshold_min");
  }
  return base;
}

}  // namespace mapmt
