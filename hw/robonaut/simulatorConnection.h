#ifndef SIMULATOR_CONNECTION_H
#define SIMULATOR_CONNECTION_H

#include <stdbool.h>
#include <sys/types.h>
#include "qemu/osdep.h"
#include "qapi/error.h"
#include "qom/object.h"
#include "qemu/thread.h"
#include <hw/robonaut/sockConnection.h>

typedef struct
{
  uint32_t x;
} SimulatoConnectionInputMessage;

typedef struct
{
  uint32_t x;
} SimulatoConnectionOutputMessage;

typedef struct
{
  QemuThread inpThread;
  QemuThread outThread;
  QemuMutex outThreadMutex;
  QemuCond outThreadCond;
  bool stopping;
  SockConnectionState sockConnection;
  SimulatoConnectionInputMessage inpMsg;
  SimulatoConnectionOutputMessage outMsg;
} SimulatorConnectionState;

bool simulatorConnection_init (SimulatorConnectionState *sc);

#endif //SIMULATOR_CONNECTION_H
