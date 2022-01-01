#ifndef DATASTREAM_OBDXPRO_H
#define DATASTREAM_OBDXPRO_H

#include <QObject>
#include "datastream_obdii.h"
#include <QSerialPort>
#include <QElapsedTimer>

#define DVI_REPLY_TIMEOUT 2000
#define DVI_RATE_LIMIT_MS 2

class datastream_obdxpro : public datastream_obdii {
public:
  datastream_obdxpro();
  ~datastream_obdxpro();

  enum _DVIcommand {
    // -----------------------
    DVI_RECEIVE =         0x08, // recieve a normal message from the cable (8 bit length)
    DVI_RECEIVE_LARGE =   0x09, // recieve a large message from the cable (16 bit length)
    //-------------------------
    DVI_COMM_ERROR =      0x80, // communication error reply from cable
    DVI_CONFIG_ERROR =    0x7F, // configuration error reply from cable
    //-------------------------
    DVI_SEND =            0x10, // send a normal message (8 bit length)
    DVI_SEND_LARGE =      0x11, // send a large message (16 bit length)
    //-------------------------
    DVI_INFO =            0x22, // get info about the cable
    //-------------------------
    DVI_CONFIG_SERIAL =   0x23,
    DVI_CONFIG_COMMAND =  0x24,
    DVI_CONFIG_PROTOCOL = 0x31,
      DVI_PROTO_SET =        0x01,
      DVI_PROTO_NET_ENABLE = 0x05,
      DVI_PROTO_VPW =        0x01,
    DVI_CONFIG_VPW =      0x33,
      DVI_VPW_FILTER_DEST =       0x00,
      DVI_VPW_FILTER_SRC =        0x01,
      DVI_VPW_FILTER_DEST_RANGE = 0x02,
      DVI_VPW_FILTER_SRC_RANGE =  0x03,
      DVI_VPW_SET_WRITE_IDLE =    0x04,
      DVI_VPW_SET_4X =            0x06,
    //-------------------------
    DVI_POLL_ADC =        0x35,
    DVI_RESET =           0x25,
    //-------------------------
    DVI_SEND_OK =         0x00,
    DVI_OFF =             0x00,
    DVI_ON =              0x01,
    DVI_LISTEN =          0x02
  };

  QString interface_name() override;

public slots:
  void configure() override;
  datastream_reply request(const datastream_request &message) override;
  datastream_reply get_reply(int device_id, int timeout_ms = 500) override;

  // open and close the port
  bool open_port() override;
  bool close_port() override;

  void test_function(int n) override;

  void keepalive() override;

  bool set_mode(_interface_mode mode) override;

protected:
  QSerialPort *port = nullptr;
  bool serial_write(QByteArray b);
  datastream_reply _request(const datastream_request &message);

  QByteArray dvi_rx();
  bool dvi_tx(unsigned char command, const QByteArray &data);
  QByteArray dvi_req(unsigned char command, const QByteArray &data);
  bool dvi_simple_request(unsigned char command, unsigned char expect_reply, const QByteArray &data);

  bool dvi_is_error(unsigned char error_type);
  void log_dvi_error(unsigned char error_type, unsigned char command_code, unsigned char error_code);

  bool dvi_filter_source(unsigned char device_id, bool allow);
  bool dvi_filter_dest(unsigned char device_id, bool allow);
  bool dvi_filter_source_range(unsigned char start_device, unsigned char end_device, bool allow);
  bool dvi_filter_dest_range(unsigned char start_device, unsigned char end_device, bool allow);

  bool dvi_init();

  QByteArray elm_rx(int timeout = 1000);
  bool elm_tx(QString s);

  QString xproinfo(); // get the below items.
  int get_firmware_version();
  int get_model_number();
  QString get_cable_name();
  QString get_unique_id();

  bool dvi_set_protocol(int protocol);
  bool dvi_enable_comms(bool enable);

  // these functions seem to be broken.
  int get_free_ram();
  bool restart();

  QElapsedTimer *ratelimit_timer;
};

#endif // DATASTREAM_OBDXPRO_H
