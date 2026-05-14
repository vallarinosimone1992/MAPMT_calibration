#include "mapmt/DetectorConfig.hpp"

#include <TFile.h>
#include <TH1D.h>
#include <TTree.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

constexpr std::size_t kMaxEdges = 100000;

struct Args {
  explicit Args(int argc, char** argv) {
    for (int i = 0; i < argc; ++i) tokens.emplace_back(argv[i]);
  }

  bool has(const std::string& key) const {
    for (const auto& token : tokens) {
      if (token == key) return true;
    }
    return false;
  }

  std::string get(const std::string& key, const std::string& fallback = "") const {
    for (std::size_t i = 0; i + 1 < tokens.size(); ++i) {
      if (tokens[i] == key) return tokens[i + 1];
    }
    return fallback;
  }

  std::vector<std::string> tokens;
};

struct Edge {
  unsigned int slot = 0;
  unsigned int fiber = 0;
  unsigned int rawChannel = 0;
  unsigned int polarity = 0;
  unsigned int time = 0;
};

struct EventBuffer {
  unsigned int event = 0;
  double triggerTime = 0.0;
  bool valid = false;
  bool overflow = false;
  std::vector<Edge> edges;

  void reset() {
    event = 0;
    triggerTime = 0.0;
    valid = false;
    overflow = false;
    edges.clear();
  }
};

struct Options {
  std::filesystem::path input;
  std::filesystem::path outputDir = ".";
  std::filesystem::path rootOutput;
  std::filesystem::path csvOutput;
  std::filesystem::path summaryOutput;
  std::filesystem::path configPath;
  bool writeRoot = true;
  bool writeCsv = true;
  int csvEdge = 0;
  std::uint64_t maxEvents = std::numeric_limits<std::uint64_t>::max();
  double clockHz = 125e6;
  mapmt::DetectorConfig detector;
};

struct Counters {
  std::uint64_t words = 0;
  std::uint64_t parsedEvents = 0;
  std::uint64_t writtenEvents = 0;
  std::uint64_t corruptedEvents = 0;
  std::uint64_t blockWordMismatches = 0;
  std::uint64_t totalEdges = 0;
  std::uint64_t csvEdges = 0;
  double sumTime = 0.0;
  double sumCsvTime = 0.0;
};

struct RootOutput {
  explicit RootOutput(const std::filesystem::path& path)
      : file(path.string().c_str(), "RECREATE"),
        tree("data", "RICH detector TDC edge data"),
        hEdgeMultiplicity("hEdgeMultiplicity", "Edge multiplicity;edges/event;events", 10000, 0,
                          100000),
        hTdcTime("hTdcTime", "TDC time;time;edges", 8192, -0.5, 8191.5),
        hTriggerTime("hTriggerTime", "Trigger timestamp;time [s];events", 10000, 0, 10000) {
    tree.Branch("evt", &evt, "evt/i");
    tree.Branch("trigtime", &trigtime, "trigtime/D");
    tree.Branch("nedge", &nedge, "nedge/i");
    tree.Branch("slot", slot.data(), "slot[nedge]/i");
    tree.Branch("fiber", fiber.data(), "fiber[nedge]/i");
    tree.Branch("ch", ch.data(), "ch[nedge]/i");
    tree.Branch("pol", pol.data(), "pol[nedge]/i");
    tree.Branch("time", time.data(), "time[nedge]/i");
  }

  void fill(const EventBuffer& event) {
    evt = event.event;
    trigtime = event.triggerTime;
    nedge = static_cast<unsigned int>(event.edges.size());
    for (std::size_t i = 0; i < event.edges.size(); ++i) {
      slot[i] = event.edges[i].slot;
      fiber[i] = event.edges[i].fiber;
      ch[i] = event.edges[i].rawChannel;
      pol[i] = event.edges[i].polarity;
      time[i] = event.edges[i].time;
      hTdcTime.Fill(static_cast<double>(time[i]));
    }
    hEdgeMultiplicity.Fill(static_cast<double>(nedge));
    hTriggerTime.Fill(trigtime);
    tree.Fill();
  }

  void write() {
    file.cd();
    tree.Write();
    hEdgeMultiplicity.Write();
    hTdcTime.Write();
    hTriggerTime.Write();
    file.Close();
  }

