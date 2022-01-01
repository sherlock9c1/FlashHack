#include "datastream_8192.h"
#include <QSerialPortInfo>
#include <QElapsedTimer>
#include "dialog_interface_select.h"
#include <QDebug>
#include <QSettings>

// throw misc temp development defines up here.  comment before release build.
//#define DEBUG_COMMS
//#define LOG_DISCARDED_TRAFFIC

void datastream_8192::test_function(int n) {
  switch(n) {
  case TESTFUNCTION_SCAN_IDLE:
    scan_idle();
    break;
  default:
    emit log("Unknown test function in datastream_8192.",LOGLEVEL_ERROR);
  }
}

void datastream_8192::scan_idle() {
  open_port();
  int n_messages = 40;
  int timeout = 10000;
  emit log("Scanning ALDL " + QString::number(n_messages) + " messages with timeout of " + QString::number(timeout) + "ms",LOGLEVEL_STATUS);
  for(int x=0;x<40;x++) {
    datastream_reply r = resync_datastream(10000);
    if(r.success == false) break;
    if(r.device_id == 0) {
      emit log("Bus is quiet.  Abort.",LOGLEVEL_STATUS);
      break;
    }
    emit log("IDLE: " + r.to_string(),LOGLEVEL_STATUS);
  }
  close_port();
}

void datastream_8192::keepalive() {
  if(bus_master_id < 1) return;
  if(state != COMM_STATE_OPEN) return;
  if(port->isOpen() == false) return; // in case above state is not reliable .
  datastream_request req(bus_master_id,0x08);
  datastream_reply repl = request(req);
  if(repl.success == false) {
    emit log("No reply to keepalive request.",LOGLEVEL_ERROR);
    disconnect_from_bus();
  }
}

datastream_8192::~datastream_8192() {
  if(port != nullptr) {
    port->close();
    delete port;
  }
  if(keepalive_timer != nullptr) {
    keepalive_timer->stop();
    delete keepalive_timer;
  }
}

QString datastream_8192::test_interface_conditions() const {
  return "Disconnect your ALDL interface or cable from the vehicle so nothing is connected to the data pin.";
}

bool datastream_8192::interface_testable() {
  return true;
}

datastream_reply datastream_8192::resync_datastream(int timeout) {
  // discard datastream buffer up until first valid message.
  // we autocalculate the timer when the reply length is known.  the initial timeout is fixed.
  QElapsedTimer time;
  time.start();

  // first discard all buffer contents
  port->clear();

  QByteArray buf_in;

  emit log("Attempting to find start of datastream...",LOGLEVEL_DEBUG);

  forever {
    if(port->waitForReadyRead(timeout - time.elapsed()) == false) {
      if(buf_in.size() == 0) { // we got nothing
        emit log("No ALDL data to resync - bus is quiet.",LOGLEVEL_DEBUG);
        datastream_reply r;
        r.device_id = 00;
        r.success = true;
        return r; // an empty but successful reply means a silent bus.
      } else { // we got garbage
        emit log("Could not resynchronize ALDL bus, no valid messages.  Serial data is likely corrupt.",LOGLEVEL_DEBUG);
        return reply_error("No ALDL sync point found.",ERRORTYPE_COMM);
      }
    }

    forever {
      // we only consume one byte at a time then rip through the whole buffer.
      // this is expensive but not in modern CPU terms of course
      // we want to leave the entire contents of the border intact.
      char in;
      if(port->read(&in,1) == 0) break;
      buf_in.append(in);

      // at least 3 bytes for a packet, right?  don't bother looking at a short buffer.
      if(buf_in.size() < 3) continue;

      // look for the first byte in a valid message.
      for(int x=0;x<buf_in.size() - 2;x++) {
        int pkt_size = buf_in.at(x + 1) - 0x52;
        if(pkt_size <= 3) continue;
        if(pkt_size > 0xAD) continue;
        if(x + pkt_size > buf_in.size()) continue; // could be a real packet but not enough data yet to find out.
        if(test_checksum(buf_in.mid(x,pkt_size)) == true) {
          buf_in.remove(0,x); // remove extraneous junk at beginning of buffer
          if(x > 0) emit log("Resynchronized ALDL datastream by fast forwarding to " + QString::number(x),LOGLEVEL_DEBUG);
          datastream_reply r;
          r.device_id = (unsigned char)buf_in.at(0);
          r.command_id = (unsigned char)buf_in.at(2);
          r.set_data(buf_in.mid(3,pkt_size - 4));
          return r;
        }
      }
    }
  }
}

