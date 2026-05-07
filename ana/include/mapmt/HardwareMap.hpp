#pragma once

#include "mapmt/DetectorConfig.hpp"

#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <vector>

namespace mapmt {

struct Address {
  int slot = 0;
  int fiber = 0;
  int asic = 0;
  int channel = 0;

  auto key() const { return std::tie(slot, fiber, asic, channel); }
  bool operator<(const Address& other) const { return key() < other.key(); }
  bool sameAsic(const Address& other) const {
    return slot == other.slot && fiber == other.fiber && asic == other.asic;
  }
};

struct AsicId {
  int slot = 0;
  int fiber = 0;
  int asic = 0;

  auto key() const { return std::tie(slot, fiber, asic); }
  bool operator<(const AsicId& other) const { return key() < other.key(); }
};

struct FiberId {
  int slot = 0;
  int fiber = 0;

  auto key() const { return std::tie(slot, fiber); }
  bool operator<(const FiberId& other) const { return key() < other.key(); }
};

struct ChannelInfo {
  int slot = 0;
  int fiber = 0;
  int asic = 0;
  int module = 0;
  int pmt = 0;
  int tile = 0;
};

class HardwareMap {
 public:
  explicit HardwareMap(DetectorConfig config = {});

  void loadSetup(const std::filesystem::path& path);
  void loadFiberMap(const std::filesystem::path& path);
  void loadThresholds(const std::filesystem::path& path);
  void loadGains(const std::filesystem::path& path);

  const DetectorConfig& config() const { return config_; }

  std::optional<ChannelInfo> infoForAsic(AsicId id) const;
  std::optional<ChannelInfo> infoForAddress(Address address) const;
  bool isActive(AsicId id) const;
  std::vector<AsicId> activeAsics() const;
  std::vector<Address> activeChannels() const;

  Address decodeAbsoluteChannel(int absoluteChannel) const;
  int encodeAbsoluteChannel(Address address) const;
  int pixelFromMaroc(int marocChannel) const;
  int marocFromPixel(int pixel) const;

  int threshold(AsicId id) const;
  int gain(Address address) const;

  std::string summary() const;

 private:
  DetectorConfig config_;
  std::set<FiberId> setupFibers_;
  std::map<AsicId, ChannelInfo> channels_;
  std::map<AsicId, int> thresholds_;
  std::map<Address, int> gains_;
};

}  // namespace mapmt
