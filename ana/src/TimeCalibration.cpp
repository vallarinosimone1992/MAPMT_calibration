#include "mapmt/TimeCalibration.hpp"

#ifndef MAPMT_ENABLE_ROOT
#define MAPMT_ENABLE_ROOT 1
#endif

#if MAPMT_ENABLE_ROOT
#include <TFile.h>
#include <TH1D.h>
#include <TTree.h>
#endif

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>

namespace mapmt {

namespace {

std::vector<double> parseNumbers(std::string line) {
  const auto comment = line.find('#');
  if (comment != std::string::npos) line.erase(comment);
  std::replace(line.begin(), line.end(), ',', ' ');
  std::istringstream input(line);
  std::vector<double> values;
  double value = 0.0;
  while (input >> value) values.push_back(value);
  return values;
}

double meanOf(const std::vector<double>& values) {
  if (values.empty()) return 0.0;
  return std::accumulate(values.begin(), values.end(), 0.0) / static_cast<double>(values.size());
}

double rmsOf(const std::vector<double>& values, double mean) {
  if (values.empty()) return 0.0;
  double sum = 0.0;
  for (double value : values) sum += (value - mean) * (value - mean);
  return std::sqrt(sum / static_cast<double>(values.size()));
}

bool isIntegerValued(double value) {
  return std::isfinite(value) && std::floor(value) == value;
}

int checkedInt(double value, const std::string& field, int lineNumber) {
  if (!isIntegerValued(value)) {
    throw std::runtime_error(field + " must be an integer at line " + std::to_string(lineNumber));
  }
  if (value < static_cast<double>(std::numeric_limits<int>::min()) ||
      value > static_cast<double>(std::numeric_limits<int>::max())) {
    throw std::runtime_error(field + " is out of integer range at line " +
                             std::to_string(lineNumber));
  }
  return static_cast<int>(value);
}

#if MAPMT_ENABLE_ROOT
std::string timeHistName(Address address) {
  char name[128];
  std::snprintf(name, sizeof(name), "hTime_%d_%02d_%d_%02d", address.slot, address.fiber,
                address.asic, address.channel);
  return name;
}
#endif

std::string jsonEscape(const std::string& value) {
  std::ostringstream out;
  for (unsigned char c : value) {
    switch (c) {
      case '"':
        out << "\\\"";
        break;
      case '\\':
        out << "\\\\";
        break;
      case '\b':
        out << "\\b";
        break;
      case '\f':
        out << "\\f";
        break;
      case '\n':
        out << "\\n";
        break;
      case '\r':
        out << "\\r";
        break;
      case '\t':
        out << "\\t";
        break;
      default:
        if (c < 0x20) {
          out << "\\u" << std::hex << std::setw(4) << std::setfill('0')
              << static_cast<int>(c) << std::dec << std::setfill(' ');
        } else {
          out << c;
        }
        break;
    }
  }
  return out.str();
}

void writeJsonString(std::ostream& out, const std::string& value) {
  out << '"' << jsonEscape(value) << '"';
}

void writeJsonPathOrNull(std::ostream& out, const std::filesystem::path& path) {
  if (path.empty()) {
    out << "null";
    return;
  }
  writeJsonString(out, path.string());
}

void writeJsonAddress(std::ostream& out, Address address, int indent) {
  const std::string pad(static_cast<std::size_t>(indent), ' ');
  out << "{\n"
      << pad << "  \"slot\": " << address.slot << ",\n"
      << pad << "  \"fiber\": " << address.fiber << ",\n"
      << pad << "  \"asic\": " << address.asic << ",\n"
      << pad << "  \"maroc\": " << address.channel << "\n"
      << pad << "}";
}

void ensureParentDirectory(const std::filesystem::path& path) {
  const auto parent = path.parent_path();
  if (!parent.empty()) std::filesystem::create_directories(parent);
}

std::filesystem::path resolvedJsonPath(const TimeOptions& options) {
  if (!options.jsonOutput.empty()) return options.jsonOutput;
  return options.outputDir / "mapmt_time_calibration.json";
}

void writeOptionalDouble(std::ostream& out, const std::optional<double>& value) {
  if (value) {
    out << *value;
  } else {
    out << "null";
  }
}

void writeTimeCalibrationJson(const TimeOptions& options, const TimeResult& result,
                              double referenceTime, std::optional<double> timeMin,
                              std::optional<double> timeMax) {
  const auto jsonPath = resolvedJsonPath(options);
  ensureParentDirectory(jsonPath);
  std::ofstream out(jsonPath);
  if (!out) throw std::runtime_error("cannot write JSON output: " + jsonPath.string());

  out << std::setprecision(12);
  out << "{\n";
  out << "  \"schema_version\": \"1.0\",\n";
  out << "  \"calibration_type\": \"mapmt_time_delay\",\n";
  out << "  \"is_null_calibration\": " << (options.nullCalibration ? "true" : "false") << ",\n";
  out << "  \"created_by\": ";
  writeJsonString(out, options.createdBy);
  if (options.createdAt) {
    out << ",\n  \"created_at\": ";
    writeJsonString(out, *options.createdAt);
  }
  out << ",\n";

  out << "  \"input\": {\n";
  out << "    \"file\": ";
  writeJsonPathOrNull(out, options.input);
  out << ",\n";
  out << "    \"format\": ";
  writeJsonString(out, toString(options.format));
  out << "\n";
  out << "  },\n";

  out << "  \"hardware\": {\n";
  out << "    \"config_file\": ";
  writeJsonPathOrNull(out, options.configPath);
  out << ",\n";
  out << "    \"setup_file\": ";
  writeJsonPathOrNull(out, options.setupPath);
  out << ",\n";
  out << "    \"fiber_map\": ";
  writeJsonPathOrNull(out, options.fiberMapPath);
  out << ",\n";
  out << "    \"threshold_file\": ";
  writeJsonPathOrNull(out, options.thresholdsPath);
  out << ",\n";
  out << "    \"gain_file\": ";
  writeJsonPathOrNull(out, options.gainsPath);
  out << ",\n";
  out << "    \"pmt_index_base\": 1,\n";
  out << "    \"pixel_index_base\": 1,\n";
  out << "    \"analysis_pmt_definition\": \"analysis_pmt = pmt - 1\"\n";
  out << "  },\n";

  out << "  \"method\": {\n";
  out << "    \"estimator\": \"" << (options.nullCalibration ? "none" : "mean") << "\",\n";
  out << "    \"min_entries\": " << options.minEntries << ",\n";
  out << "    \"time_range\": {\n";
  out << "      \"min\": ";
  writeOptionalDouble(out, timeMin);
  out << ",\n";
  out << "      \"max\": ";
  writeOptionalDouble(out, timeMax);
  out << ",\n";
  out << "      \"source\": \""
      << (options.nullCalibration ? "none" : (options.minTime || options.maxTime ? "user" : "data"))
      << "\"\n";
  out << "    },\n";
  out << "    \"reference_channel\": ";
  if (options.reference) {
    writeJsonAddress(out, *options.reference, 4);
  } else {
    out << "null";
  }
  out << ",\n";
  out << "    \"reference_time\": " << referenceTime << "\n";
  out << "  },\n";

  out << "  \"analysis_convention\": {\n";
  out << "    \"delay_usage\": \"time_corrected = time_raw - delay + target_time\",\n";
  out << "    \"null_calibration_usage\": \"if is_null_calibration is true, consumers should leave raw time unchanged\",\n";
  out << "    \"legacy_target_time\": " << options.legacyTargetTime << ",\n";
  out << "    \"legacy_target_time_applied\": false\n";
  out << "  },\n";

  out << "  \"channels\": [\n";
  for (std::size_t i = 0; i < result.channels.size(); ++i) {
    const auto& channel = result.channels[i];
    const int analysisPmt = channel.pmt - 1;
    out << "    {\n";
    out << "      \"slot\": " << channel.address.slot << ",\n";
    out << "      \"fiber\": " << channel.address.fiber << ",\n";
    out << "      \"asic\": " << channel.address.asic << ",\n";
    out << "      \"maroc\": " << channel.address.channel << ",\n";
    out << "      \"module\": " << channel.module << ",\n";
    out << "      \"pmt\": " << channel.pmt << ",\n";
    out << "      \"analysis_pmt\": " << analysisPmt << ",\n";
    out << "      \"pixel\": " << channel.pixel << ",\n";
    out << "      \"entries\": " << channel.entries << ",\n";
    out << "      \"delay\": " << channel.mean << ",\n";
    out << "      \"sigma\": " << channel.sigma << ",\n";
    out << "      \"offset_vs_reference\": " << channel.offset << "\n";
    out << "    }" << (i + 1 == result.channels.size() ? "\n" : ",\n");
  }
  out << "  ]\n";
  out << "}\n";
}

void writeLegacyMapmtOutput(const std::filesystem::path& output,
                            const std::vector<TimeChannelStats>& channels) {
  if (output.empty()) return;
  ensureParentDirectory(output);
  std::ofstream out(output);
  if (!out) throw std::runtime_error("cannot write legacy MAPMT output: " + output.string());
  out << std::setprecision(12);
  for (const auto& channel : channels) {
    const int analysisPmt = channel.pmt - 1;
    if (analysisPmt < 0 || channel.pixel < 1) {
      throw std::runtime_error(
          "legacy MAPMT output requires 1-based positive PMT and pixel identifiers");
    }
    const int key = analysisPmt * 256 + channel.pixel;
    out << key << ' ' << channel.mean << '\n';
  }
}

TimeResult makeNullCalibrationResult(const HardwareMap& hardware, TimeOptions options) {
  options.nullCalibration = true;
  std::filesystem::create_directories(options.outputDir);

  TimeResult result;
  if (options.writeJson) result.jsonFile = resolvedJsonPath(options);
  if (!options.legacyMapmtOutput.empty()) result.legacyMapmtFile = options.legacyMapmtOutput;

  for (const Address& address : hardware.activeChannels()) {
    const auto info = hardware.infoForAddress(address);
    if (!info) continue;

    TimeChannelStats stats;
    stats.address = address;
    stats.module = info->module;
    stats.pmt = info->pmt;
    stats.tile = info->tile;
    stats.pixel = hardware.pixelFromMaroc(address.channel);
    stats.entries = 0;
    stats.mean = 0.0;
    stats.sigma = 0.0;
    stats.offset = 0.0;
    result.channels.push_back(stats);
  }

  if (options.writeJson) {
    writeTimeCalibrationJson(options, result, 0.0, std::nullopt, std::nullopt);
  }
  writeLegacyMapmtOutput(options.legacyMapmtOutput, result.channels);
  return result;
}

}  // namespace

TimeCalibration::TimeCalibration(const HardwareMap& hardware) : hardware_(hardware) {}

TimeInputFormat parseTimeInputFormat(const std::string& value) {
  if (value == "auto") return TimeInputFormat::Auto;
  if (value == "abs" || value == "absolute" || value == "absolute-channel") {
    return TimeInputFormat::AbsoluteChannel;
  }
  if (value == "address") return TimeInputFormat::Address;
  if (value == "event-address" || value == "event") return TimeInputFormat::EventAddress;
  throw std::runtime_error("unknown time input format: " + value);
}

std::string toString(TimeInputFormat format) {
  switch (format) {
    case TimeInputFormat::Auto:
      return "auto";
    case TimeInputFormat::AbsoluteChannel:
      return "absolute-channel";
    case TimeInputFormat::Address:
      return "address";
    case TimeInputFormat::EventAddress:
      return "event-address";
  }
  return "auto";
}

TimeResult TimeCalibration::run(const TimeOptions& options) const {
  if (options.nullCalibration) return makeNullCalibrationResult(hardware_, options);

  if (options.input.empty()) throw std::runtime_error("missing time input file");
  if (options.minEntries <= 0) throw std::runtime_error("min_entries must be positive");
  if (options.bins <= 0) throw std::runtime_error("time histogram bins must be positive");
  std::filesystem::create_directories(options.outputDir);

  std::ifstream input(options.input);
  if (!input) throw std::runtime_error("cannot open time input: " + options.input.string());

  std::map<Address, std::vector<double>> times;
  std::string line;
  int lineNumber = 0;
  double globalMin = std::numeric_limits<double>::infinity();
  double globalMax = -std::numeric_limits<double>::infinity();

  while (std::getline(input, line)) {
    ++lineNumber;
    const auto values = parseNumbers(line);
    if (values.empty()) continue;

    TimeInputFormat format = options.format;
    if (format == TimeInputFormat::Auto) {
      if (values.size() == 2 || values.size() == 3) format = TimeInputFormat::AbsoluteChannel;
      else if (values.size() == 5) format = TimeInputFormat::Address;
      else if (values.size() >= 6) format = TimeInputFormat::EventAddress;
      else throw std::runtime_error("cannot infer time format at line " + std::to_string(lineNumber));
    }

    Address address;
    double time = 0.0;
    if (format == TimeInputFormat::AbsoluteChannel) {
      if (values.size() < 2) {
        throw std::runtime_error("absolute-channel time row needs at least 2 columns");
      }
      const int absoluteChannel = checkedInt(values[0], "absolute channel", lineNumber);
      if (absoluteChannel < 0) {
        throw std::runtime_error("absolute channel must be non-negative at line " +
                                 std::to_string(lineNumber));
      }
      address = hardware_.decodeAbsoluteChannel(absoluteChannel);
      time = values[1];
    } else if (format == TimeInputFormat::Address) {
      if (values.size() < 5) {
        throw std::runtime_error("address time row needs slot fiber asic channel time");
      }
      address = Address{checkedInt(values[0], "slot", lineNumber),
                        checkedInt(values[1], "fiber", lineNumber),
                        checkedInt(values[2], "asic", lineNumber),
                        checkedInt(values[3], "maroc", lineNumber)};
      time = values[4];
    } else if (format == TimeInputFormat::EventAddress) {
      if (values.size() < 6) {
        throw std::runtime_error("event-address time row needs event slot fiber asic channel time");
      }
      (void)checkedInt(values[0], "event", lineNumber);
      address = Address{checkedInt(values[1], "slot", lineNumber),
                        checkedInt(values[2], "fiber", lineNumber),
                        checkedInt(values[3], "asic", lineNumber),
                        checkedInt(values[4], "maroc", lineNumber)};
      time = values[5];
    }

    if (!std::isfinite(time)) {
      throw std::runtime_error("time must be finite at line " + std::to_string(lineNumber));
    }
    if (address.channel < 0 || address.channel >= hardware_.config().nPixels) {
      throw std::runtime_error("MAROC channel out of range at line " + std::to_string(lineNumber));
    }
    if (!hardware_.isActive(AsicId{address.slot, address.fiber, address.asic})) continue;
    times[address].push_back(time);
    globalMin = std::min(globalMin, time);
    globalMax = std::max(globalMax, time);
  }

  if (times.empty()) {
    if (options.allowEmptyInput) return makeNullCalibrationResult(hardware_, options);
    throw std::runtime_error("no decoded TDC hits matched the hardware map");
  }

  const double timeMin = options.minTime.value_or(globalMin);
  const double timeMax = options.maxTime.value_or(globalMax);
  if (!(timeMax > timeMin)) throw std::runtime_error("invalid time histogram range");

  std::map<Address, std::vector<double>> selectedTimes;
  std::map<Address, double> means;
  for (const auto& [address, values] : times) {
    auto& selected = selectedTimes[address];
    for (double value : values) {
      if (value >= timeMin && value <= timeMax) selected.push_back(value);
    }
    if (static_cast<int>(selected.size()) >= options.minEntries) means[address] = meanOf(selected);
  }
  if (means.empty()) {
    if (options.allowEmptyInput) return makeNullCalibrationResult(hardware_, options);
    throw std::runtime_error("no channel has enough entries for time calibration");
  }

  double referenceMean = 0.0;
  if (options.reference) {
    const auto found = means.find(*options.reference);
    if (found == means.end()) throw std::runtime_error("reference channel has insufficient entries");
    referenceMean = found->second;
  } else {
    for (const auto& [address, mean] : means) referenceMean += mean;
    referenceMean /= static_cast<double>(means.size());
  }

  TimeResult result;
  result.offsetsCsv = options.outputDir / "time_offsets.csv";
  if (options.writeJson) result.jsonFile = resolvedJsonPath(options);
  if (!options.legacyMapmtOutput.empty()) result.legacyMapmtFile = options.legacyMapmtOutput;

#if MAPMT_ENABLE_ROOT
  const bool writeRoot = options.writeRoot;
#else
  const bool writeRoot = false;
  if (options.writeRoot) {
    std::cerr << "warning: ROOT support is disabled in this build; skipping time_calibration.root\n";
  }
#endif
  if (writeRoot) result.rootFile = options.outputDir / "time_calibration.root";

  std::ofstream csv(result.offsetsCsv);
  if (!csv) throw std::runtime_error("cannot write time offsets CSV: " + result.offsetsCsv.string());
  csv << "slot,fiber,asic,maroc,pixel,module,pmt,tile,entries,mean_time,sigma_time,offset\n";

#if MAPMT_ENABLE_ROOT
  std::unique_ptr<TFile> root;
  std::unique_ptr<TTree> tree;
  int outSlot = 0;
  int outFiber = 0;
  int outAsic = 0;
  int outMaroc = 0;
  int outPixel = 0;
  int outModule = 0;
  int outPmt = 0;
  int outTile = 0;
  int outEntries = 0;
  double outMean = 0.0;
  double outSigma = 0.0;
  double outOffset = 0.0;
  if (writeRoot) {
    root = std::make_unique<TFile>(result.rootFile.string().c_str(), "RECREATE");
    tree = std::make_unique<TTree>("time_offsets", "MAPMT time calibration offsets");
    tree->SetDirectory(nullptr);
    tree->Branch("slot", &outSlot);
    tree->Branch("fiber", &outFiber);
    tree->Branch("asic", &outAsic);
    tree->Branch("maroc", &outMaroc);
    tree->Branch("pixel", &outPixel);
    tree->Branch("module", &outModule);
    tree->Branch("pmt", &outPmt);
    tree->Branch("tile", &outTile);
    tree->Branch("entries", &outEntries);
    tree->Branch("mean", &outMean);
    tree->Branch("sigma", &outSigma);
    tree->Branch("offset", &outOffset);
  }
#endif

  for (const auto& [address, values] : selectedTimes) {
    if (static_cast<int>(values.size()) < options.minEntries) continue;
    const auto info = hardware_.infoForAddress(address);
    if (!info) continue;

    const double mean = meanOf(values);
    const double sigma = rmsOf(values, mean);
    const double offset = referenceMean - mean;

#if MAPMT_ENABLE_ROOT
    if (writeRoot) {
      TH1D hist(timeHistName(address).c_str(), "", options.bins, timeMin, timeMax);
      hist.GetXaxis()->SetTitle("TDC time");
      hist.GetYaxis()->SetTitle("Entries");
      for (double value : values) hist.Fill(value);
      char title[256];
      std::snprintf(title, sizeof(title), "M%d PMT%d pixel %d slot %d fiber %d asic %d ch %d",
                    info->module, info->pmt, hardware_.pixelFromMaroc(address.channel),
                    address.slot, address.fiber, address.asic, address.channel);
      hist.SetTitle(title);
      hist.Write();
    }
#endif

    TimeChannelStats stats;
    stats.address = address;
    stats.module = info->module;
    stats.pmt = info->pmt;
    stats.tile = info->tile;
    stats.pixel = hardware_.pixelFromMaroc(address.channel);
    stats.entries = static_cast<int>(values.size());
    stats.mean = mean;
    stats.sigma = sigma;
    stats.offset = offset;
    result.channels.push_back(stats);

    csv << address.slot << ',' << address.fiber << ',' << address.asic << ',' << address.channel
        << ',' << stats.pixel << ',' << stats.module << ',' << stats.pmt << ',' << stats.tile
        << ',' << stats.entries << ',' << stats.mean << ',' << stats.sigma << ',' << stats.offset
        << '\n';

#if MAPMT_ENABLE_ROOT
    if (writeRoot && tree) {
      outSlot = address.slot;
      outFiber = address.fiber;
      outAsic = address.asic;
      outMaroc = address.channel;
      outPixel = stats.pixel;
      outModule = stats.module;
      outPmt = stats.pmt;
      outTile = stats.tile;
      outEntries = stats.entries;
      outMean = stats.mean;
      outSigma = stats.sigma;
      outOffset = stats.offset;
      tree->Fill();
    }
#endif
  }

#if MAPMT_ENABLE_ROOT
  if (writeRoot && root && tree) {
    tree->Write();
    root->Close();
  }
#endif

  if (options.writeJson) {
    writeTimeCalibrationJson(options, result, referenceMean, timeMin, timeMax);
  }
  writeLegacyMapmtOutput(options.legacyMapmtOutput, result.channels);
  return result;
}

}  // namespace mapmt
