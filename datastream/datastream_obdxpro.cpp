#include "datastream_obdxpro.h"
#include <QSettings>
#include <QElapsedTimer>

datastream_obdxpro::datastream_obdxpro() {

}

datastream_obdxpro::~datastream_obdxpro() {
  if(port != nullptr) delete port;
}

QString datastream_obdxpro::interface_name() {
  return QString("OBDX Pro DVI");
}

void datastream_obdxpro::configure() {
  port = new QSerialPort();
  ratelimit_timer = new QElapsedTimer;
  ratelimit_timer->start();
  port->setBaudRate(115200);
}

bool datastream_obdxpro::restart() {
  QByteArray b;
  b.append((unsigned char)0x00);
  return dvi_simple_request(DVI_RESET,(unsigned char)0x00,b);
}

bool datastream_obdxpro::dvi_init() {
  //emit log("Restarting interface...",LOGLEVEL_DEBUG);
  //restart();

  // enter dvi mode
  if(elm_tx("DXDP1") == false) {
    emit log("Could not send ELM command for some reason.",LOGLEVEL_ERROR);
    return false;
  }

  QByteArray reply = elm_rx(500);
  if(reply.isEmpty()) {
    emit log("No response to ELM command (Interface missing or already in DVI mode)",LOGLEVEL_DEBUG);
  }

  QByteArray b;

  // set protocol
  emit log("Setting interface to VPW protocol...",LOGLEVEL_DEBUG);
  if(dvi_set_protocol(DVI_PROTO_VPW) == false) return false;

  // filter everything
  emit log("Configuring traffic filtering",LOGLEVEL_DEBUG);
  if(dvi_filter_dest_range(0x00,0xFF,false) == false) return false;

  // allow things intended for us
  if(dvi_filter_dest(0xF0,true) == false) return false;

  // print cable info
  emit log("Getting cable info...",LOGLEVEL_DEBUG);
  emit log("OBDX Pro Info:\n" + xproinfo(),LOGLEVEL_DEBUG);

  emit log("Enabling communications...",LOGLEVEL_DEBUG);
  // enable comms
  if(dvi_enable_comms(true) == false) return false;

  return true;
}

bool datastream_obdxpro::dvi_enable_comms(bool enable) {
  QByteArray b;
  b.append(DVI_PROTO_NET_ENABLE);
  b.append((unsigned char)enable);
  QByteArray reply = dvi_req(DVI_CONFIG_PROTOCOL,b);
  if(reply.isEmpty() || reply.size() < 2 || reply.at(1) != (unsigned char)enable || reply.at(0) != 0x05) {
    emit log("Failed to enable comunications.",LOGLEVEL_ERROR);
    return false;
  }
  return true;
}

bool datastream_obdxpro::dvi_set_protocol(int protocol) {
  QByteArray b;
  b.append(DVI_PROTO_SET);
  b.append((unsigned char)protocol);
  QByteArray reply = dvi_req(DVI_CONFIG_PROTOCOL,b);
  if(reply.isEmpty() || reply.size() < 2 || reply.at(1) != protocol || reply.at(0) != 0x01) {
    emit log("Failed to set interface protocol.",LOGLEVEL_ERROR);
    return false;
  }
  return true;
}

bool datastream_obdxpro::set_mode(_interface_mode mode) {
  QByteArray b;
  b.append(DVI_VPW_SET_4X);
  if(mode == MODE_HIGHSPEED){
    b.append(DVI_ON);
  } else {
    b.append(DVI_OFF);
  }
  return dvi_simple_request(DVI_CONFIG_VPW,DVI_VPW_SET_4X,b);
}

datastream_reply datastream_obdxpro::request(const datastream_request &message) {
  int max_retries = 3;
  if(message.override_retries > -1) max_retries = message.override_retries;

  datastream_reply reply;

  for(int x=0;x<=max_retries;x++) {
    reply = _request(message);

    // successful reply
    if(reply.success == true) return reply;
  }

  return reply;
}

