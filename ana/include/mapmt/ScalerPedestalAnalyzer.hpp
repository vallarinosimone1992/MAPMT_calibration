#pragma once

#include "mapmt/HardwareMap.hpp"

#include <filesystem>
#include <map>
#include <vector>

namespace mapmt {

struct PedestalOptions {
  std::filesystem::path input;
  std::filesystem::path outputDir = "results/pedestal";
  int thresholdOffset = -1;
  bool writeRoot = true;
};

struct ChannelStats {
  Address address;
  int module = 0;
  int pmt = 0;
  int tile = 0;
  int pixel = 0;
  int threshold = 0;
  int gain = 0;
  int nPoints = 0;
  double pedestalMean = 0.0;
  double pedestalRms = 0.0;
  double maxRate = 0.0;
  double flatRateMean = 0.0;
  double flatRateRms = 0.0;
  double shoulderRateMean = 0.0;
  double shoulderRateRms = 0.0;
};

struct PedestalResult {
  std::vector<ChannelStats> channels;
  std::filesystem::path rootFile;
  std::filesystem::path channelStatsCsv;
  std::filesystem::path chipPedestalsTxt;
  std::filesystem::path suggestedThresholdsTxt;
  std::filesystem::path skippedThresholdsTxt;
};

class ScalerPedestalAnalyzer {
 public:
  explicit ScalerPedestalAnalyzer(const HardwareMap& hardware);

  PedestalResult run(const PedestalOptions& options) const;
  static void writeThresholdsFromChipPedestals(const std::filesystem::path& chipPedestals,
                                               const std::filesystem::path& output,
                                               const DetectorConfig& config,
                                               int thresholdOffset);

 private:
  const HardwareMap& hardware_;
};

}  // namespace mapmt
