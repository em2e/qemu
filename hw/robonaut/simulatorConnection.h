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
  uint8_t state;
  bool morelines;
  bool oneline;
  double pFront; //in mm -100,75 -> 100,75
  double pRead;
  double delta; //orientation of the line in [rad] atan((*p_rear-*p_front)/D

  double distanceSensor;

  uint64_t encoder; //calculated from speed

} SimulatoConnectionInputMessage;

typedef struct
{
	int64_t virtualTime;
  double motorPower; //motor power ration -1 - 1 (expected values -0.3 - 0.3)
  double fwdSteeringWheelAngle; //degree -32,5 - +32,5
  double revSteeringWheelAngle; //degree -32,5 - +32,5
  double distanceRotationAngle; //degree -32,5 - +32,5
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
void simulatorConnection_signalOutThread (SimulatorConnectionState *sc);

#endif //SIMULATOR_CONNECTION_H
