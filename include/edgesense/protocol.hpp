#pragma once
#include <cstdint>
#include <optional>
#include <vector>

namespace edgesense {
constexpr uint8_t kVersion = 1;
constexpr uint8_t kTelemetryType = 1;

struct Telemetry { uint16_t node_id; uint32_t sequence; uint64_t timestamp_ms; float temperature_c; float humidity_pct; float vibration_g; };

uint16_t crc16_ccitt(const std::vector<uint8_t>& bytes);
std::vector<uint8_t> encode(const Telemetry& telemetry);

class FrameParser {
 public:
  std::vector<Telemetry> feed(const uint8_t* data, size_t size);
 private:
  std::vector<uint8_t> buffer_;
};
}  // namespace edgesense
