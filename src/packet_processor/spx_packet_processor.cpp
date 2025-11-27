#include "packet_processor/spx_packet_processor.h"

using namespace std;

/**
 * @brief Converts a 16-bit value from network (big-endian) to host endianness.
 * @param value The 16-bit value in big-endian format.
 * @return The 16-bit value in host endianness.
 */
// uint16_t ntohs_custom(uint16_t value) {
//     // This is a common way to handle network-to-host conversion in C/C++
//     // for systems that might be little-endian.
//     return (value << 8) | (value >> 8);
// }

unique_ptr<RadarVideoSweep> SpxProcessor::processPacket(vector<uint8_t>& p)
{
  if (p.size() < SPX_MIN_PACKET_SIZE)
    return NULL;

  if (p.size() == SPX_CONTINUATION_SIZE)
  {
    // uncompress continuation
    HeaderData_Continuation header;
    memcpy(&header, p.data(), SPX_HEADER_CONTINUATION_SIZE);

    if (!headerCheck(header.identifier))
      return NULL;

    vector<uint8_t> continuationData(p.begin() + SPX_HEADER_CONTINUATION_SIZE, p.end());
    uint16_t sequence = ntohs_custom(header.seq1);
    RadarVideoSweep continuationSweep;
    continuationSweep.sequence = sequence;
    continuationSweep.intensities = continuationData;

    // lock_guard<mutex> lk(latestMutex);
    if (continuationOrder.size() >= MAX_QUEUE_SIZE)
    {
      uint16_t seq = continuationOrder.back();
      continuationList.erase(seq);
      continuationOrder.pop_back();
    }
    continuationList[sequence] = continuationSweep;
    continuationOrder.push_front(sequence);
    RadarVideoSweep result;
    return combineUncompress(sequence, result) ?
      unique_ptr<RadarVideoSweep>(new RadarVideoSweep(move(result))) : 
      NULL;
  }
  else
  {
    HeaderData header;
    memcpy(&header, p.data(), sizeof(HeaderData));

    if (!headerCheck(header.identifier))
      return NULL;

    double azimuth = ntohs_custom(header.azimuth) * SCALE;

    const vector<uint8_t> data(p.begin() + SPX_HEADER_SIZE, p.end());

    uint16_t sequence = ntohs_custom(header.seq1);
    RadarVideoSweep result;
    result.sequence = sequence;
    result.azimuth = azimuth;

    bool processComplete = false;
    if (p.size() == SPX_PRIMARY_SIZE)
    {
      result.intensities = data;
      if (primaryOrder.size() >= MAX_QUEUE_SIZE)
      {
        uint16_t seq = primaryOrder.back();
        primaryList.erase(seq);
        primaryOrder.pop_back();
      }
      primaryList[sequence] = result;
      primaryOrder.push_front(sequence);
      processComplete = combineUncompress(sequence, result);
    }
    else
    {
      // Compressed SPX data
      vector<uint8_t> video_cells;
      if (!decompress_video_cells(data, video_cells))
        return NULL;

      result.intensities = video_cells;
      processComplete = true;
    }
    return processComplete ?
      unique_ptr<RadarVideoSweep>(new RadarVideoSweep(move(result))) : 
      NULL;
  }

}

bool SpxProcessor::decompress_video_cells(vector<uint8_t> compressed_data, vector<uint8_t> &video_cells)
{
  video_cells.clear();
  if (compressed_data.empty())
  {
    // No compressed data to process
    return false;
  }

  // Initialize decompression stream
  z_stream strm;
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;

  // The decompression is expected to produce MAX_RANGE_SAMPLES (2048) bytes
  // We'll use an initial buffer size and potentially resize it.
  vector<uint8_t> decompressed_buffer(SPX_MAX_RANGE_SAMPLES);
  int ret;

  // Z_OK on success
  // windowBits parameter is set to 15 (default) + 32 (to automatically detect
  // zlib or gzip headers, which is a good practice when unsure of the source).
  if (inflateInit2(&strm, 15 + 32) != Z_OK)
  {
    // printf("Failed to initialize zlib");
    return false;
  }

  strm.avail_in = compressed_data.size();
  strm.next_in = const_cast<Bytef *>(compressed_data.data());

  // Set up output buffer
  strm.avail_out = decompressed_buffer.size();
  strm.next_out = decompressed_buffer.data();

  // Decompress the data
  ret = inflate(&strm, Z_FINISH);

  // Check for decompression errors
  if (ret != Z_STREAM_END)
  {
    // If ret is Z_OK, it might mean more data is expected, but we used Z_FINISH.
    // For a single chunk of compressed data, we expect Z_STREAM_END.
    inflateEnd(&strm);
    // printf("Decompressing data error");
    return false;
  }

  // Actual size of decompressed data
  size_t decompressed_size = decompressed_buffer.size() - strm.avail_out;
  inflateEnd(&strm);

  // 5. Truncate/Pad and store the result
  if (decompressed_size > SPX_MAX_RANGE_SAMPLES)
  {
    // Truncate
    video_cells.assign(decompressed_buffer.begin(), decompressed_buffer.begin() + SPX_MAX_RANGE_SAMPLES);
  }
  else if (decompressed_size < SPX_MAX_RANGE_SAMPLES)
  {
    // Pad with zeros, if the data is truly short, we just take what we have.
    video_cells.assign(decompressed_buffer.begin(), decompressed_buffer.begin() + decompressed_size);
    // Note: If padding with zeros is required, you'd resize the vector here:
    video_cells.resize(SPX_MAX_RANGE_SAMPLES, 0);
  }
  else
  {
    // Correct size
    video_cells = move(decompressed_buffer);
  }

  return true;
}

// bool SpxProcessor::getLatestVideoData(RadarVideoSweep &out)
// {
//   lock_guard<mutex> lock(latestMutex);
//   if (queue.empty())
//     return false;

//   out = move(queue.front());
//   queue.pop_front();
//   return true;
// }

bool SpxProcessor::combineUncompress(uint16_t sequence, RadarVideoSweep& result)
{
  auto primaryPos = primaryList.find(sequence);
  auto continuationPos = continuationList.find(sequence);

  if (primaryPos == primaryList.end() || continuationPos == continuationList.end())
    return false;

  RadarVideoSweep combined = primaryPos->second;
  combined.intensities.insert(
      combined.intensities.end(),
      continuationPos->second.intensities.begin(),
      continuationPos->second.intensities.end());

  // queue.push_back(move(combined));
  primaryList.erase(primaryPos);
  continuationList.erase(continuationPos);
  removeFromDeque(primaryOrder, sequence);
  removeFromDeque(continuationOrder, sequence);

  result = move(combined);

  return true;
}

void SpxProcessor::removeFromDeque(deque<uint16_t> &deq, uint16_t seq)
{
  auto it = find(deq.begin(), deq.end(), seq);
  if (it != deq.end())
    deq.erase(it);
}