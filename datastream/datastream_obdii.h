#ifndef DATASTREAM_OBDII_H
#define DATASTREAM_OBDII_H

// This parent class is never to be used directly, as it has no way to actually communicate.
// It needs to be subclassed for your physical interface.

#include <QObject>
#include "datastream_interface.h"

class datastream_obdii : public datastream_interface {
public:
  enum _generic_obdii_devices {
    DEVICE_BROADCAST = 0xFE,
    DEVICE_TESTER = 0xF0
  };

  enum _obdii_command {
    COMM_STOP_COMMS = 0x28,
    COMM_UNLOCK_PCM = 0x27
  };

  datastream_obdii();
  QByteArray message_to_packet(const datastream_request &r);
protected:
  void become_bus_master();
};

#endif // DATASTREAM_OBDII_H
