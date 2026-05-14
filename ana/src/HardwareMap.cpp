#include "mapmt/HardwareMap.hpp"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace mapmt {

namespace {

std::vector<int> parseInts(std::string line) {
  const auto comment = line.find('#');
  if (comment != std::string::npos) line.erase(comment);
  std::istringstream in(line);
  std::vector<int> values;
  int value = 0;
  while (in >> value) values.push_back(value);
  return values;
}

void requireOpen(const std::ifstream& file, const std::filesystem::path& path) {
  if (!file) throw std::runtime_error("cannot open file: " + path.string());
}

}  // namespace

HardwareMap::HardwareMap(DetectorConfig config) : config_(std::move(config)) {}

void HardwareMap::loadSetup(const std::filesystem::path& path) {
  if (path.empty()) return;
  std::ifstream input(path);
  requireOpen(input, path);

  std::string line;
  int lineNumber = 0;
  while (std::getline(input, line)) {
    ++lineNumber;
    if (lineNumber <= config_.setupHeaderLines) continue;
    const auto values = parseInts(line);
    if (values.size() < 3) continue;
    if (values[2] > 0) setupFibers_.insert(FiberId{values[0], values[1]});
  }
}

void HardwareMap::loadFiberMap(const std::filesystem::path& path) {
  std::ifstream input(path);
  requireOpen(input, path);

  std::string line;
  int lineNumber = 0;
  while (std::getline(input, line)) {
    ++lineNumber;
    const auto values = parseInts(line);
    if (values.empty()) continue;
    if (values.size() < 6) {
      throw std::runtime_error("bad fiber map line " + std::to_string(lineNumber) + " in " +
                               path.string());
    }

    ChannelInfo info;
    info.slot = values[0];
    info.fiber = values[1];
    info.asic = values[2];
    info.module = values[3];
    info.pmt = values[4];
    info.tile = values[5];
    channels_[AsicId{info.slot, info.fiber, info.asic}] = info;
  }
}

void HardwareMap::loadThresholds(const std::filesystem::path& path) {
  if (path.empty()) return;
  std::ifstream input(path);
  if (!input) return;

  std::string line;
  int lineNumber = 0;
  while (std::getline(input, line)) {
    ++lineNumber;
    const auto values = parseInts(line);
    if (values.empty()) continue;
    if (values.size() < 4) {
      throw std::runtime_error("bad threshold line " + std::to_string(lineNumber) + " in " +
                               path.string());
    }
    thresholds_[AsicId{values[0], values[1], values[2]}] = values[3];
  }
}

void HardwareMap::loadGains(const std::filesystem::path& path) {
  if (path.empty()) return;
  std::ifstream input(path);
  if (!input) return;

  std::string line;
  int lineNumber = 0;
  while (std::getline(input, line)) {
    ++lineNumber;
    const auto values = parseInts(line);
    if (values.empty()) continue;
    if (values.size() < 4) {
      throw std::runtime_error("bad gain line " + std::to_string(lineNumber) + " in " +
                               path.string());
    }
    const int asic = values[2] / config_.nPixels;
    const int channel = values[2] % config_.nPixels;
    gains_[Address{values[0], values[1], asic, channel}] = values[3];
  }
}

std::optional<ChannelInfo> HardwareMap::infoForAsic(AsicId id) const {
  const auto found = channels_.find(id);
  if (found == channels_.end()) return std::nullopt;
  return found->second;
}

std::optional<ChannelInfo> HardwareMap::infoForAddress(Address address) const {
  return infoForAsic(AsicId{address.slot, address.fiber, address.asic});
}

bool HardwareMap::isActive(AsicId id) const {
  if (channels_.find(id) == channels_.end()) return false;
  if (setupFibers_.empty()) return true;
  return setupFibers_.find(FiberId{id.slot, id.fiber}) != setupFibers_.end();
}

std::vector<AsicId> HardwareMap::activeAsics() const {
  std::vector<AsicId> result;
  result.reserve(channels_.size());
  for (const auto& [id, info] : channels_) {
    if (isActive(id)) result.push_back(id);
  }
  return result;
}

std::vector<Address> HardwareMap::activeChannels() const {
  std::vector<Address> result;
  result.reserve(channels_.size() * static_cast<std::size_t>(config_.nPixels));
  for (const AsicId& id : activeAsics()) {
    for (int channel = 0; channel < config_.nPixels; ++channel) {
      result.push_back(Address{id.slot, id.fiber, id.asic, channel});
    }
  }
  return result;
}

Address HardwareMap::decodeAbsoluteChannel(int absoluteChannel) const {
  const int perFiber = config_.nAsics * config_.nPixels;
  const int perSlot = config_.maxFibers * perFiber;
  const int slot = config_.slotBase + absoluteChannel / perSlot;
  const int slotRemainder = absoluteChannel - (slot - config_.slotBase) * perSlot;
  const int fiber = slotRemainder / perFiber;
  const int fiberRemainder = slotRemainder - fiber * perFiber;
  const int asic = fiberRemainder / config_.nPixels;
  const int channel = fiberRemainder - asic * config_.nPixels;
  return Address{slot, fiber, asic, channel};
}

int HardwareMap::encodeAbsoluteChannel(Address address) const {
  return address.channel + config_.nPixels * address.asic +
         config_.nPixels * config_.nAsics * address.fiber +
         config_.nPixels * config_.nAsics * config_.maxFibers *
             (address.slot - config_.slotBase);
}

int HardwareMap::pixelFromMaroc(int marocChannel) const {
  if (marocChannel < 0 || marocChannel >= config_.nPixels) return -1;
  return config_.marocToPixel.at(static_cast<std::size_t>(marocChannel));
}

int HardwareMap::marocFromPixel(int pixel) const {
  const auto found = std::find(config_.marocToPixel.begin(), config_.marocToPixel.end(), pixel);
  if (found == config_.marocToPixel.end()) return -1;
  return static_cast<int>(std::distance(config_.marocToPixel.begin(), found));
}

int HardwareMap::threshold(AsicId id) const {
  const auto found = thresholds_.find(id);
  if (found == thresholds_.end()) return config_.defaultThreshold;
  return found->second;
}

int HardwareMap::gain(Address address) const {
  const auto found = gains_.find(address);
  if (found == gains_.end()) return config_.defaultGain;
  return found->second;
}

std::string HardwareMap::summary() const {
  std::set<int> modules;
  std::set<int> pmts;
  std::set<int> tiles;
  std::set<int> slots;
  const auto active = activeAsics();
  for (const AsicId& id : active) {
    const auto found = channels_.find(id);
    if (found == channels_.end()) continue;
    const auto& info = found->second;
    modules.insert(info.module);
    pmts.insert(info.pmt);
    tiles.insert(info.tile);
    slots.insert(info.slot);
  }

  std::ostringstream out;
  out << "mapped_asics=" << channels_.size() << '\n';
  out << "active_asics=" << active.size() << '\n';
  out << "active_channels=" << active.size() * static_cast<std::size_t>(config_.nPixels)
      << '\n';
  out << "modules=" << modules.size() << '\n';
  out << "pmts=" << pmts.size() << '\n';
  out << "tiles=" << tiles.size() << '\n';
  out << "slots=";
  bool first = true;
  for (int slot : slots) {
    if (!first) out << ',';
    out << slot;
    first = false;
  }
  out << '\n';
  out << "threshold_entries=" << thresholds_.size() << '\n';
  out << "gain_entries=" << gains_.size() << '\n';
  if (!setupFibers_.empty()) out << "setup_active_fibers=" << setupFibers_.size() << '\n';
  return out.str();
}

}  // namespace mapmt
