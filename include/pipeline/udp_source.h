#ifndef UDP_SOURCE_H
#define UDP_SOURCE_H

#include <iostream>
#include <thread>
#include "element_ports.h"
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

using namespace std;

class UdpSource : ItElement
{
private:
  int port;
  ItOutputPort<RawVideoData> *outputPort;

  unique_ptr<thread> worker_thread;

protected:
  bool canStart()
  {
    return state == IT_STATE_STOPED;
  }

  bool canStop()
  {
    return state == IT_STATE_STARTED || state == IT_STATE_PAUSED;
  }

  void processLoop()
  {
    SOCKET_TYPE sockfd;
    struct sockaddr_in addr{};
    const int MAX_PACKET_SIZE = 2048;
    vector<char> buffer(MAX_PACKET_SIZE);

    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd == INVALID_SOCKET)
    {
      cerr << "[UDP] Failed to create socket\n";
      return;
    }

    // Force blocking mode
    u_long mode = 0;
    ioctlsocket(sockfd, FIONBIO, &mode);

    // Allow rebinding even if the port is still in TIME_WAIT
    int reuse = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
      cerr << "[UDP] Bind failed on port " << port << "\n";
      CLOSE_SOCKET(sockfd);
      return;
    }

    cout << "[UDP] Listening on port " << port << "...\n";

    sockaddr_in senderAddr{};
    socklen_t addrLen = sizeof(senderAddr);

    while (state == IT_STATE_STARTED)
    {
      int len = recvfrom(sockfd, buffer.data(), MAX_PACKET_SIZE, 0,
                         (struct sockaddr *)&senderAddr, &addrLen);

      if (len == SOCKET_ERROR)
      {
#ifdef _WIN32
        int err = WSAGetLastError();
        if (err == WSAEWOULDBLOCK || err == WSAETIMEDOUT)
          continue;
#else
        if (errno == EAGAIN || errno == EWOULDBLOCK)
          continue;
#endif
        if (!state != IT_STATE_STARTED)
          break;
        cerr << "[" << getName() << "] recvfrom() error: " << err << "\n";
        continue;
      }

      if (len > 0)
      {
        vector<uint8_t> udp_data;
        udp_data.assign(buffer.begin(), buffer.begin() + len);
        RawVideoData data(udp_data);
        outputPort->push(move(data));
      }
    }

    CLOSE_SOCKET(sockfd);
  }

public:
  UdpSource(std::string elName, int portArg) : ItElement(elName), port(portArg)
  {
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

  bool start()
  {
    if (!canStart())
      return false;

    state = IT_STATE_STARTING;

    worker_thread.reset(new thread(&processLoop, this));
    cout << "[" << getName() << "] Starting thread ID: " << worker_thread->get_id() << endl;

    return ItElement::start();
  }

  bool stop()
  {
    if (!canStop())
      return false;

    state = IT_STATE_STOPING;
    if (worker_thread && worker_thread->joinable())
    {
      worker_thread->join();
      cout << "[" << getName() << "] Thread stopped." << endl;
    }
    return ItElement::stop();
  }
};

#endif // UDP_SOURCE_H