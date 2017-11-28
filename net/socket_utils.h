#ifndef LIGHTING_NET_SOCKET_UTILS_H
#define LIGHTING_NET_SOCKET_UTILS_H

#include <arpa/inet.h>
#include <string>

namespace net {
namespace socketutils {

#define SocketFd int

struct sockaddr* sockaddr_cast(struct sockaddr_in6* addr);
struct sockaddr* sockaddr_cast(struct sockaddr_in* addr);
const struct sockaddr* sockaddr_cast(const struct sockaddr_in* addr);
const struct sockaddr* sockaddr_cast(const struct sockaddr_in6* addr);
const struct sockaddr_in* sockaddr_in_cast(const struct sockaddr* addr);
const struct sockaddr_in6* sockaddr_in6_cast(const struct sockaddr* addr);

SocketFd CreateNonBlockingSocket(sa_family_t family);

void ListenSocket(SocketFd fd);

void BindSocketFd(SocketFd fd, const struct sockaddr* addr);

SocketFd SocketConnect(SocketFd fd, const struct sockaddr* addr);

SocketFd AcceptSocket(SocketFd fd, struct sockaddr_in* addr);

ssize_t Read(SocketFd fd, void* buf, size_t count);

ssize_t Write(SocketFd fd, const void* buf, size_t count);

ssize_t ReadV(SocketFd fd, const struct iovec* iov, int iovcnt);

void CloseSocket(SocketFd fd);

void ShutdownWrite(SocketFd fd);

std::string SocketAddr2Ip(const struct sockaddr* addr);
std::string SocketAddr2IpPort(const struct sockaddr* addr);

void FromIpPort(const char* ip, uint16_t port, struct sockaddr_in* addr);
void FromIpPort(const char* ip, uint16_t port, struct sockaddr_in6* addr);

int GetSocketError(SocketFd fd);

struct sockaddr_in6 GetLocalAddr(int sockfd);
struct sockaddr_in6 GetPeerAddr(int sockfd);
bool IsSelfConnect(int sockfd);

bool ReUseSocketPort(SocketFd, bool reuse);
bool ReUseSocketAddress(SocketFd socket_fd, bool reuse);

}}
#endif