bool datastream_8192::test_interface() {
  if(open_port() == false) return false;
  log("Examining interface " + interface_id + "...",LOGLEVEL_STATUS);
  log(interface_info(interface_id),LOGLEVEL_STATUS);

  // check for idle data
  port->clear();
  if(port->waitForReadyRead(2000) == true) {
    log("There is data on the bus.  Your interface may be connected to something, or it may be faulty.",LOGLEVEL_ERROR);
    return false;
  } else {
    log("The bus has been silent for two seconds.",LOGLEVEL_STATUS);
  }

  if(looptest(10,1) == false) return false;
  if(looptest(12,2) == false) return false;
  if(looptest(20,3) == false) return false;
  if(looptest(10,32) == false) return false;
  if(looptest(10,64) == false) return false;
  if(looptest(8,150) == false) return false;
  if(looptest(4,400) == false) return false;
  if(looptest(1,1000) == false) return false;

  log("All tests OK.  This interface should be safe to use.  No warranty.",LOGLEVEL_STATUS);
  return true;
}

bool datastream_8192::looptest(int iterations, int test_pkt_size) {
  QElapsedTimer t;
  int corruption_errors = 0;
  int write_errors = 0;
  int read_errors = 0; // read or loopback errors

  int write_latency = 0; // time for write to complete
  int write_to_read_latency = 0; // time between write completion and read
  int read_pkt_latency = 0; // time to read 100 bytes

  int timeout = test_pkt_size * 30; // 5 second timeout for 100 bytes should be plenty.

  log("Test data: " + QString::number(iterations) + " chunks @ " + QString::number(test_pkt_size) + "b",LOGLEVEL_STATUS);

  for(int x=0;x<iterations;x++) {

    // fill random array of 100 chars
    QByteArray out;
    for(int y=0;y<test_pkt_size;y++) out.append(rand() % 255);

    // write the data and time it
    t.restart();
    port->write(out);
    while(port->bytesToWrite() > 0) {
      if(port->waitForBytesWritten(timeout) == false) {
        write_errors++;
      }
    }

    write_latency += t.elapsed();

    // get the reply and time it
    QByteArray in;
    t.restart();
    bool any_data_recieved = false;
    while(in.size() < out.size()) {
      port->waitForReadyRead(1);
      in.append(port->readAll());
      if(any_data_recieved == false && in.size() > 0) {
        write_to_read_latency += t.elapsed();
        any_data_recieved = true;
      }
      if(t.elapsed() > timeout) {
        read_errors++;
        port->clear();
        break;
      }
    }

    if(in.size() == out.size()) { // if equal ensure integrity
      read_pkt_latency += t.elapsed();       // get total packet read time
      for(int y=0;y<in.size();y++) {
        if(in.at(y) != out.at(y)) corruption_errors++;
      }
    } else {
      corruption_errors++;
      port->clear();
    }
  }

  // examine test results
  bool major_fault = false;

  if(corruption_errors > 0) {
    major_fault = true;
    log(QString::number(corruption_errors) + " bytes of corruption or lost data.",LOGLEVEL_ERROR);
  }

  if(write_errors > 0) {
    major_fault = true;
    log(QString::number(write_errors) + " failure(s) to transmit data on the interface.",LOGLEVEL_ERROR);
  }

  if(read_errors > 0) {
    major_fault = true;
    log(QString::number(read_errors) + " read errors.",LOGLEVEL_ERROR);
  }

  log("Test latency results: TX=" + QString::number(write_latency / iterations) + "ms"
      + "  TX/RX=" + QString::number(write_to_read_latency / iterations) + "ms"
      + "  RX=" + QString::number(read_pkt_latency / iterations) + "ms",LOGLEVEL_STATUS);

  if(major_fault == true) {
    log("WARNING!  A critical failure occurred in the test.  This probably means the interface is not stable enough to use for reproramming.",LOGLEVEL_ERROR);
    return false;
  } else if((write_latency / iterations) > 250 + test_pkt_size || read_pkt_latency / iterations > 400 + test_pkt_size) {
    log("WARNING!  The interface has quite a bit more latency than normal.  The performance or connection stability of this interface may not be good enough.",LOGLEVEL_ERROR);
    return false;
  } else {
    return true;
  }

}

