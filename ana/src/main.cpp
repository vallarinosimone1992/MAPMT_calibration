#include "mapmt/DetectorConfig.hpp"
#include "mapmt/HardwareMap.hpp"
#include "mapmt/ScalerPedestalAnalyzer.hpp"
#include "mapmt/TimeCalibration.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

class Args {
 public:
  Args(int argc, char** argv) {
    for (int i = 0; i < argc; ++i) tokens_.emplace_back(argv[i]);
  }

  bool has(const std::string& key) const {
    for (const auto& token : tokens_) {
      if (token == key) return true;
    }
    return false;
  }

  std::string get(const std::string& key, const std::string& fallback = "") const {
    for (std::size_t i = 0; i + 1 < tokens_.size(); ++i) {
      if (tokens_[i] == key) return tokens_[i + 1];
    }
    return fallback;
  }

 private:
  std::vector<std::string> tokens_;
};

void usage() {
  std::cout
      << "MAPMT calibration toolkit\n\n"
      << "Commands:\n"
      << "  inspect-map [--map fiber.map] [--setup setup.txt] [--thresholds threshold.txt] [--gains gain.txt]\n"
      << "  pedestal --input rich_pedestal.txt [--map fiber.map] [--setup setup.txt] --output outdir [--thresholds threshold.txt] [--gains gain.txt]\n"
      << "  thresholds --chip-pedestals chip_pedestals.txt --output threshold.txt [--offset 25]\n"
      << "  time --input decoded_tdc.txt [--map fiber.map] [--setup setup.txt] --output outdir [--format auto|abs|address|event-address]\n"
      << "  time --null-calibration [--map fiber.map] [--setup setup.txt] --output outdir\n\n"
      << "Common options:\n"
      << "  --config detector.conf     override detector constants\n"
      << "  --slot-base 3              absolute-channel decoding slot base\n"
      << "  --clock-hz 125000000       scaler reference clock\n"
      << "  --threshold-offset 25      pedestal mean offset for suggested thresholds\n\n"
      << "ROOT output options:\n"
      << "  --no-root                  skip ROOT output files\n\n"
      << "Time output options:\n"
      << "  --json-output FILE         write MAPMT JSON time calibration to FILE\n"
      << "  --legacy-mapmt-output FILE write legacy MAPMT_time_calibration.dat-style file\n"
      << "  --no-json                  disable JSON output\n"
      << "  --null-calibration         write a zero/no-op calibration without TDC input\n"
      << "  --allow-empty              fall back to a zero/no-op calibration if input has no usable hits\n\n"
      << "If MAPMT_SUITE is set, default files are read from:\n"
      << "  $MAPMT_SUITE/cnf/detector.conf\n"
      << "  $MAPMT_SUITE/cnf/maps/{setup.txt,fiber.map,threshold.txt,gain.txt}\n";
}

std::filesystem::path requiredPath(const Args& args, const std::string& key) {
  const std::string value = args.get(key);
  if (value.empty()) throw std::runtime_error("missing required option " + key);
  return value;
}

std::filesystem::path suiteRoot() {
  const char* value = std::getenv("MAPMT_SUITE");
  if (value == nullptr || std::string(value).empty()) return {};
  return value;
}

std::filesystem::path suitePath(const std::filesystem::path& relativePath) {
  const auto root = suiteRoot();
  if (root.empty()) return {};
  return root / relativePath;
}

std::filesystem::path optionalPathFromSuite(const Args& args, const std::string& key,
                                            const std::filesystem::path& suiteRelativePath) {
  const std::string explicitValue = args.get(key);
  if (!explicitValue.empty()) return explicitValue;
  const auto defaultPath = suitePath(suiteRelativePath);
  if (!defaultPath.empty() && std::filesystem::exists(defaultPath)) return defaultPath;
  return {};
}