datastream_reply datastream_obdxpro::_request(const datastream_request &message) {
  // construct packet
  QByteArray packet = message_to_packet(message);

  // send to interface and bail on error.
  if(packet.size() > 176) {
    if(dvi_simple_request(DVI_SEND_LARGE,DVI_SEND_OK,packet) == false) return datastream_reply::fail("DVI Error");
  } else {
    if(dvi_simple_request(DVI_SEND,DVI_SEND_OK,packet) == false) return datastream_reply::fail("DVI Error");
  }

  // use custom timeout from the message if we have one
  int timeout = 1000;
  if(message.override_timeout > 0) timeout = message.override_timeout;

  QElapsedTimer t;
  t.start();
  while(t.elapsed() < timeout) {
    datastream_reply r = get_reply(message.device_id,timeout - t.elapsed()); // this needs to handle errors on its own.
    if(r.command_id - 0x40 == message.command_id) return r;
  }

  emit log("Timeout waiting for reply",LOGLEVEL_ERROR);

  return datastream_reply::fail("Timeout waiting for reply");
}

bool datastream_obdxpro::dvi_filter_source(unsigned char device_id, bool allow) {
  QByteArray b;
  b.append(DVI_VPW_FILTER_SRC);
  b.append(device_id);
  if(allow == true) {
    b.append((unsigned char)0x00);
  } else {
    b.append(0x01);
  }
  return dvi_simple_request(DVI_CONFIG_VPW,b.at(0),b);
}

bool datastream_obdxpro::dvi_filter_source_range(unsigned char start_device, unsigned char end_device, bool allow) {
  QByteArray b;
  b.append(DVI_VPW_FILTER_SRC_RANGE);
  b.append(start_device);
  b.append(end_device);
  if(allow == true) {
    b.append(DVI_OFF);
  } else {
    b.append(DVI_ON);
  }
  return dvi_simple_request(DVI_CONFIG_VPW,b.at(0),b);
}

bool datastream_obdxpro::dvi_filter_dest_range(unsigned char start_device, unsigned char end_device, bool allow) {
  QByteArray b;
  b.append(DVI_VPW_FILTER_DEST_RANGE);
  b.append(start_device);
  b.append(end_device);
  if(allow == true) {
    b.append(DVI_OFF);
  } else {
    b.append(DVI_ON);
  }
  return dvi_simple_request(DVI_CONFIG_VPW,b.at(0),b);
}

bool datastream_obdxpro::dvi_filter_dest(unsigned char device_id, bool allow) {
  QByteArray b;
  b.append(DVI_VPW_FILTER_DEST);
  b.append(device_id);
  if(allow == true) {
    b.append(DVI_OFF);
  } else {
    b.append(DVI_ON);
  }
  return dvi_simple_request(DVI_CONFIG_VPW,b.at(0),b);
}

bool datastream_obdxpro::dvi_simple_request(unsigned char command, unsigned char expect_reply, const QByteArray &data) {
  QByteArray in = dvi_req(command,data);
  if(in.isEmpty()) return false;
  if((unsigned char)in.at(0) == expect_reply) return true;
  return false;
}

QByteArray datastream_obdxpro::dvi_req(unsigned char command, const QByteArray &data) {
  if(dvi_tx(command,data) == false) return QByteArray();

  QByteArray in = dvi_rx();

  if(in.isEmpty()) return QByteArray();

  // dvi error handling
  if(dvi_is_error((unsigned char)in.at(0)) == true) {
    while(in.size() < 4) in.append((unsigned char)0xFF);
    log_dvi_error(in.at(0),in.at(2),in.at(3));
    return QByteArray();
  }

  if((unsigned char)in.at(0) - 0x10 != command) {
    emit log("DVI reply mismatch",LOGLEVEL_ERROR);
    return QByteArray();
  }

  in.remove(0,2); // drop the command and size from the reply.
  return in;
}

bool datastream_obdxpro::dvi_is_error(unsigned char error_type) {
  if(error_type == DVI_COMM_ERROR || error_type == DVI_CONFIG_ERROR) return true;
  return false;
}

void datastream_obdxpro::log_dvi_error(unsigned char error_type, unsigned char command_code, unsigned char error_code) {
  switch(error_type) {
  case DVI_COMM_ERROR:
    emit log("DVI COMM Error for Command=" + QString::number(command_code,16).toUpper() + " Error=" + QString::number(error_code,16).toUpper(),LOGLEVEL_ERROR);
    break;
  case DVI_CONFIG_ERROR:
    emit log("DVI CONFIG Error for Command=" + QString::number(command_code,16).toUpper() + " Error=" + QString::number(error_code,16).toUpper(),LOGLEVEL_ERROR);
    break;
  }
}

