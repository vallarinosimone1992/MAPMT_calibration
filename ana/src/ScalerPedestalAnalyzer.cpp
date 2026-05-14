#include "mapmt/ScalerPedestalAnalyzer.hpp"

#include <TFile.h>
#include <TH1D.h>
#include <TTree.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>

namespace mapmt {

namespace {

struct WeightedMoments {
  double sumW = 0.0;
  double sumXW = 0.0;
  double sumX2W = 0.0;

  void add(double x, double weight) {
    if (weight <= 0.0) return;
    sumW += weight;
    sumXW += x * weight;
    sumX2W += x * x * weight;
  }

  double mean() const {
    if (sumW <= 0.0) return 0.0;
    return sumXW / sumW;
  }

  double rms() const {
    if (sumW <= 0.0) return 0.0;
    const double mu = mean();
    const double variance = std::max(0.0, sumX2W / sumW - mu * mu);
    return std::sqrt(variance);
  }
};

struct PlainMoments {
  std::vector<double> values;

  void add(double value) { values.push_back(value); }

  double mean() const {
    if (values.empty()) return 0.0;
    return std::accumulate(values.begin(), values.end(), 0.0) /
           static_cast<double>(values.size());
  }

  double rms() const {
    if (values.empty()) return 0.0;
    const double mu = mean();
    double sum = 0.0;
    for (double value : values) sum += (value - mu) * (value - mu);
    return std::sqrt(sum / static_cast<double>(values.size()));
  }
};

struct ChipAccumulator {
  AsicId id;
  int module = 0;
  int pmt = 0;
  double sumMean = 0.0;
  double sumRms2 = 0.0;
  int n = 0;
};

void ensureOutputDir(const std::filesystem::path& path) {
  std::filesystem::create_directories(path);
  if (!std::filesystem::is_directory(path)) {
    throw std::runtime_error("cannot create output directory: " + path.string());
  }
}

std::string histName(Address address) {
  char name[128];
  std::snprintf(name, sizeof(name), "hCount_%d_%02d_%d_%02d", address.slot, address.fiber,
                address.asic, address.channel);
  return name;
}

std::vector<long long> parseScalars(std::string line) {
  const auto comment = line.find('#');
  if (comment != std::string::npos) line.erase(comment);
  std::istringstream input(line);
  std::vector<long long> values;
  long long value = 0;
  while (input >> value) values.push_back(value);
  return values;
}

}  // namespace

ScalerPedestalAnalyzer::ScalerPedestalAnalyzer(const HardwareMap& hardware) : hardware_(hardware) {}

PedestalResult ScalerPedestalAnalyzer::run(const PedestalOptions& options) const {
  if (options.input.empty()) throw std::runtime_error("missing pedestal input file");
  ensureOutputDir(options.outputDir);

  const auto& config = hardware_.config();
  const int thresholdOffset =
      options.thresholdOffset >= 0 ? options.thresholdOffset : config.thresholdOffset;

  std::ifstream input(options.input);
  if (!input) throw std::runtime_error("cannot open scaler input: " + options.input.string());

  std::map<Address, std::vector<std::pair<int, double>>> rates;
  const auto activeAsics = hardware_.activeAsics();
  if (activeAsics.empty()) {
    throw std::runtime_error("no active ASICs found; check setup and fiber-map configuration");
  }
  int nBlocks = 0;
  int badReferenceBlocks = 0;
  int currentThreshold = 0;
  bool currentGoodReference = false;
  double currentDuration = 0.0;
  std::array<long long, 4> header{};
  int headerCount = 0;
  std::string line;

  while (std::getline(input, line)) {
    const auto values = parseScalars(line);
    if (values.empty()) continue;

    if (values.size() == 1) {
      if (headerCount >= static_cast<int>(header.size())) {
        throw std::runtime_error("bad scaler header in " + options.input.string());
      }
      header[headerCount++] = values[0];
      if (headerCount == static_cast<int>(header.size())) {
        currentThreshold = static_cast<int>(header[0]);
        const auto ref = static_cast<unsigned long>(header[2]);
        ++nBlocks;
        currentGoodReference = ref > 0;
        if (!currentGoodReference) ++badReferenceBlocks;
        currentDuration = currentGoodReference ? static_cast<double>(ref) / config.clockHz : 0.0;
        headerCount = 0;
      }
      continue;
    }

    if (headerCount != 0) {
      throw std::runtime_error("incomplete scaler header before data row in " +
                               options.input.string());
    }
    if (nBlocks == 0) {
      throw std::runtime_error("data row before scaler header in " + options.input.string());
    }
    if (!currentGoodReference) continue;

    const int absoluteChannel = static_cast<int>(values[0]);
    const int counts = static_cast<int>(values[1]);
    const Address address = hardware_.decodeAbsoluteChannel(absoluteChannel);
    if (!hardware_.isActive(AsicId{address.slot, address.fiber, address.asic})) continue;

    const double rate = static_cast<double>(counts) / currentDuration;
    rates[address].push_back({currentThreshold, rate});
  }

  if (headerCount != 0) {
    throw std::runtime_error("incomplete scaler header at end of " + options.input.string());
  }

  if (nBlocks == 0) {
    throw std::runtime_error("no scaler blocks found in " + options.input.string());
  }
  if (badReferenceBlocks > 0) {
    std::cerr << "warning: skipped " << badReferenceBlocks
              << " scaler blocks with zero reference clock\n";
  }

  PedestalResult result;
  result.rootFile = options.outputDir / "histo.root";
  result.channelStatsCsv = options.outputDir / "channel_stats.csv";
  result.chipPedestalsTxt = options.outputDir / "chip_pedestals.txt";
  result.suggestedThresholdsTxt = options.outputDir / "thresholds_suggested.txt";

  std::ofstream channelCsv(result.channelStatsCsv);
  std::ofstream chipTxt(result.chipPedestalsTxt);
  std::ofstream thresholdsTxt(result.suggestedThresholdsTxt);
  std::ofstream noisyTxt(options.outputDir / "noisy_channels.txt");
  std::ofstream deadTxt(options.outputDir / "dead_channels.txt");

  channelCsv << "slot,fiber,asic,maroc,pixel,module,pmt,tile,npoints,pedestal_mean_dac,"
                "pedestal_rms_dac,max_rate_cps,flat_rate_mean_cps,flat_rate_rms_cps,"
                "shoulder_rate_mean_cps,shoulder_rate_rms_cps,threshold_dac,gain\n";

  std::unique_ptr<TFile> rootFile;
  std::unique_ptr<TTree> statsTree;
  int outSlot = 0;
  int outFiber = 0;
  int outAsic = 0;
  int outMaroc = 0;
  int outPixel = 0;
  int outModule = 0;
  int outPmt = 0;
  int outTile = 0;
  int outNPoints = 0;
  int outThreshold = 0;
  int outGain = 0;
  double outPedMean = 0.0;
  double outPedRms = 0.0;
  double outMaxRate = 0.0;
  double outFlatMean = 0.0;
  double outFlatRms = 0.0;
  double outShoulderMean = 0.0;
  double outShoulderRms = 0.0;
  if (options.writeRoot) {
    rootFile = std::make_unique<TFile>(result.rootFile.string().c_str(), "RECREATE");
    statsTree = std::make_unique<TTree>("channel_stats", "MAPMT scaler pedestal calibration");
    statsTree->Branch("slot", &outSlot);
    statsTree->Branch("fiber", &outFiber);
    statsTree->Branch("asic", &outAsic);
    statsTree->Branch("maroc", &outMaroc);
    statsTree->Branch("pixel", &outPixel);
    statsTree->Branch("module", &outModule);
    statsTree->Branch("pmt", &outPmt);
    statsTree->Branch("tile", &outTile);
    statsTree->Branch("npoints", &outNPoints);
    statsTree->Branch("threshold", &outThreshold);
    statsTree->Branch("gain", &outGain);
    statsTree->Branch("pedestal_mean", &outPedMean);
    statsTree->Branch("pedestal_rms", &outPedRms);
    statsTree->Branch("max_rate", &outMaxRate);
    statsTree->Branch("flat_rate_mean", &outFlatMean);
    statsTree->Branch("flat_rate_rms", &outFlatRms);
    statsTree->Branch("shoulder_rate_mean", &outShoulderMean);
    statsTree->Branch("shoulder_rate_rms", &outShoulderRms);
  }

  std::map<AsicId, ChipAccumulator> chips;
  const int nBins = config.thresholdMax - config.thresholdMin + 1;

  for (const Address& address : hardware_.activeChannels()) {
    auto info = hardware_.infoForAddress(address);
    if (!info) continue;

    const int threshold = hardware_.threshold(AsicId{address.slot, address.fiber, address.asic});
    const auto foundRates = rates.find(address);
    const std::vector<std::pair<int, double>> empty;
    const auto& measurements = foundRates == rates.end() ? empty : foundRates->second;

    WeightedMoments pedestal;
    PlainMoments flat;
    PlainMoments shoulder;
    double maxRate = 0.0;

    TH1D* hist = nullptr;
    if (options.writeRoot) {
      hist = new TH1D(histName(address).c_str(), "", nBins, config.thresholdMin - 0.5,
                      config.thresholdMax + 0.5);
      hist->GetXaxis()->SetTitle("Threshold [DAC]");
      hist->GetYaxis()->SetTitle("Count rate [cps]");
    }

    for (const auto& [value, rate] : measurements) {
      pedestal.add(value, rate);
      maxRate = std::max(maxRate, rate);
      if (value >= config.flatRateMin && value <= config.flatRateMax) flat.add(rate);
      if (value >= threshold && value <= threshold + config.shoulderRange) shoulder.add(rate);
      if (hist) {
        const int bin = hist->FindBin(value);
        hist->SetBinContent(bin, hist->GetBinContent(bin) + rate);
      }
    }

    ChannelStats stats;
    stats.address = address;
    stats.module = info->module;
    stats.pmt = info->pmt;
    stats.tile = info->tile;
    stats.pixel = hardware_.pixelFromMaroc(address.channel);
    stats.threshold = threshold;
    stats.gain = hardware_.gain(address);
    stats.nPoints = static_cast<int>(measurements.size());
    stats.pedestalMean = pedestal.mean();
    stats.pedestalRms = pedestal.rms();
    stats.maxRate = maxRate;
    stats.flatRateMean = flat.mean();
    stats.flatRateRms = flat.rms();
    stats.shoulderRateMean = shoulder.mean();
    stats.shoulderRateRms = shoulder.rms();
    result.channels.push_back(stats);

    channelCsv << stats.address.slot << ',' << stats.address.fiber << ',' << stats.address.asic
               << ',' << stats.address.channel << ',' << stats.pixel << ',' << stats.module << ','
               << stats.pmt << ',' << stats.tile << ',' << stats.nPoints << ','
               << stats.pedestalMean << ',' << stats.pedestalRms << ',' << stats.maxRate << ','
               << stats.flatRateMean << ',' << stats.flatRateRms << ',' << stats.shoulderRateMean
               << ',' << stats.shoulderRateRms << ',' << stats.threshold << ',' << stats.gain
               << '\n';

    if (stats.nPoints == 0 || stats.maxRate <= 0.0) {
      deadTxt << stats.address.slot << ' ' << stats.address.fiber << ' ' << stats.address.asic
              << ' ' << stats.address.channel << "  " << stats.module << ' ' << stats.pmt << ' '
              << stats.pixel << '\n';
    }
    if (stats.pedestalRms > config.noisyPedestalRms) {
      noisyTxt << stats.address.slot << ' ' << stats.address.fiber << ' ' << stats.address.asic
               << ' ' << stats.address.channel << "  " << stats.module << ' ' << stats.pmt << ' '
               << stats.pixel << "  " << stats.pedestalMean << ' ' << stats.pedestalRms << '\n';
    }

    if (stats.nPoints > 0) {
      const AsicId asicId{address.slot, address.fiber, address.asic};
      auto& chip = chips[asicId];
      chip.id = asicId;
      chip.module = stats.module;
      chip.pmt = stats.pmt;
      chip.sumMean += stats.pedestalMean;
      chip.sumRms2 += stats.pedestalRms * stats.pedestalRms;
      ++chip.n;
    }

    if (options.writeRoot && hist) {
      hist->SetTitle(Form("M%d PMT%d pixel %d slot %d fiber %d asic %d ch %d", stats.module,
                          stats.pmt, stats.pixel, stats.address.slot, stats.address.fiber,
                          stats.address.asic, stats.address.channel));
      hist->Write();

      outSlot = stats.address.slot;
      outFiber = stats.address.fiber;
      outAsic = stats.address.asic;
      outMaroc = stats.address.channel;
      outPixel = stats.pixel;
      outModule = stats.module;
      outPmt = stats.pmt;
      outTile = stats.tile;
      outNPoints = stats.nPoints;
      outThreshold = stats.threshold;
      outGain = stats.gain;
      outPedMean = stats.pedestalMean;
      outPedRms = stats.pedestalRms;
      outMaxRate = stats.maxRate;
      outFlatMean = stats.flatRateMean;
      outFlatRms = stats.flatRateRms;
      outShoulderMean = stats.shoulderRateMean;
      outShoulderRms = stats.shoulderRateRms;
      statsTree->Fill();
      delete hist;
    }
  }

  for (const auto& [id, chip] : chips) {
    if (chip.n <= 0) continue;
    const double mean = chip.sumMean / static_cast<double>(chip.n);
    const double rms = std::sqrt(chip.sumRms2 / static_cast<double>(chip.n));
    const int suggestedThreshold = static_cast<int>(std::ceil(mean)) + thresholdOffset;
    chipTxt << id.slot << ' ' << std::setw(2) << id.fiber << ' ' << id.asic << "  "
            << chip.module << ' ' << std::setw(3) << chip.pmt << "  " << std::fixed
            << std::setprecision(3) << std::setw(8) << mean << ' ' << std::setw(8) << rms
            << '\n';
    thresholdsTxt << id.slot << ' ' << std::setw(2) << id.fiber << ' ' << id.asic << ' '
                  << std::setw(4) << suggestedThreshold << '\n';
  }

  if (options.writeRoot) {
    statsTree->Write();
    rootFile->Close();
  }

  return result;
}

void ScalerPedestalAnalyzer::writeThresholdsFromChipPedestals(
    const std::filesystem::path& chipPedestals, const std::filesystem::path& output,
    int thresholdOffset) {
  std::ifstream input(chipPedestals);
  if (!input) throw std::runtime_error("cannot open chip pedestal file: " + chipPedestals.string());
  std::ofstream out(output);
  if (!out) throw std::runtime_error("cannot write threshold file: " + output.string());

  int slot = 0;
  int fiber = 0;
  int asic = 0;
  int module = 0;
  int pmt = 0;
  double mean = 0.0;
  double rms = 0.0;
  while (input >> slot >> fiber >> asic >> module >> pmt >> mean >> rms) {
    const int threshold = static_cast<int>(std::ceil(mean)) + thresholdOffset;
    out << slot << ' ' << std::setw(2) << fiber << ' ' << asic << ' ' << std::setw(4)
        << threshold << '\n';
  }
}

}  // namespace mapmt