QString datastream_8192::interface_name() {
  return QStringLiteral("ALDL 8192 Serial");
}

QString datastream_8192::interface_info(QString interface_id) {
  return dialog_interface_select::serial_interface_info(interface_id);
}

void datastream_8192::configure() {
  // configure port
  port = new QSerialPort();

  port->setBaudRate(8192);

  port->setReadBufferSize(MAX_BUF_SIZ);

  // configure keepalive timer
  keepalive_timer = new QTimer();
  connect(keepalive_timer,SIGNAL(timeout()),this,SLOT(keepalive()));
  keepalive_timer->setInterval(1500);
}

bool datastream_8192::test_checksum(const QByteArray &b) {
  int x = 0;
  unsigned int sum = 0;
  for(x=0;x<b.size();x++) sum += (unsigned char)b.at(x);
  if((sum & 0xFF) == 0) {
    return true;
  } else {
    return false;
  }
}

unsigned char datastream_8192::calc_checksum(const QByteArray &b) {
  int x = 0;
  unsigned int sum = 0;
  for(x=0;x<b.size();x++) sum += (unsigned char)b.at(x);
  return ( 256 - ( sum % 256 ) );
}

QByteArray datastream_8192::message_to_packet(const datastream_request &message) {
  QByteArray out;
  out.append((unsigned char)message.device_id);
  out.append((unsigned char)(message.size() + 4 + 0x52));
  out.append((unsigned char)message.command_id);
  out.append(message.get_data());
  out.append(calc_checksum(out));
  return out;
}

bool datastream_8192::connect_to_bus() {
  if(get_state() == COMM_STATE_CLOSED) {
    bus_master_id = 0; // reset bus master_id
    if(open_port() == false) {
      return false;
    } else {
      return become_bus_master();
    }
  }
  // otherwise check if there's idle traffic... if there is we probably lost connection.
  if(serial_discard(5,10) == true) {
    return become_bus_master();
  } else {
    return true;
  }
}

bool datastream_8192::disconnect_from_bus() {
  keepalive_timer->stop();
  if(get_switch("ALDL_DISCONNECT_PROPERLY",false) == true && bus_master_id > 0) {
    if(serial_discard(10,200) == true) return true; // there's already idle traffic.
    datastream_request r(bus_master_id,0x09);
    emit log("Giving back ALDL bus master to " + QString::number(bus_master_id,16).toUpper(),LOGLEVEL_DETAIL);
    return serial_write(message_to_packet(r));
  }
  return true;
}