bool datastream_obdxpro::dvi_tx(unsigned char command, const QByteArray &data) {
  if(open_port() == false) return false;

  QByteArray packet = data;

  int size = packet.size();

  if(command == DVI_SEND_LARGE) { // use 16 bit size
    if(packet.size() > 4100) {
      emit log("Data too large for dvi protocol! ",LOGLEVEL_ERROR);
      return false;
    }
    packet.prepend(datastream_request::n_to_array16(size));
  } else { // use 8 bit size
    if(packet.size() > 176) {
      emit log("Data too large for dvi protocol! ",LOGLEVEL_ERROR);
      return false;
    }
    packet.prepend((unsigned char)size);
  }

  packet.prepend(command);

  // checksum
  unsigned char c = 0;
  for(int x = 0;x<packet.size();x++) c += (unsigned char)packet.at(x);
  c = ~c; // invert
  packet.append(c);

  // rate limit
  while(ratelimit_timer->hasExpired(DVI_RATE_LIMIT_MS) == false) sleep_ms(1);
  ratelimit_timer->restart();

  if(serial_write(packet) == false) {
    emit log("Error writing to serial interface",LOGLEVEL_ERROR);
    return false;
  }

  emit log("DVI TX: " + packet.toHex().toUpper(),LOGLEVEL_COMM);
  return true;
}

QByteArray datastream_obdxpro::dvi_rx() {
  if(open_port() == false) return QByteArray();

  // we autocalculate the timer when the reply length is known.  the initial timeout is fixed.
  QElapsedTimer time;
  time.start();

  QByteArray buf_in;
  int pkt_size = -1; // -1 designates no header received yet.

  forever {
    buf_in.append(port->readAll());

    if(pkt_size == -1) {
      if(buf_in.size() > 3) {
        if(buf_in.at(0) == 0x09) { // large packet uses a 16 bit checksum
          pkt_size = datastream_message::get16(1,buf_in);
        } else { // otherwise 8 bit
          pkt_size = (unsigned char)buf_in.at(1) + 3;
        }
      }
    }

    if(pkt_size != -1 && buf_in.size() >= pkt_size) break;

    // wait for more data until timeout
    if(port->waitForReadyRead(DVI_REPLY_TIMEOUT - time.elapsed()) == false) {
      emit log("Timeout waiting for DVI reply",LOGLEVEL_ERROR);
      return QByteArray();
    }
  }

  // print output to debug log
  emit log("DVI RX: " + buf_in.toHex().toUpper(),LOGLEVEL_COMM);

  // verify checksum
  unsigned char c = 0;
  for(int x=0;x<buf_in.size() - 1;x++) c += (unsigned char)buf_in.at(x);
  c = ~c;
  if((unsigned char)buf_in.at(buf_in.size() - 1) !=  c) {
    emit log("Invalid checksum in dvi reply",LOGLEVEL_ERROR);
    return QByteArray();
  }

  // drop the checksum from the reply.
  buf_in.chop(1);

  ratelimit_timer->restart();

  // return
  return buf_in;
}

bool datastream_obdxpro::elm_tx(QString s) {
  emit log("Send ELM command: " + s,LOGLEVEL_COMM);
  QByteArray out = s.toLocal8Bit();
  out.append('\r');
  out.append('\n');
  return serial_write(out);
}

QByteArray datastream_obdxpro::elm_rx(int timeout) {
  QElapsedTimer time;
  time.start();

  QByteArray buf_in;

  // get data
  while(buf_in.endsWith('>') == false) {
    buf_in.append(port->readAll()); // get contents of buffer.
    if(port->waitForReadyRead(timeout - time.elapsed()) == false) return QByteArray();
  }

  // print output to debug log
  emit log("Received ELM reply: " + buf_in,LOGLEVEL_COMM);

  // return
  return buf_in;
}

