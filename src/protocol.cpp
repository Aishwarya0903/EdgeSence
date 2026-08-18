#include "edgesense/protocol.hpp"
#include <algorithm>
#include <cstring>

namespace edgesense {
namespace {
void put16(std::vector<uint8_t>& out, uint16_t x) { out.push_back(x & 0xff); out.push_back(x >> 8); }
void put32(std::vector<uint8_t>& out, uint32_t x) { for (int i=0;i<4;++i) out.push_back((x >> (8*i)) & 0xff); }
void put64(std::vector<uint8_t>& out, uint64_t x) { for (int i=0;i<8;++i) out.push_back((x >> (8*i)) & 0xff); }
uint16_t get16(const std::vector<uint8_t>& b, size_t p) { return b[p] | (uint16_t(b[p+1]) << 8); }
uint32_t get32(const std::vector<uint8_t>& b, size_t p) { uint32_t v=0; for(int i=0;i<4;++i)v|=uint32_t(b[p+i])<<(8*i); return v; }
uint64_t get64(const std::vector<uint8_t>& b, size_t p) { uint64_t v=0; for(int i=0;i<8;++i)v|=uint64_t(b[p+i])<<(8*i); return v; }
float getfloat(const std::vector<uint8_t>& b, size_t p) { uint32_t bits=get32(b,p); float v; std::memcpy(&v,&bits,4); return v; }
void putfloat(std::vector<uint8_t>& out, float v) { uint32_t bits; std::memcpy(&bits,&v,4); put32(out,bits); }
}
uint16_t crc16_ccitt(const std::vector<uint8_t>& bytes) { uint16_t crc=0xffff; for(uint8_t b:bytes){crc^=uint16_t(b)<<8;for(int i=0;i<8;++i)crc=(crc&0x8000)?uint16_t((crc<<1)^0x1021):uint16_t(crc<<1);}return crc; }
std::vector<uint8_t> encode(const Telemetry& t) { std::vector<uint8_t> out={0xaa,0x55,kVersion,kTelemetryType,26,0}; put16(out,t.node_id);put32(out,t.sequence);put64(out,t.timestamp_ms);putfloat(out,t.temperature_c);putfloat(out,t.humidity_pct);putfloat(out,t.vibration_g); std::vector<uint8_t> crc_input(out.begin()+2,out.end()); put16(out,crc16_ccitt(crc_input)); return out; }
std::vector<Telemetry> FrameParser::feed(const uint8_t* data, size_t size) { buffer_.insert(buffer_.end(),data,data+size); std::vector<Telemetry> result; while(true){auto pos=std::search(buffer_.begin(),buffer_.end(),std::begin("\xaa\x55") , std::end("\xaa\x55")-1); if(pos==buffer_.end()){if(buffer_.size()>1)buffer_.erase(buffer_.begin(),buffer_.end()-1);break;} buffer_.erase(buffer_.begin(),pos); if(buffer_.size()<6)break; uint16_t len=get16(buffer_,4); if(buffer_[2]!=kVersion||buffer_[3]!=kTelemetryType||len!=26){buffer_.erase(buffer_.begin());continue;} size_t total=6+len+2; if(buffer_.size()<total)break; std::vector<uint8_t> body(buffer_.begin()+2,buffer_.begin()+6+len); if(crc16_ccitt(body)!=get16(buffer_,6+len)){buffer_.erase(buffer_.begin());continue;} result.push_back({get16(buffer_,6),get32(buffer_,8),get64(buffer_,12),getfloat(buffer_,20),getfloat(buffer_,24),getfloat(buffer_,28)}); buffer_.erase(buffer_.begin(),buffer_.begin()+total);} return result; }
}  // namespace edgesense
