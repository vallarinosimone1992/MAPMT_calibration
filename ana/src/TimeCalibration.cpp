#include "mapmt/TimeCalibration.hpp"

#include <TFile.h>
#include <TH1D.h>
#include <TTree.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <map>
#include <numeric>
#include <sstream>
#include <stdexcept>

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

std::string timeHistName(Address address) {
  char name[128];
  std::snprintf(name, sizeof(name), "hTime_%d_%02d_%d_%02d", address.slot, address.fiber,
                address.asic, address.channel);
  return name;
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
  if (options.input.empty()) throw std::runtime_error("missing time input file");
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
      address = hardware_.decodeAbsoluteChannel(static_cast<int>(values[0]));
      time = values[1];
    } else if (format == TimeInputFormat::Address) {
      if (values.size() < 5) {
        throw std::runtime_error("address time row needs slot fiber asic channel time");
      }
      address = Address{static_cast<int>(values[0]), static_cast<int>(values[1]),
                        static_cast<int>(values[2]), static_cast<int>(values[3])};
      time = values[4];
    } else if (format == TimeInputFormat::EventAddress) {
      if (values.size() < 6) {
        throw std::runtime_error("event-address time row needs event slot fiber asic channel time");
      }
      address = Address{static_cast<int>(values[1]), static_cast<int>(values[2]),
                        static_cast<int>(values[3]), static_cast<int>(values[4])};
      time = values[5];
    }

    if (!hardware_.isActive(AsicId{address.slot, address.fiber, address.asic})) continue;
    times[address].push_back(time);
    globalMin = std::min(globalMin, time);
    globalMax = std::max(globalMax, time);
  }

  if (times.empty()) {
    throw std::runtime_error("no decoded TDC hits matched the hardware map");
  }

  const double timeMin = options.minTime.value_or(globalMin);
  const double timeMax = options.maxTime.value_or(globalMax);
  if (!(timeMax > timeMin)) throw std::runtime_error("invalid time histogram range");

  std::map<Address, double> means;
  for (const auto& [address, values] : times) {
    if (static_cast<int>(values.size()) >= options.minEntries) means[address] = meanOf(values);
  }
  if (means.empty()) {
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
  result.rootFile = options.outputDir / "time_calibration.root";
  result.offsetsCsv = options.outputDir / "time_offsets.csv";

  std::ofstream csv(result.offsetsCsv);
  csv << "slot,fiber,asic,maroc,pixel,module,pmt,tile,entries,mean_time,sigma_time,offset\n";

  TFile root(result.rootFile.string().c_str(), "RECREATE");
  TTree tree("time_offsets", "MAPMT time calibration offsets");
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
  tree.Branch("slot", &outSlot);
  tree.Branch("fiber", &outFiber);
  tree.Branch("asic", &outAsic);
  tree.Branch("maroc", &outMaroc);
  tree.Branch("pixel", &outPixel);
  tree.Branch("module", &outModule);
  tree.Branch("pmt", &outPmt);
  tree.Branch("tile", &outTile);
  tree.Branch("entries", &outEntries);
  tree.Branch("mean", &outMean);
  tree.Branch("sigma", &outSigma);
  tree.Branch("offset", &outOffset);

  for (const auto& [address, values] : times) {
    if (static_cast<int>(values.size()) < options.minEntries) continue;
    const auto info = hardware_.infoForAddress(address);
    if (!info) continue;

    const double mean = meanOf(values);
    const double sigma = rmsOf(values, mean);
    const double offset = referenceMean - mean;

    TH1D hist(timeHistName(address).c_str(), "", options.bins, timeMin, timeMax);
    hist.GetXaxis()->SetTitle("TDC time");
    hist.GetYaxis()->SetTitle("Entries");
    for (double value : values) hist.Fill(value);
    hist.SetTitle(Form("M%d PMT%d pixel %d slot %d fiber %d asic %d ch %d", info->module,
                       info->pmt, hardware_.pixelFromMaroc(address.channel), address.slot,
                       address.fiber, address.asic, address.channel));
    hist.Write();

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
    tree.Fill();
  }

  tree.Write();
  root.Close();
  return result;
}

}  // namespace mapmt