std::filesystem::path requiredPathFromSuite(const Args& args, const std::string& key,
                                            const std::filesystem::path& suiteRelativePath) {
  auto path = optionalPathFromSuite(args, key, suiteRelativePath);
  if (!path.empty()) return path;
  if (suiteRoot().empty()) {
    throw std::runtime_error("missing required option " + key +
                             " or MAPMT_SUITE for default configuration");
  }
  throw std::runtime_error("missing required option " + key + " and default file not found: " +
                           suitePath(suiteRelativePath).string());
}

mapmt::DetectorConfig readConfig(const Args& args) {
  mapmt::DetectorConfig config;
  const auto configPath = optionalPathFromSuite(args, "--config", "cnf/detector.conf");
  if (!configPath.empty()) config = mapmt::loadDetectorConfig(configPath, config);
  if (!args.get("--slot-base").empty()) config.slotBase = std::stoi(args.get("--slot-base"));
  if (!args.get("--clock-hz").empty()) config.clockHz = std::stod(args.get("--clock-hz"));
  if (!args.get("--threshold-offset").empty()) {
    config.thresholdOffset = std::stoi(args.get("--threshold-offset"));
  }
  return config;
}

struct LoadedHardware {
  mapmt::HardwareMap hardware;
  std::filesystem::path configPath;
  std::filesystem::path setupPath;
  std::filesystem::path fiberMapPath;
  std::filesystem::path thresholdsPath;
  std::filesystem::path gainsPath;

  explicit LoadedHardware(mapmt::DetectorConfig config) : hardware(std::move(config)) {}
};

LoadedHardware loadHardware(const Args& args) {
  LoadedHardware loaded(readConfig(args));
  loaded.configPath = optionalPathFromSuite(args, "--config", "cnf/detector.conf");
  loaded.setupPath = optionalPathFromSuite(args, "--setup", "cnf/maps/setup.txt");
  loaded.fiberMapPath = requiredPathFromSuite(args, "--map", "cnf/maps/fiber.map");
  loaded.thresholdsPath = optionalPathFromSuite(args, "--thresholds", "cnf/maps/threshold.txt");
  loaded.gainsPath = optionalPathFromSuite(args, "--gains", "cnf/maps/gain.txt");

  if (!loaded.setupPath.empty()) loaded.hardware.loadSetup(loaded.setupPath);
  loaded.hardware.loadFiberMap(loaded.fiberMapPath);
  if (!loaded.thresholdsPath.empty()) loaded.hardware.loadThresholds(loaded.thresholdsPath);
  if (!loaded.gainsPath.empty()) loaded.hardware.loadGains(loaded.gainsPath);
  return loaded;
}

mapmt::Address parseAddress(const std::string& text) {
  std::string normalized = text;
  for (char& c : normalized) {
    if (c == ':' || c == ',' || c == '/') c = ' ';
  }
  std::istringstream in(normalized);
  mapmt::Address address;
  if (!(in >> address.slot >> address.fiber >> address.asic >> address.channel)) {
    throw std::runtime_error("reference must be slot:fiber:asic:channel");
  }
  return address;
}

int runInspectMap(const Args& args) {
  const auto loaded = loadHardware(args);
  std::cout << loaded.hardware.summary();
  return EXIT_SUCCESS;
}

int runPedestal(const Args& args) {
  const auto loaded = loadHardware(args);
  mapmt::PedestalOptions options;
  options.input = requiredPath(args, "--input");
  options.outputDir = requiredPath(args, "--output");
  if (!args.get("--threshold-offset").empty()) {
    options.thresholdOffset = std::stoi(args.get("--threshold-offset"));
  }
  options.writeRoot = !args.has("--no-root");

  mapmt::ScalerPedestalAnalyzer analyzer(loaded.hardware);
  const auto result = analyzer.run(options);
  std::cout << "channels=" << result.channels.size() << '\n';
  std::cout << "channel_stats=" << result.channelStatsCsv << '\n';
  std::cout << "chip_pedestals=" << result.chipPedestalsTxt << '\n';
  std::cout << "suggested_thresholds=" << result.suggestedThresholdsTxt << '\n';
  if (!result.rootFile.empty()) std::cout << "root=" << result.rootFile << '\n';
  return EXIT_SUCCESS;
}

