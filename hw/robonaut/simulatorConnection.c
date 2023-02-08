#include <hw/robonaut/simulatorConnection.h>

static const char *socketPath = "@/emze/robonaut/emulator";


static void *simulatorConnection_inpThread (void *opaque)
{
  SimulatorConnectionState *sc = opaque;
  while(!sc->stopping)
  {
    int size = sockConnectionRecv (&sc->sockConnection, &sc->inpMsg, sizeof(SimulatoConnectionInputMessage));
    if (size >= 0)
    {
    	if (sc->inpCallback != NULL)
    	{
    		sc->inpCallback(sc->inpCallbackParam, size, &sc->inpMsg);
    	}
    }
    else
    {
      //TODO error happened, close emulator?
    }
  }
  return NULL;
}

static void *simulatorConnection_outThread (void *opaque)
{
  SimulatorConnectionState *sc = opaque;
  while(!sc->stopping)
  {
    qemu_mutex_lock(&sc->outThreadMutex);
    qemu_cond_wait(&sc->outThreadCond, &sc->outThreadMutex);//no timeout
    if (!sc->stopping) {
      //send data
      if (!sockConnectionSend (&sc->sockConnection, &sc->outMsg, sizeof(SimulatoConnectionOutputMessage) ))
      {
        //TODO error happened, close emulator?
      }
    }
    qemu_mutex_unlock(&sc->outThreadMutex);
  }
  return NULL;
}

bool simulatorConnection_init (SimulatorConnectionState *sc)
{
  sc->stopping = false;
  sc->inpCallback = NULL;
  sc->inpCallbackParam = NULL;

  if (!sockConnectionOpen (&sc->sockConnection, socketPath)) {
    //socket open error, close emulator
    return false;
  }

  if (sc->sockConnection.isServer && !sc->sockConnection.isConnected && !sockConnectionAccept (&sc->sockConnection))
  {
  	return false;
  }

  if (sockConnectionRecv (&sc->sockConnection, &sc->inpMsg, sizeof(SimulatoConnectionInputMessage) < 0 ))
  {
  	return false;
  }

  printf ("simulator connection initialized\n");

  qemu_mutex_init (&sc->outThreadMutex);
  qemu_cond_init (&sc->outThreadCond);
  qemu_thread_create (&sc->inpThread, "simulatorConnectionInp", simulatorConnection_inpThread, sc, QEMU_THREAD_JOINABLE);
  qemu_thread_create (&sc->outThread, "simulatorConnectionOut", simulatorConnection_outThread, sc, QEMU_THREAD_JOINABLE);

  return true;
}

void simulatorConnection_signalOutThread (SimulatorConnectionState *sc)
{
	qemu_mutex_lock(&sc->outThreadMutex);
	qemu_cond_signal(&sc->outThreadCond);
	qemu_mutex_unlock(&sc->outThreadMutex);
}
