#ifndef SOCKET_CONNECTION_H
#define SOCKET_CONNECTION_H

#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

typedef struct
{
  int socketFd;
  int peerFd;
  bool isOpen;
  bool isServer;
  bool isConnected;
  int lastError;
} SockConnectionState;

bool sockConnectionOpen (SockConnectionState *status, const char *socketPath);
bool sockConnectionAccept (SockConnectionState *status);
void sockConnectionClose (SockConnectionState *status);
bool sockConnectionRecv (SockConnectionState *status, void * buffer, u_int32_t size);
bool sockConnectionSend (SockConnectionState *status, void * buffer, u_int32_t size);

#endif /* SOCKET_CONNECTION_H */