datastream_reply datastream_8192::request(const datastream_request &message) {
  // we need to boundary check our message or following functions may overrun length and we'll hang the bus.
  // our payload plus 4 bytes (three header, one tailing checksum) plus 0x52 must not overrun a byte.
  if(message.size() > 0xFF - 0x04 - 0x52) {
    return reply_error("Request too large for protocol.",ERRORTYPE_SIZE);
  }

  // restart keepalive timer .
  keepalive_timer->start();

  // predelay by 50ms if slow mode is active
  if(get_switch("ALDL_SLOW_MODE",false) == true) sleep_ms(50);

  // this will ensure the bus is silent before we attempt to use it, and inherently handles initial connection
  // if required.  the overhead is small-ish.
  if(connect_to_bus() == false) {
    return reply_error("Could not connect to interface and/or aldl bus.",ERRORTYPE_COMM);
  }

  int max_retries = 3;
  if(message.override_retries > -1) max_retries = message.override_retries;

  datastream_reply reply;

  for(int x=0;x<=max_retries;x++) {
    reply = _request(message);

    // successful reply
    if(reply.success == true) return reply;

    // we handle a serial echo error as a bus disconnection event as a likely cause is a device sending
    // out-of-order packets as it assumes it is the bus master.
    if(reply.error_type == ERRORTYPE_COMM) {
      emit log("Trying to reconnect to bus...",LOGLEVEL_DETAIL);
      sleep_ms(250);
      if(connect_to_bus() == false) {
        emit log("Could not stabilize ALDL bus.",LOGLEVEL_ERROR);
        return reply;
      }
      // else continue to retry.
    }
  }

  return reply;
}

datastream_reply datastream_8192::_request(const datastream_request &message) {
  // write the packet and make sure it actually did get written.
  QByteArray packet = message_to_packet(message);
  if(serial_write(packet) == false) {
    return reply_error("Could not write packet to the serial interface.",ERRORTYPE_COMM);
  }

  // handle echo
  if(serial_read_specific(packet,packet.size() * 10) == false) {
    datastream_reply fail_reply;
    fail_reply.error_type = ERRORTYPE_COMM;
    fail_reply.success = false;
    fail_reply.error_message = "Echo corrupt.";
    return fail_reply;
  }

  // use custom timeout from the message if we have one
  int timeout = 1000;
  if(message.override_timeout > 0) timeout = message.override_timeout;

  return get_reply(message.device_id,timeout); // this needs to handle errors on its own.
}

datastream_reply datastream_8192::get_reply(int device_id, int timeout_ms) {
  // we autocalculate the timer when the reply length is known.  the initial timeout is fixed.
  QElapsedTimer time;
  time.start();

  QByteArray buf_in;
  int pkt_size = -1; // -1 designates no header recieved yet.

  // get data
  forever {
    // get contents of buffer.
    buf_in.append(port->readAll());

    // if header not recieved yet....
    if(pkt_size == -1) {

      // clear buffer up to next instance of device id
      if(device_id > 0) {
        while(buf_in.isEmpty() == false && (unsigned char)buf_in.at(0) != device_id) {
          buf_in.remove(0,1);
        }
      }

      if(buf_in.size() > 2) {
        pkt_size = (unsigned char)buf_in.at(1) - 0x52;

        // if packet size is nonsensical, continue and hope there is another packet.
        if(pkt_size > 0xAD) {
          buf_in.remove(0,1);
          pkt_size = -1;
          continue; // invalid size.
        }
      }

    }

    if(pkt_size != -1 && buf_in.size() >= pkt_size) {

      // is it a valid packet?
      if(test_checksum(buf_in.mid(0,pkt_size)) == false) {
        // no..and we should keep trying until the timeout expires, i guess.
        buf_in.remove(0,1);
        pkt_size = -1;
        continue; // invalid size.

      } else { // valid.
        break;
      }
    }

    // wait for more data.
    if(port->waitForReadyRead(timeout_ms - time.elapsed()) == false) {
      return reply_error("Timeout waiting for reply payload.",ERRORTYPE_COMM);
    }
  }

  // print output to debug log
  emit log("Recieved reply: " + buf_in.toHex().toUpper(),LOGLEVEL_COMM);

  // construct reply...
  datastream_reply reply;
  reply.device_id = (unsigned char)buf_in.at(0);
  reply.command_id = (unsigned char)buf_in.at(2);
  reply.set_data(buf_in.mid(3,pkt_size - 4));

  // return
  return reply;
}

