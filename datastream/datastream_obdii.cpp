#include "datastream_obdii.h"

datastream_obdii::datastream_obdii() {

}

QByteArray datastream_obdii::message_to_packet(const datastream_request &r) {
  QByteArray out;
  out.append(r.priority_id); // priority
  out.append(r.device_id); // destination device
  out.append(0xF0); // source device (us)
  out.append(r.command_id);
  return out;
}

void datastream_obdii::become_bus_master() {
  datastream_request req(DEVICE_BROADCAST,COMM_STOP_COMMS);
  req.set_first(0x00);
  datastream_reply rep = request(req);
     // <6C FE F0 28 00
     // >6C F0 FE 68 01 00 00
}
