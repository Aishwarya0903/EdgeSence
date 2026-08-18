#include "edgesense/sensor.hpp"
#include <chrono>
#include <cmath>
namespace edgesense {
Telemetry VirtualSensorBoard::poll(uint32_t sequence) const { auto now=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count(); float phase=static_cast<float>(sequence)*0.18f; float vibration=0.08f+0.02f*std::sin(phase); if(sequence>0 && sequence%17==0) vibration=0.55f; return {7,sequence,static_cast<uint64_t>(now),24.0f+1.2f*std::sin(phase),48.0f+3.0f*std::cos(phase),vibration}; }
}  // namespace edgesense