  TFile file;
  TTree tree;
  TH1D hEdgeMultiplicity;
  TH1D hTdcTime;
  TH1D hTriggerTime;
  unsigned int evt = 0;
  double trigtime = 0.0;
  unsigned int nedge = 0;
  std::array<unsigned int, kMaxEdges> slot{};
  std::array<unsigned int, kMaxEdges> fiber{};
  std::array<unsigned int, kMaxEdges> ch{};
  std::array<unsigned int, kMaxEdges> pol{};
  std::array<unsigned int, kMaxEdges> time{};
};

void usage() {
  std::cout
      << "MAPMT binary TDC converter\n\n"
      << "Usage:\n"
      << "  mapmt_bin2root --input run.bin [--output-dir dir]\n"
      << "  mapmt_bin2root --input run.bin --output-root run.root --output-csv decoded_tdc.csv\n\n"
      << "Options:\n"
      << "  --config detector.conf       load electronics constants, including n_asics/n_pixels/clock_hz\n"
      << "  --output-root FILE           ROOT output compatible with dRICH raw analysis\n"
      << "  --output-csv FILE            decoded edge CSV for mapmt_calibrate time\n"
      << "  --summary FILE               text summary output\n"
      << "  --csv-edge 0|1|all           edge polarity exported to CSV, default 0\n"
      << "  --max-events N               stop after N parsed events\n"
      << "  --clock-hz HZ                override trigger timestamp clock, default 125000000\n"
      << "  --no-root                    do not write ROOT output\n"
      << "  --no-csv                     do not write CSV output\n";
}

std::filesystem::path suiteRoot() {
  const char* value = std::getenv("MAPMT_SUITE");
  if (value == nullptr || std::string(value).empty()) return {};
  return value;
}

std::filesystem::path defaultConfigPath() {
  const auto root = suiteRoot();
  if (root.empty()) return {};
  const auto path = root / "cnf/detector.conf";
  if (std::filesystem::exists(path)) return path;
  return {};
}

std::filesystem::path defaultOutputPath(const Options& options, const std::string& suffix) {
  const auto stem = options.input.stem().string();
  return options.outputDir / (stem + suffix);
}

int parseCsvEdge(const std::string& value) {
  if (value == "all") return -1;
  if (value == "0" || value == "rising") return 0;
  if (value == "1" || value == "falling") return 1;
  throw std::runtime_error("bad --csv-edge value: " + value);
}

Options parseOptions(const Args& args) {
  Options options;
  options.input = args.get("--input");
  if (options.input.empty()) throw std::runtime_error("missing required option --input");
  if (!args.get("--output-dir").empty()) options.outputDir = args.get("--output-dir");
  if (!args.get("--config").empty()) {
    options.configPath = args.get("--config");
  } else {
    options.configPath = defaultConfigPath();
  }
  if (!options.configPath.empty()) {
    options.detector = mapmt::loadDetectorConfig(options.configPath, options.detector);
    options.clockHz = options.detector.clockHz;
  }
  if (!args.get("--clock-hz").empty()) {
    options.clockHz = std::stod(args.get("--clock-hz"));
  }
  if (options.clockHz <= 0.0) throw std::runtime_error("clock frequency must be positive");
  if (options.detector.nPixels <= 0) throw std::runtime_error("n_pixels must be positive");
  if (options.detector.nAsics <= 0) throw std::runtime_error("n_asics must be positive");
  if (!args.get("--max-events").empty()) {
    options.maxEvents = static_cast<std::uint64_t>(std::stoull(args.get("--max-events")));
  }
  if (!args.get("--csv-edge").empty()) options.csvEdge = parseCsvEdge(args.get("--csv-edge"));
  options.writeRoot = !args.has("--no-root");
  options.writeCsv = !args.has("--no-csv");
  options.rootOutput = args.get("--output-root");
  options.csvOutput = args.get("--output-csv");
  options.summaryOutput = args.get("--summary");
  if (options.rootOutput.empty()) options.rootOutput = defaultOutputPath(options, ".root");
  if (options.csvOutput.empty()) options.csvOutput = defaultOutputPath(options, "_tdc.csv");
  if (options.summaryOutput.empty()) options.summaryOutput = defaultOutputPath(options, "_summary.txt");
  return options;
}

void ensureParentDirectory(const std::filesystem::path& path) {
  const auto parent = path.parent_path();
  if (!parent.empty()) std::filesystem::create_directories(parent);
}