int datastream_8192::timeout_for_size(int size) {
  return size * 3;
}

bool datastream_8192::serial_read_specific(QByteArray packet, int timeout) {
  QByteArray reply = serial_read_size(packet.size(),timeout);
  if(reply.isEmpty()) return false; // this throws its own debug message
  if(reply != packet) {
    emit log("Reply was not as expected: " + packet.toHex() + " vs " + reply.toHex().toUpper(),LOGLEVEL_COMM);
    return false;
  }
  return true;
}

QByteArray datastream_8192::serial_read_size(int size, int timeout) {
  QElapsedTimer time;
  time.start();
  QByteArray buf_out;
  while(buf_out.size() < size) {
    if(time.hasExpired(timeout)) {
      emit log("Timeout waiting for read of size " + QString::number(size),LOGLEVEL_COMM);
      return QByteArray();
    }
    if(port->waitForReadyRead(1) == true) {
      int read_size = size - buf_out.size();
      buf_out.append(port->read(read_size));
    }
  }

  return buf_out;
}

bool datastream_8192::become_bus_master() {
  emit log("Reconnecting to ALDL bus, please wait....",LOGLEVEL_STATUS);

  int n_retries = 3;

  for(int x=0;x<n_retries;x++) {

    // if there is lets check for a heartbeat, if there is, silence the bus master device.
    unsigned char master_id = wait_for_bus_master(5000);
    if(master_id > 0) {
      bus_master_id = master_id;
      emit log("Got heartbeat frame for current master " + QString::number(master_id,16),LOGLEVEL_DETAIL);
      bool silence_success = silence_module(master_id);
      if(silence_success == true) {
        emit log("Successfully connected to the ALDL bus.",LOGLEVEL_STATUS);
        return true;
      }
    } else {
      // now see if the bus is actually silent.
      port->waitForReadyRead(500);
      QByteArray noise = port->readAll();
      if(noise.isEmpty()) {
        emit log("The bus is silent.",LOGLEVEL_STATUS);
        keepalive_timer->start(); // now we need keepalives to maintain silence.
        return true;
      } else {
        emit log("The bus is still noisy (retry)...",LOGLEVEL_ERROR);
      }
    }
  }

  emit log("Could not become the ALDL bus master.",LOGLEVEL_ERROR);
  return false;
}

unsigned char datastream_8192::wait_for_bus_master(int msTimeout) {
  QElapsedTimer t;
  t.start();

  emit log("Listening for ALDL heartbeat to determine current bus master...",LOGLEVEL_DETAIL);
  purge_bus();

  QByteArray buf_in;

  while(t.hasExpired(msTimeout) == false) {
    if(port->waitForReadyRead(20) == false) {
      if(buf_in.isEmpty() && t.elapsed() > 500) return 0x00;
      continue;
    }

    buf_in.append(port->readAll());

    // minimum 4 bytes.
    if(buf_in.size() < 4) continue;

    // the location we are testing for vailidity as a heartbeat.  start 4 bytes back.
    int cursor = buf_in.size() - 1 - 4;

    // search backwards for the latest 0xF0
    while(cursor > 0) {
      cursor--;
      if((unsigned char)buf_in.at(cursor) == 0xF0 &&
         (unsigned char)buf_in.at(cursor + 1) == 0x56 &&
         (unsigned char)buf_in.at(cursor + 2) > 0x52) {
        if(test_checksum(buf_in.mid(cursor,4)) == true) {
          return (unsigned char)buf_in.at(cursor + 2); // valid.  return id of bus master.
        } else {
          continue;
        }
      }
    }
  }

  emit log("Timed out listening for heartbeat.",LOGLEVEL_DETAIL);
  return 0x00;
}