datastream_reply datastream_obdxpro::get_reply(int device_id, int timeout_ms) {
  QElapsedTimer time;
  time.start();

  while(time.elapsed() < timeout_ms) {
    QByteArray in = dvi_rx();
    if(in.isEmpty()) continue;
    unsigned char dvi_command_id = in.at(0);
    if(dvi_command_id == DVI_RECEIVE || dvi_command_id == DVI_RECEIVE_LARGE) {
      int pkt_offset = 2;
      if(dvi_command_id == DVI_RECEIVE_LARGE) pkt_offset++;
      datastream_reply r;
      unsigned char dest_device = in.at(pkt_offset + 1);
      unsigned char src_device = in.at(pkt_offset + 2);
      if(src_device != device_id || dest_device != DEVICE_TESTER) continue;
      r.priority_id = in.at(pkt_offset);
      r.command_id = in.at(pkt_offset + 3) - 0x40;
      r.set_data(in.mid(pkt_offset + 4));
      return r;
    }
  }
  return datastream_reply::fail("Reply timeout");
}

bool datastream_obdxpro::serial_write(QByteArray b) {
  port->clear();
  if(port->write(b) == false) {
    emit log("Error writing data to port." + b.toHex().toUpper(),LOGLEVEL_ERROR);
    return false;
  }
  port->waitForBytesWritten();
  return true;
}

bool datastream_obdxpro::open_port() {
  if(state > COMM_STATE_CLOSED) return true;

  if(port->isOpen() == true) {
    set_state(COMM_STATE_OPEN);
    return true;
  }

  // FIXME probe for device here ...
  QSettings s;
  interface_id = s.value("SERIAL_PORT",QString()).toString();

  // set the port.
  port->setPortName(interface_id);

  //-----------------
  if(port->open(QIODevice::ReadWrite)) {
    emit log("SERIAL: Opened port " + port->portName(),LOGLEVEL_DETAIL);
    emit log("SERIAL: Port info: " + interface_info(port->portName()),LOGLEVEL_DETAIL);
    if(dvi_init() == true) {
      set_state(COMM_STATE_OPEN);
      return true;
    } else {
      emit log("Could not enter dvi mode.  Perhaps this is not a proper OBDXPRO cable.",LOGLEVEL_ERROR);
      set_state(COMM_STATE_CLOSED);
      port->close();
      return false;
    }
  } else {
    emit log("Could not open port: " + interface_id + "(" + port->errorString() + ")",LOGLEVEL_ERROR);
    set_state(COMM_STATE_CLOSED);
    return false;
  }
}

bool datastream_obdxpro::close_port() {
  if(port->isOpen() == false) return true; // alraedy closed
  sleep_ms(15);
  port->close();
  set_state(COMM_STATE_CLOSED);
  return true; // can't really fail.
}

void datastream_obdxpro::test_function(int n) {
  Q_UNUSED(n);
}

void datastream_obdxpro::keepalive() {

}

int datastream_obdxpro::get_firmware_version() {
  QByteArray b;
  b.append(0x01);
  QByteArray in = dvi_req(DVI_INFO,b);
  if(in.isEmpty()) return 0;
  return datastream_message::get16(0,in);
}

int datastream_obdxpro::get_model_number() {
  QByteArray b;
  b.append(0x02);
  QByteArray in = dvi_req(DVI_INFO,b);
  if(in.isEmpty()) return 0;
  return datastream_message::get16(0,in);
}

int datastream_obdxpro::get_free_ram() {
  QByteArray b;
  b.append(0x07);
  QByteArray in = dvi_req(DVI_INFO,b);
  if(in.isEmpty()) return 0;
  return datastream_message::get32(0,in);
}

QString datastream_obdxpro::get_cable_name() {
  QByteArray b;
  b.append(0x03);
  QByteArray in = dvi_req(DVI_INFO,b);
  if(in.isEmpty()) return "ERR";
  return in.mid(1);
}

QString datastream_obdxpro::get_unique_id() {
  QByteArray b;
  b.append(0x04);
  QByteArray in = dvi_req(DVI_INFO,b);
  if(in.isEmpty()) return "ERR";
  return in.toHex().toUpper();
}

QString datastream_obdxpro::xproinfo() {
  QString out;
  out.append("Firmware v" + QString::number((float)get_firmware_version() / 10) + "\n");
  out.append("Model #" + QString::number(get_model_number()) + "\n");
  out.append("Cable Name: " + get_cable_name() + "\n");
  out.append("UID: " + get_unique_id());
  return out;
}