bool acceptCsvEdge(int filter, const Edge& edge) {
  return filter < 0 || static_cast<int>(edge.polarity) == filter;
}

void writeCsvHeader(std::ostream& out, const Options& options) {
  out << "# Decoded MAPMT TDC edges from " << options.input << '\n';
  out << "# Compatible with: mapmt_calibrate time --format event-address --input <this file>\n";
  out << "# event slot fiber asic maroc time edge trigtime raw_channel\n";
}

void writeCsvRows(std::ostream& out, const Options& options, const EventBuffer& event,
                  Counters& counters) {
  for (const auto& edge : event.edges) {
    if (!acceptCsvEdge(options.csvEdge, edge)) continue;
    const int rawChannel = static_cast<int>(edge.rawChannel);
    const int asic = rawChannel / options.detector.nPixels;
    const int maroc = rawChannel % options.detector.nPixels;
    if (asic < 0 || asic >= options.detector.nAsics || maroc < 0 ||
        maroc >= options.detector.nPixels) {
      continue;
    }
    out << event.event << ' ' << edge.slot << ' ' << edge.fiber << ' ' << asic << ' ' << maroc
        << ' ' << edge.time << ' ' << edge.polarity << ' ' << event.triggerTime << ' '
        << edge.rawChannel << '\n';
    ++counters.csvEdges;
    counters.sumCsvTime += static_cast<double>(edge.time);
  }
}

void writeSummary(const Options& options, const Counters& counters) {
  ensureParentDirectory(options.summaryOutput);
  std::ofstream out(options.summaryOutput);
  if (!out) throw std::runtime_error("cannot write summary: " + options.summaryOutput.string());
  out << "Input: " << options.input << '\n';
  if (!options.configPath.empty()) out << "Config: " << options.configPath << '\n';
  if (options.writeRoot) out << "ROOT output: " << options.rootOutput << '\n';
  if (options.writeCsv) out << "CSV output: " << options.csvOutput << '\n';
  out << "Read words: " << counters.words << '\n';
  out << "Parsed events: " << counters.parsedEvents << '\n';
  out << "Written events: " << counters.writtenEvents << '\n';
  out << "Corrupted events: " << counters.corruptedEvents << '\n';
  out << "Block word-count mismatches: " << counters.blockWordMismatches << '\n';
  out << "Total edges: " << counters.totalEdges << '\n';
  out << "CSV edges: " << counters.csvEdges << '\n';
  const double meanTime =
      counters.totalEdges > 0 ? counters.sumTime / static_cast<double>(counters.totalEdges) : 0.0;
  const double meanCsvTime =
      counters.csvEdges > 0 ? counters.sumCsvTime / static_cast<double>(counters.csvEdges) : 0.0;
  out << "Average edge time: " << meanTime << '\n';
  out << "Average CSV edge time: " << meanCsvTime << '\n';
  out << "Average occupancy: "
      << (counters.writtenEvents > 0
              ? static_cast<double>(counters.totalEdges) / static_cast<double>(counters.writtenEvents)
              : 0.0)
      << '\n';
}

std::uint32_t readWord(std::ifstream& input, bool& ok) {
  std::uint32_t value = 0;
  input.read(reinterpret_cast<char*>(&value), sizeof(value));
  ok = static_cast<bool>(input);
  return value;
}

void finishEvent(const Options& options, EventBuffer& event, RootOutput* rootOutput,
                 std::ostream* csvOutput, Counters& counters) {
  if (!event.valid) return;
  ++counters.parsedEvents;
  if (event.overflow || event.edges.size() > kMaxEdges) {
    ++counters.corruptedEvents;
    event.reset();
    return;
  }

  counters.totalEdges += event.edges.size();
  for (const auto& edge : event.edges) counters.sumTime += static_cast<double>(edge.time);
  if (rootOutput != nullptr) rootOutput->fill(event);
  if (csvOutput != nullptr) writeCsvRows(*csvOutput, options, event, counters);
  ++counters.writtenEvents;
  event.reset();
}

