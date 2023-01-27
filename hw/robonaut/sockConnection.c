#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <errno.h>

#include <hw/robonaut/sockConnection.h>

//static bool sendHello (SockConnectionState *status);
//static bool receiveHello (SockConnectionState *status);

static bool sockConnection_openAsServer (SockConnectionState *status, const struct sockaddr *address)
{

  printf ("try to open address as server\n");
  status->lastError = 0;

  if (bind (status->socketFd, address, sizeof(struct sockaddr_un)) == 0)
  {

    printf ("bind ok\n");

    if (listen (status->socketFd, 1) == 0)
    {

      printf ("listening...\n");
      status->isServer = true;

      return true;
    }
  }
  status->lastError = errno;
  perror ("openAsServer failed");
  return false;
}

static bool sockConnection_accept(SockConnectionState *status) {
  struct sockaddr_un clientAddr;
  socklen_t clientAddrSize = sizeof(clientAddr);
  if ((status->peerFd = accept (status->socketFd, (struct sockaddr*) &clientAddr, &clientAddrSize)) > 0)
  {
	status->isConnected = true;
	if (clientAddr.sun_path[0] == 0)
	{
	  clientAddr.sun_path[0] = '@';
	}
	printf ("client connected from addr: %s\n", clientAddr.sun_path);
	return true;
  }
  return false;
}

static bool sockConnection_openAsClient (SockConnectionState *status, const struct sockaddr *address)
{
  printf ("try to open address as client\n");
  status->lastError = 0;

  if (connect (status->socketFd, address, sizeof(struct sockaddr_un)) == 0)
  {
    status->peerFd = status->socketFd;
    printf ("connected\n");
    status->isConnected = true;
    return true;
  }
  status->lastError = errno;
  perror ("openAsClient failed");
  return false;
}

bool sockConnectionOpen (SockConnectionState *status, const char *socketPath)
{

  if ((status->socketFd = socket (AF_UNIX, SOCK_SEQPACKET, 0)) < 0)
  {
    perror ("socket open");
    return false;
  }

  struct sockaddr_un address;

  memset (&address, 0, sizeof(address));
  address.sun_family = AF_UNIX;
  strcpy (address.sun_path, socketPath);
  if (address.sun_path[0] == '@')
  {
    address.sun_path[0] = 0; //abstract path
  }

  //try to act as server
  if (sockConnection_openAsServer (status, (const struct sockaddr*) &address))
  {

  }
  else if (status->lastError == EADDRINUSE && sockConnection_openAsClient (status, (const struct sockaddr*) &address))
  {

  }

  if (status->lastError < 0)
  {

    close (status->socketFd);
    perror ("bind");
    return false;
  }

  status->isOpen = true;

  return true;
}

void sockConnectionClose (SockConnectionState *status)
{
  if (status->isOpen)
  {
    close (status->socketFd);
    status->socketFd = -1;
    status->isOpen = false;
  }
}

bool sockConnectionRecv (SockConnectionState *status, void * buffer, u_int32_t size)
{
  if (!status->isConnected && !sockConnection_accept(status))
  {
    status->lastError = errno;
    printf ("recv failed: %s\n", strerror (status->lastError));
    return false;
  }

  if (recv (status->peerFd, buffer, size, 0) < 0)
  {
    status->lastError = errno;
    printf ("recv failed: %s\n", strerror (status->lastError));
    return false;
  }

  status->lastError = 0;
  return true;

}

bool sockConnectionSend (SockConnectionState *status, void * buffer, u_int32_t size)
{
  if (send (status->peerFd, buffer, size, MSG_EOR) < 0)
  {
    status->lastError = errno;
    printf ("send failed: %s\n", strerror (status->lastError));
    return false;
  }
  status->lastError = 0;
  return true;

}

/*
bool sockConnectionHello (SockConnectionState *status)
{
  sendHello (status);
  receiveHello (status);
  return true;
}

static bool sendHello (SockConnectionState *status)
{

  if (send (status->peerFd, message, strlen (message) + 1, MSG_EOR) < 0)
  {
    status->lastError = errno;
    printf ("send failed: %s\n", strerror (status->lastError));
    return false;
  }
  status->lastError = 0;
  return true;
}

static bool receiveHello (SockConnectionState *status)
{
  char message[64];

  if (recv (status->peerFd, message, sizeof(message), 0) < 0)
  {
    status->lastError = errno;
    printf ("recv failed: %s\n", strerror (status->lastError));
    return false;
  }
  printf ("recv message: %s\n", message);
  status->lastError = 0;
  return true;
}
*/