bool datastream_8192::serial_discard(int gap, int timeout) {
  QElapsedTimer t;
  t.start();

  QByteArray discarded;

  while(port->waitForReadyRead(gap) == true) {
    discarded.append(port->readAll());
    if(t.elapsed() > timeout) {
      emit log("Error purging bus: traffic exceeded timeout length.",LOGLEVEL_DETAIL);
      #ifdef LOG_DISCARDED_TRAFFIC
      emit log("Discarded " + QString::number(t.elapsed()) + "ms of idle traffic: " + discarded.toHex().toUpper(),LOGLEVEL_COMM);
      #endif
      return true;
    }
  }

  if(discarded.isEmpty() == false) {
#ifdef LOG_DISCARDED_TRAFFIC
    emit log("Discarded " + QString::number(t.elapsed()) + "ms of idle traffic: " + discarded.toHex().toUpper(),LOGLEVEL_COMM);
#endif
    return true;
  } else {
    return false;
  }
}

bool datastream_8192::silence_module(unsigned char device) {
  // the basic concept is we want to hit our heartbeat frame in the middle, because of unknown interface lag.
  // each iteration we add an additional 1ms of pre-delay but reset that pre-delay at 10 in case we've missed.
  // this should cover most serial timing possibilities and error rates.

  emit log("Silencing bus master device " + QString::number(device,16),LOGLEVEL_DETAIL);

  // store request as bytes just because we might be using it quite a bit
  datastream_request req;
  req.device_id = device;
  req.command_id = 0x08;
  QByteArray shutup_request = message_to_packet(req);

  // we only need half the heartbeat due to preemption.
  QByteArray heartbeat;
  heartbeat.append(0xF0);
  heartbeat.append(0x56);

  // for main timeout
  QElapsedTimer total_elapsed;
  total_elapsed.start();

  QSettings s;

  // configuration -------------------------------------------------
  // default to a reliable configuration
  int timeout = 20000;             // ms until fail
  int min_silence_length = 300;    // ms of silence until win
  int max_heartbeat_window = 12;   // heartbeat frame must be in last window bytes
  bool log_idle_traffic = get_switch("ALDL_DISPLAY_IDLE_TRAFFIC",false);
  int buf_err_siz = 1024;          // when buffer is this large, probably timing is lunched
  int alt_heartbeat_window = 500;  // ... so use a massive window instead
  int predelay = 0;                // pre-emption delay (seems like 2ms would be good)
  int max_predelay = 10;           // when predelay exceeds this ms value reset to 0
  int postdelay = 12;              // ms after silence request is sent
  // ---------------------------------------------------------------
  // here's the fast configuration if selected
  if(get_switch("ALDL_FAST_CONNECTION",false) == true) {
    timeout = 8000;
    min_silence_length = 100;
    max_heartbeat_window = 5;
    max_predelay = 5;
    alt_heartbeat_window = 12;
    postdelay = 8;
    predelay = 2;
  }
  //----------------------------------------------------------------

  // pull last pre-delay value that was successful.. but subtract 1.
  predelay = s.value("8192_LAST_PREDELAY",predelay).toInt();
  if(predelay > 2) predelay--;

  // secondary buffer
  QByteArray buf_in;

  // run main loop until timeout expires
  while(total_elapsed.elapsed() < timeout) {

    // get data...
    if(log_idle_traffic == true) {
      // log in chunks if configured to do so
      QByteArray chunk_in = port->readAll();
      buf_in.append(chunk_in);
      if(chunk_in.size() > 0) emit log("Got idle traffic: " + chunk_in.toHex().toUpper(),LOGLEVEL_COMM);
    } else {
      buf_in.append(port->readAll());
    }

    // wait for just the first two bytes of the heartbeat to appear in the secondary buffer within the window.
    if(buf_in.right(max_heartbeat_window).contains(heartbeat) == true) {
      emit log("Found heartbeat, sending mode 8 request with predelay " + QString::number(predelay),LOGLEVEL_DEBUG);

      // restart the buffer to clear the current heartbeat out of the window
      buf_in.clear();

      // incremental but resetting predelay
      sleep_ms(predelay);
      predelay++;
      if(predelay > max_predelay) predelay = 0;

      // send the request
      serial_write(shutup_request);

      // post delay and clear.  this effectively removes some of the search time for min_silence_length,
      // but doesn't seem to hurt.  there's no way any action will be required in this time.
      sleep_ms(postdelay);
      port->clear();      
    }

    // see if the buffer is filling heavily, which means we're looping hard and overshooting our window
    // probably due to a really laggy serial interface.
    if(buf_in.size() > buf_err_siz) {
      emit log("We are stuck trying to time the heartbeat, trying a larger window size...",LOGLEVEL_ERROR);
      max_heartbeat_window = alt_heartbeat_window; // switch to large window size for remaider
    }

    // wait for more data or (hopefully) silence
    if(port->waitForReadyRead(min_silence_length) == false) {
      emit log("Attained bus silence, stable connection is now likely.",LOGLEVEL_COMM);
      int predelay_memory = predelay;
      s.setValue("8192_LAST_PREDELAY",predelay_memory);
      port->readAll(); // just in case ...
      return true; // bus silent
    }
  }
  return false;
}