Counters convert(const Options& options) {
  std::ifstream input(options.input, std::ios::binary);
  if (!input) throw std::runtime_error("cannot open input binary: " + options.input.string());

  std::unique_ptr<RootOutput> rootOutput;
  if (options.writeRoot) {
    ensureParentDirectory(options.rootOutput);
    rootOutput = std::make_unique<RootOutput>(options.rootOutput);
  }

  std::ofstream csvOutput;
  if (options.writeCsv) {
    ensureParentDirectory(options.csvOutput);
    csvOutput.open(options.csvOutput);
    if (!csvOutput) throw std::runtime_error("cannot write CSV output: " + options.csvOutput.string());
    writeCsvHeader(csvOutput, options);
  }

  Counters counters;
  EventBuffer event;
  int currentSlot = -1;
  int currentFiber = -1;
  int blockWordCount = 0;
  int timeHigh = 0;
  std::uint64_t timestamp = 0;
  int tag = 0;
  int tagIndex = 0;

  while (true) {
    bool ok = false;
    const std::uint32_t value = readWord(input, ok);
    if (!ok) break;
    ++counters.words;
    ++blockWordCount;

    if (value & 0x80000000U) {
      tag = static_cast<int>((value >> 27U) & 0xFU);
      tagIndex = 0;
    } else {
      ++tagIndex;
    }

    switch (tag) {
      case 0: {
        currentSlot = static_cast<int>((value >> 22U) & 0x1FU);
        blockWordCount = 1;
        break;
      }
      case 1: {
        const int trailerWords = static_cast<int>((value >> 0U) & 0x3FFFFFU);
        currentSlot = static_cast<int>((value >> 22U) & 0x1FU);
        if (trailerWords != blockWordCount) ++counters.blockWordMismatches;
        blockWordCount = 0;
        break;
      }
      case 2: {
        const int slot = static_cast<int>((value >> 22U) & 0x1FU);
        const unsigned int trigger = static_cast<unsigned int>((value >> 0U) & 0x3FFFFFU);
        currentSlot = slot;
        if (event.valid && event.event != trigger) {
          finishEvent(options, event, rootOutput.get(), options.writeCsv ? &csvOutput : nullptr,
                      counters);
          if (counters.parsedEvents >= options.maxEvents) return counters;
        }
        event.valid = true;
        event.event = trigger;
        break;
      }
      case 3: {
        if (tagIndex == 0) {
          timeHigh = static_cast<int>(value & 0xFFFFFFU);
          timestamp = static_cast<std::uint64_t>(timeHigh);
        } else if (tagIndex == 1) {
          const int timeLow = static_cast<int>(value & 0xFFFFFFU);
          timestamp |= (static_cast<std::uint64_t>(timeLow) << 24U);
          event.triggerTime = static_cast<double>(timestamp) / options.clockHz;
        }
        break;
      }
      case 7: {
        currentFiber = static_cast<int>((value >> 22U) & 0x1FU);
        break;
      }
      case 8: {
        if (!event.valid) break;
        if (event.edges.size() >= kMaxEdges) {
          event.overflow = true;
          break;
        }
        Edge edge;
        edge.slot = static_cast<unsigned int>(std::max(0, currentSlot));
        edge.fiber = static_cast<unsigned int>(std::max(0, currentFiber));
        edge.polarity = static_cast<unsigned int>((value >> 26U) & 0x1U);
        edge.rawChannel = static_cast<unsigned int>((value >> 16U) & 0xFFU);
        edge.time = static_cast<unsigned int>((value >> 0U) & 0x7FFFU);
        event.edges.push_back(edge);
        break;
      }
      default:
        break;
    }
  }

  finishEvent(options, event, rootOutput.get(), options.writeCsv ? &csvOutput : nullptr, counters);
  if (rootOutput) rootOutput->write();
  writeSummary(options, counters);
  return counters;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc < 2 || std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h") {
      usage();
      return argc < 2 ? EXIT_FAILURE : EXIT_SUCCESS;
    }

    const Args args(argc - 1, argv + 1);
    const Options options = parseOptions(args);
    const Counters counters = convert(options);

    std::cout << "events=" << counters.writtenEvents << '\n';
    std::cout << "edges=" << counters.totalEdges << '\n';
    if (options.writeRoot) std::cout << "root=" << options.rootOutput << '\n';
    if (options.writeCsv) std::cout << "csv=" << options.csvOutput << '\n';
    std::cout << "summary=" << options.summaryOutput << '\n';
    return EXIT_SUCCESS;
  } catch (const std::exception& error) {
    std::cerr << "error: " << error.what() << '\n';
    return EXIT_FAILURE;
  }
}
