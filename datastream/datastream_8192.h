#ifndef DATASTREAM_8192_H
#define DATASTREAM_8192_H

#include "datastream_interface.h"
#include <QSerialPort>
#include <QTimer>

// read buffer of 12 packets maximum length packets.
// having more than one packet in the buffer is probably an error anyway.
#define MAX_BUF_SIZ ( 0xFF - 0x52 ) * 12

class datastream_8192 : public datastream_interface {
  Q_OBJECT
public:

  ~datastream_8192();
  enum { PRIMARY_ECM = 0xF4, SECONDARY_ECM = 0xE4, MOD_CCM = 0xF1, MOD_ABS = 0xF9};

  static unsigned char calc_checksum(const QByteArray &b);
  static bool test_checksum(const QByteArray &b);

  QString interface_name() override;
  QString interface_info(QString interface_id) override;

  void configure() override; // Don't forget to run this from the processor or it'll crash.

  QString test_interface_conditions() const override;
  bool test_interface() override;
  bool interface_testable() override;

public slots:
  datastream_reply request(const datastream_request &message) override;

  // lower level public operations
  QByteArray serial_read_size(int size, int timeout);
  bool serial_read_specific(QByteArray packet, int timeout);
  datastream_reply get_reply(int device_id, int timeout_ms = 500) override; // device_id = 0 means any device.

  // open and close the port
  bool open_port() override;
  bool close_port() override;

  // fairly universal aldl level functions
  unsigned char wait_for_bus_master(int msTimeout);
  bool silence_module(unsigned char device);
  datastream_reply resync_datastream(int timeout);

  void test_function(int n) override;

  void keepalive() override;

protected:
  unsigned char bus_master_id = 0x00;

  QByteArray message_to_packet(const datastream_request &message);

  int timeout_for_size(int size);

  bool serial_write(const QByteArray &packet);
  bool become_bus_master();
  bool serial_discard(int gap = 10, int timeout = 3000);
  void purge_bus(int timeout = 3000);

  QSerialPort *port = nullptr;

  bool connect_to_bus();
  bool disconnect_from_bus();

  datastream_reply _request(const datastream_request &message);

private:
  QTimer *keepalive_timer = nullptr;

  bool looptest(int iterations, int test_pkt_size);
  void scan_idle();
};
#endif // DATASTREAM_8192_H
