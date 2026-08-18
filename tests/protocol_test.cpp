#include "edgesense/protocol.hpp"
#include <gtest/gtest.h>

using edgesense::Telemetry;

TEST(Protocol, CrcKnownVector) {
  std::vector<uint8_t> data={'1','2','3','4','5','6','7','8','9'};
  EXPECT_EQ(edgesense::crc16_ccitt(data), 0x29B1);
}

TEST(Protocol, EncodesAndParsesFragmentedFrame) {
  Telemetry expected{7, 12, 1700000000000ULL, 24.5f, 49.0f, 0.08f};
  auto frame = edgesense::encode(expected);
  edgesense::FrameParser parser;
  EXPECT_TRUE(parser.feed(frame.data(), 5).empty());
  auto result=parser.feed(frame.data()+5, frame.size()-5);
  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(result[0].node_id, expected.node_id);
  EXPECT_EQ(result[0].sequence, expected.sequence);
  EXPECT_FLOAT_EQ(result[0].vibration_g, expected.vibration_g);
}

TEST(Protocol, RejectsBadCrcAndResynchronizes) {
  auto bad=edgesense::encode({1,1,1,20,40,0.1f}); bad.back() ^= 0xff;
  auto good=edgesense::encode({2,2,2,21,41,0.2f});
  bad.insert(bad.end(),good.begin(),good.end());
  edgesense::FrameParser parser;
  auto readings=parser.feed(bad.data(),bad.size());
  ASSERT_EQ(readings.size(),1);
  EXPECT_EQ(readings[0].node_id,2);
}
