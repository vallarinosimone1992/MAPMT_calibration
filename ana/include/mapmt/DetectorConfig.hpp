#pragma once

#include <array>
#include <filesystem>
#include <string>

namespace mapmt {

struct DetectorConfig {
  int maxSlots = 32;
  int maxFibers = 32;
  int nAsics = 3;
  int nPixels = 64;
  int setupHeaderLines = 23;
  int slotBase = 3;

  double clockHz = 125e6;

  int defaultThreshold = 230;
  int defaultGain = 64;
  int thresholdOffset = 25;
  int thresholdMin = 100;
  int thresholdMax = 700;

  int flatRateMin = 400;
  int flatRateMax = 425;
  int shoulderRange = 25;

  double pedestalMeanMin = 150.0;
  double pedestalMeanMax = 220.0;
  double noisyPedestalRms = 4.0;
  double hotRateCps = 10000.0;

  std::array<int, 64> marocToPixel = {
      60, 58, 59, 57, 52, 50, 51, 49, 44, 42, 43, 41, 36, 34, 35, 33,
      28, 26, 27, 25, 20, 18, 19, 17, 12, 10, 11, 9,  4,  2,  3,  1,
      5,  7,  6,  8,  13, 15, 14, 16, 21, 23, 22, 24, 29, 31, 30, 32,
      37, 39, 38, 40, 45, 47, 46, 48, 53, 55, 54, 56, 61, 63, 62, 64};
};

DetectorConfig loadDetectorConfig(const std::filesystem::path& path, DetectorConfig base = {});

std::string trim(std::string value);

}  // namespace mapmt
