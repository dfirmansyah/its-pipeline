#include "pipeline/udp_listener.h"
#include <vector>

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

UdpListener::UdpListener(int port) : port(port)
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

UdpListener::~UdpListener()
{
    stop();
#ifdef _WIN32
    WSACleanup();
#endif
}

void UdpListener::process()
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

    while (running)
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
            if (!running)
                break;
            cerr << "[UDP] recvfrom() error: " << err << "\n";
            continue;
        }

        if (len > 0)
        {
            vector<uint8_t> data;
            data.assign(buffer.begin(), buffer.begin() + len);
            // processor.enqueuePacket(move(p));
            downstream_port->receive(move(data));
        }
    }

    CLOSE_SOCKET(sockfd);
    cout << "[UDP] Listener stopped.\n";
}