bool datastream_8192::open_port() {

  if(state > COMM_STATE_CLOSED) return true;

  if(port->isOpen() == true) {
    set_state(COMM_STATE_OPEN);
    return true;
  }

  // select our port.  first try the one in settings.
  QStringList interface_list = dialog_interface_select::serial_interface_list();
  QSettings s;
  interface_id = s.value("SERIAL_PORT",QString()).toString();

  // if the port doesn't exist clear it.
  if(interface_id.isEmpty() == false && interface_list.contains(interface_id) == false) interface_id.clear();

  // choose the serial port automatically if necessary.
  if(interface_id.isEmpty()) {
    QString ftdi_if = dialog_interface_select::first_ftdi_interface();
    if(ftdi_if.isEmpty()) {
      emit log("No valid serial port is selected, and no FTDI interface could be detected.  Select a serial port manually.",LOGLEVEL_ERROR);
      return false;
    } else {
      interface_id = ftdi_if;
      emit log("Autoselected FTDI interface at " + interface_id,LOGLEVEL_DETAIL);
    }
  }

  // set the port.
  port->setPortName(interface_id);

  //-----------------
  if(port->open(QIODevice::ReadWrite)) {
    if(port->baudRate() != 8192) {
      emit log("Serial interface cannot set baud rate to 8192 (actual rate " + QString::number(port->baudRate()),LOGLEVEL_ERROR);
      port->close();
      set_state(COMM_STATE_CLOSED);
      return false;
    }
    emit log("SERIAL: Opened port " + port->portName(),LOGLEVEL_DETAIL);
    emit log("SERIAL: Port info: " + interface_info(port->portName()),LOGLEVEL_DETAIL);
    set_state(COMM_STATE_OPEN);
    return true;
  } else {
    emit log("Could not open port: " + interface_id + "(" + port->errorString() + ")",LOGLEVEL_ERROR);
    set_state(COMM_STATE_CLOSED);
    return false;
  }
}

void datastream_8192::purge_bus(int timeout) {
  port->clear(); // clear existing
  serial_discard(10,timeout); // wait until more data stops arriving
}

bool datastream_8192::serial_write(const QByteArray &packet) {
  keepalive_timer->start();

  port->clear();

  port->write(packet);

  while(port->bytesToWrite() > 0) {
    if(port->waitForBytesWritten(1000) == false) {
      emit log("Error writing message to port: " + packet.toHex().toUpper(),LOGLEVEL_COMM);
      return false;
    }
  }

  emit log("Sent message: " + packet.toHex().toUpper(),LOGLEVEL_COMM);

  return true;
}

bool datastream_8192::close_port() {
  if(port->isOpen() == false) return true; // alraedy closed
  disconnect_from_bus();
  sleep_ms(15);
  port->close();
  set_state(COMM_STATE_CLOSED);
  return true; // can't really fail.
}
