#ifndef UDP_SOURCE_H
#define UDP_SOURCE_H

#include <iostream>
#include <thread>
#include "common.h"
#include "it_port.h"
#include "it_element.h"
#include "RadarTypes.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#define CLOSE_SOCKET(s) closesocket(s)
#define SOCKET_TYPE SOCKET
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#define SOCKET_TYPE int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define CLOSE_SOCKET(s) close(s)
#endif

enum class UdpMode
{
  Blocking,
  NonBlocking
};

class UdpSource : public ItElement
{
private:
  const int MAX_PACKET_SIZE = 2048;

  int port;
  UdpMode mode;
  unique_ptr<ItOutputPort<RawVideoData>> outputPort;
  unique_ptr<thread> worker_thread = nullptr;

  void sendOut(const std::vector<uint8_t> udp_data)
  {
    unique_ptr<RawVideoData> dataPtr(new RawVideoData(std::move(udp_data)));
    outputPort->push(std::move(dataPtr));
  }

protected:
  std::atomic<bool> _running{false};

  bool canStart()
  {
    return getState() == IT_STATE_STOPED;
  }

  bool canStop()
  {
    ItObjState _state = getState();
    return _state == IT_STATE_STARTED || _state == IT_STATE_PAUSED;
  }

  void processLoopBlocking(SOCKET_TYPE sockfd,
                           std::vector<char> &buffer,
                           std::vector<uint8_t> &udp_data,
                           sockaddr_in &senderAddr,
                           socklen_t &addrLen,
                           int maxPacketSize)
  {
    while (getState() == IT_STATE_STARTED)
    {
      fd_set readfds;
      FD_ZERO(&readfds);
      FD_SET(sockfd, &readfds);

      timeval tv;
      tv.tv_sec = 0;
      tv.tv_usec = 500000; // 500 ms

      int sel = select(sockfd + 1, &readfds, nullptr, nullptr, &tv);
      if (sel <= 0)
      {
        if (getState() != IT_STATE_STARTED)
          break;
        continue; // timeout or benign error, just retry
      }

      if (!FD_ISSET(sockfd, &readfds))
        continue;

      int len = recvfrom(sockfd, buffer.data(), maxPacketSize, 0,
                         reinterpret_cast<sockaddr *>(&senderAddr), &addrLen);
      if (len <= 0)
      {
        if (getState() != IT_STATE_STARTED)
          break;
        continue;
      }

      udp_data.clear();
      udp_data.assign(buffer.begin(), buffer.begin() + len);
      sendOut(udp_data);
    }
  }

  void processLoopNonBlocking(SOCKET_TYPE sockfd,
                              std::vector<char> &buffer,
                              std::vector<uint8_t> &udp_data,
                              sockaddr_in &senderAddr,
                              socklen_t &addrLen,
                              int maxPacketSize)
  {
    using namespace std::chrono;

    while (getState() == IT_STATE_STARTED)
    {
      int len = recvfrom(sockfd, buffer.data(), maxPacketSize, 0,
                         reinterpret_cast<sockaddr *>(&senderAddr), &addrLen);

      if (len == SOCKET_ERROR)
      {
#ifdef _WIN32
        int err = WSAGetLastError();
        if (err == WSAEWOULDBLOCK || err == WSAETIMEDOUT)
        {
          std::this_thread::sleep_for(milliseconds(1)); // small backoff
          continue;
        }
#else
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
          std::this_thread::sleep_for(milliseconds(1));
          continue;
        }
#endif
        if (getState() != IT_STATE_STARTED)
          break;
        continue;
      }

      if (len > 0)
      {
        udp_data.clear();
        udp_data.assign(buffer.begin(), buffer.begin() + len);
        sendOut(udp_data);
      }
    }
  }

  void processLoop()
  {
    SOCKET_TYPE sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd == INVALID_SOCKET)
    {
      std::cerr << "[" << getName() << "] Failed to create socket\n";
      return;
    }

    // Allow rebinding even if the port is still in TIME_WAIT
    int reuse = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse));

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
      std::cerr << "[" << getName() << "] Bind failed on port " << port << "\n";
      CLOSE_SOCKET(sockfd);
      return;
    }

    cout << "[" << getName() << "] Listening on port " << port << "...\n";

#ifndef _WIN32
    if (mode == UdpMode::NonBlocking)
    {
      int flags = fcntl(sockfd, F_GETFL, 0);
      if (flags != -1)
      {
        fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
      }
    }
#else
    u_long nb = (mode == UdpMode::NonBlocking) ? 1 : 0;
    ioctlsocket(sockfd, FIONBIO, &nb);
#endif

    sockaddr_in senderAddr{};
    socklen_t addrLen = sizeof(senderAddr);

    std::vector<char> buffer(MAX_PACKET_SIZE);
    std::vector<uint8_t> udp_data;

    if (mode == UdpMode::Blocking)
    {
      processLoopBlocking(sockfd, buffer, udp_data, senderAddr, addrLen, MAX_PACKET_SIZE);
    }
    else
    {
      processLoopNonBlocking(sockfd, buffer, udp_data, senderAddr, addrLen, MAX_PACKET_SIZE);
    }

    CLOSE_SOCKET(sockfd);
  }

public:
  UdpSource(std::string elName, int portArg, UdpMode m = UdpMode::Blocking)
      : ItElement(elName), port(portArg), mode(m)
  {
    outputPort = make_unique<ItOutputPort<RawVideoData>>("udp_source_out");
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
      cerr << "[UDP] WSAStartup failed.\n";
      throw runtime_error("WSAStartup failed");
    }
#endif
  }

  ~UdpSource()
  {
#ifdef _WIN32
    WSACleanup();
#endif
    stop();
  }

  bool start() override
  {
    if (!canStart())
      return false;

    setState(IT_STATE_STARTING);
    worker_thread.reset(new thread(&processLoop, this));
    _running = true;
    cout << "[" << getName() << "] Starting thread ID: " << worker_thread->get_id() << endl;

    return ItElement::start();
  }

  bool stop() override
  {
    if (!canStop())
      return false;

    setState(IT_STATE_STOPING);
    if (worker_thread && worker_thread->joinable())
    {
      worker_thread->join();
      cout << "[" << getName() << "] Thread stopped." << endl;
    }
    _running = false;
    return ItElement::stop();
  }

  ItOutputPort<RawVideoData> *getOutputPort() const { return outputPort.get(); }
};

#endif // UDP_SOURCE_H