#include "edgesense/protocol.hpp"
#include "edgesense/sensor.hpp"
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
using Socket = SOCKET;
constexpr Socket kInvalidSocket = INVALID_SOCKET;
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
using Socket = int;
constexpr Socket kInvalidSocket = -1;
#endif

int main(int argc, char** argv) {
  std::string host = "127.0.0.1"; int port = 9000; int count = 30;
  for (int i=1; i<argc; ++i) { std::string arg=argv[i]; if(arg=="--host" && i+1<argc) host=argv[++i]; else if(arg=="--port" && i+1<argc) port=std::atoi(argv[++i]); else if(arg=="--count" && i+1<argc) count=std::atoi(argv[++i]); }
#ifdef _WIN32
  WSADATA data; if (WSAStartup(MAKEWORD(2,2), &data) != 0) return 1;
#endif
  Socket socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (socket_fd == kInvalidSocket) { std::cerr << "socket creation failed\n"; return 1; }
  sockaddr_in address{}; address.sin_family=AF_INET; address.sin_port=htons(static_cast<uint16_t>(port));
  if (inet_pton(AF_INET, host.c_str(), &address.sin_addr) != 1 || connect(socket_fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) { std::cerr << "Cannot connect to gateway at " << host << ':' << port << "\n"; return 1; }
  edgesense::VirtualSensorBoard board;
  for (int sequence=1; sequence<=count; ++sequence) { auto frame=edgesense::encode(board.poll(sequence)); size_t sent=0; while(sent<frame.size()) { int n=send(socket_fd,reinterpret_cast<const char*>(frame.data()+sent),static_cast<int>(frame.size()-sent),0); if(n<=0){std::cerr<<"send failed\n";return 1;}sent+=n;} std::cout << "published sequence " << sequence << '\n'; std::this_thread::sleep_for(std::chrono::milliseconds(250)); }
#ifdef _WIN32
  closesocket(socket_fd); WSACleanup();
#else
  close(socket_fd);
#endif
  return 0;
}
