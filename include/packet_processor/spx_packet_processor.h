#ifndef SPX_PACKET_PROCESSOR_H
#define SPX_PACKET_PROCESSOR_H

#include "RadarTypes.h"
#include "packet_processor/packet_processor.h"
#include <zlib.h>
#include <cstring>
#include <unordered_map>
#include <algorithm>
#include <memory>

const int SPX_MAX_RANGE_SAMPLES = 2048;
const int SPX_HEADER_SIZE = 72;
const int SPX_HEADER_CONTINUATION_SIZE = 24;
const int SPX_MIN_PACKET_SIZE = SPX_HEADER_SIZE + 4;
const int SPX_MIN_CONTINUATION_PACKET_SIZE = SPX_HEADER_CONTINUATION_SIZE + 4;
const int SPX_PRIMARY_SIZE = 1400;
const int SPX_CONTINUATION_SIZE = 744;

// Structure to map the header layout for easier access.
// Note: Alignment and padding might be an issue. Direct byte reading is safer.
// For simplicity, we'll define a struct to visualize the layout, but
// we'll rely on direct byte-level access for *actual* field extraction
// to handle the big-endian format and padding correctly.
struct HeaderData
{
  char identifier[4];   // 4s
  uint8_t padding1[4];  // 4x
  uint16_t seq1;        // H
  uint8_t padding2[28]; // 28x
  uint16_t seq2;        // H
  uint8_t padding3[4];  // 4x
  uint16_t azimuth;     // B (Azimuth MSB)
  // uint8_t az_lsb; // B (Azimuth LSB)
  uint8_t padding5[26]; // 26x
};

struct HeaderData_Continuation
{
  char identifier[4];   // 4s
  uint8_t padding1[4];  // 4x
  uint16_t seq1;        // H
  uint8_t padding2[14]; // 14x
};

class SpxProcessor : public AbstractPacketProcessor
{
public:
  SpxProcessor() : AbstractPacketProcessor() {}

  std::unique_ptr<RadarVideoSweep> processPacket(std::vector<uint8_t>& p) override;
  // bool getLatestVideoData(RadarVideoSweep &out);

private:
  bool decompress_video_cells(std::vector<uint8_t> compressed_data, std::vector<uint8_t> &video_cells);

  // std::mutex latestMutex;
  // std::deque<RadarVideoSweep> queue;
  size_t MAX_QUEUE_SIZE = 50;
  const double SCALE = 360.0 / 65536.0;

  std::unordered_map<uint16_t, RadarVideoSweep> primaryList, continuationList;
  std::deque<uint16_t> primaryOrder, continuationOrder;
  void removeFromDeque(std::deque<uint16_t> &deq, uint16_t seq);
  bool combineUncompress(uint16_t sequence, RadarVideoSweep& result);

  bool headerCheck(const char id[4])
  {
    return std::memcmp(id, "SPXN", 4) == 0;
  }
};

#endif // SPX_PACKET_PROCESSOR_H