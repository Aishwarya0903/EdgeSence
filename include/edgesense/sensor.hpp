#pragma once
#include "edgesense/protocol.hpp"

namespace edgesense {
class VirtualSensorBoard {
 public:
  Telemetry poll(uint32_t sequence) const;
};
}  // namespace edgesense
