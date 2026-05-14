#pragma once

#include "mapmt/HardwareMap.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace mapmt {

enum class TimeInputFormat {
  Auto,
  AbsoluteChannel,
  Address,
  EventAddress,
};

struct TimeOptions {
  std::filesystem::path input;
  std::filesystem::path outputDir = "results/time";
  TimeInputFormat format = TimeInputFormat::Auto;
  std::optional<Address> reference;
  int minEntries = 20;
  int bins = 400;
  std::optional<double> minTime;
  std::optional<double> maxTime;
  bool nullCalibration = false;
  bool allowEmptyInput = false;
  bool writeRoot = true;
  bool writeJson = true;
  std::filesystem::path jsonOutput;
  std::filesystem::path legacyMapmtOutput;
  std::string createdBy = "mapmt_calibrate time";
  std::optional<std::string> createdAt;
  std::filesystem::path configPath;
  std::filesystem::path setupPath;
  std::filesystem::path fiberMapPath;
  std::filesystem::path thresholdsPath;
  std::filesystem::path gainsPath;
  double legacyTargetTime = 390.0;
};

struct TimeChannelStats {
  Address address;
  int module = 0;
  int pmt = 0;
  int tile = 0;
  int pixel = 0;
  int entries = 0;
  double mean = 0.0;
  double sigma = 0.0;
  double offset = 0.0;
};

struct TimeResult {
  std::vector<TimeChannelStats> channels;
  std::filesystem::path rootFile;
  std::filesystem::path offsetsCsv;
  std::filesystem::path jsonFile;
  std::filesystem::path legacyMapmtFile;
};

class TimeCalibration {
 public:
  explicit TimeCalibration(const HardwareMap& hardware);
  TimeResult run(const TimeOptions& options) const;

 private:
  const HardwareMap& hardware_;
};

TimeInputFormat parseTimeInputFormat(const std::string& value);
std::string toString(TimeInputFormat format);

}  // namespace mapmt