int runThresholds(const Args& args) {
  const auto chipPedestals = requiredPath(args, "--chip-pedestals");
  const auto output = requiredPath(args, "--output");
  const int offset = std::stoi(args.get("--offset", args.get("--threshold-offset", "25")));
  mapmt::ScalerPedestalAnalyzer::writeThresholdsFromChipPedestals(chipPedestals, output, offset);
  std::cout << "thresholds=" << output << '\n';
  return EXIT_SUCCESS;
}

int runTime(const Args& args) {
  const auto loaded = loadHardware(args);
  mapmt::TimeOptions options;
  options.nullCalibration = args.has("--null-calibration");
  if (options.nullCalibration) {
    const std::string input = args.get("--input");
    if (!input.empty()) options.input = input;
  } else {
    options.input = requiredPath(args, "--input");
  }
  options.outputDir = requiredPath(args, "--output");
  options.format = mapmt::parseTimeInputFormat(args.get("--format", "auto"));
  if (!args.get("--reference").empty()) options.reference = parseAddress(args.get("--reference"));
  if (!args.get("--min-entries").empty()) options.minEntries = std::stoi(args.get("--min-entries"));
  if (!args.get("--bins").empty()) options.bins = std::stoi(args.get("--bins"));
  if (!args.get("--min-time").empty()) options.minTime = std::stod(args.get("--min-time"));
  if (!args.get("--max-time").empty()) options.maxTime = std::stod(args.get("--max-time"));
  if (!args.get("--json-output").empty()) options.jsonOutput = args.get("--json-output");
  if (!args.get("--legacy-mapmt-output").empty()) {
    options.legacyMapmtOutput = args.get("--legacy-mapmt-output");
  }
  if (!args.get("--created-at").empty()) options.createdAt = args.get("--created-at");
  if (!args.get("--legacy-target-time").empty()) {
    options.legacyTargetTime = std::stod(args.get("--legacy-target-time"));
  }
  options.allowEmptyInput = args.has("--allow-empty");
  options.writeRoot = !args.has("--no-root");
  options.writeJson = !args.has("--no-json");
  options.configPath = loaded.configPath;
  options.setupPath = loaded.setupPath;
  options.fiberMapPath = loaded.fiberMapPath;
  options.thresholdsPath = loaded.thresholdsPath;
  options.gainsPath = loaded.gainsPath;

  mapmt::TimeCalibration calibration(loaded.hardware);
  const auto result = calibration.run(options);
  std::cout << "channels=" << result.channels.size() << '\n';
  if (!result.offsetsCsv.empty()) std::cout << "offsets=" << result.offsetsCsv << '\n';
  if (!result.rootFile.empty()) std::cout << "root=" << result.rootFile << '\n';
  if (!result.jsonFile.empty()) std::cout << "json=" << result.jsonFile << '\n';
  if (!result.legacyMapmtFile.empty()) {
    std::cout << "legacy_mapmt=" << result.legacyMapmtFile << '\n';
  }
  return EXIT_SUCCESS;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc < 2 || std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h") {
      usage();
      return argc < 2 ? EXIT_FAILURE : EXIT_SUCCESS;
    }

    const std::string command = argv[1];
    Args args(argc - 2, argv + 2);
    if (command == "inspect-map") return runInspectMap(args);
    if (command == "pedestal") return runPedestal(args);
    if (command == "thresholds") return runThresholds(args);
    if (command == "time") return runTime(args);

    usage();
    std::cerr << "unknown command: " << command << '\n';
    return EXIT_FAILURE;
  } catch (const std::exception& error) {
    std::cerr << "error: " << error.what() << '\n';
    return EXIT_FAILURE;
  }
}
