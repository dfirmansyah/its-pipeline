#ifndef PACKET_PROCESSOR_H
#define PACKET_PROCESSOR_H

#include <iostream>
#include <vector>
#include <thread>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <memory>

// Define a structure for a single packet
struct Packet
{
  std::vector<uint8_t> data;
};

/**
 * @brief Abstract base class defining the interface for packet processing.
 */
class AbstractPacketProcessor
{
public:
  virtual ~AbstractPacketProcessor() = default;

  // Public interface methods
  // void start();
  // void stop();
  // void enqueuePacket(Packet &&p);

  // Pure virtual method: Derived classes MUST implement this.
  virtual std::unique_ptr<RadarVideoSweep> processPacket(std::vector<uint8_t>& p) = 0;

  // Class is non-copyable
  AbstractPacketProcessor(const AbstractPacketProcessor &) = delete;
  AbstractPacketProcessor &operator=(const AbstractPacketProcessor &) = delete;

protected:
  AbstractPacketProcessor() {} // Protected constructor defined in the implementation file

  uint16_t ntohs_custom(uint16_t value)
  {
    // This is a common way to handle network-to-host conversion in C/C++
    // for systems that might be little-endian.
    return (value << 8) | (value >> 8);
  }

private:
  // std::queue<Packet> packet_queue_;
  // std::mutex queue_mutex_;
  // std::condition_variable queue_cv_;
  // std::thread processor_thread_;
  // std::atomic<bool> running_;

  // Private method containing the thread's main loop
  // void processLoop();
};

#endif // PACKET_PROCESSOR_H