#ifndef DATASTREAM_INTERFACE_H
#define DATASTREAM_INTERFACE_H

#include "enums.h"
#include "datastream_message.h"
#include <QObject>
#include <QHash>
#include <QDebug>

class datastream_interface : public QObject {
  Q_OBJECT

public:
  enum _interface_mode {
    MODE_NORMAL,
    MODE_HIGHSPEED
  };

  enum _interface_function {
    TESTFUNCTION_SCAN_IDLE = 0x00
  };

  datastream_interface();

  virtual QStringList interface_list();
  virtual QString interface_info(QString interface_id);
  virtual QString interface_name();

  virtual bool test_interface();
  virtual QString test_interface_conditions() const;
  virtual bool interface_testable();

  QString last_error();
  _comm_state get_state();

  bool get_capability(const QByteArray &name);
  void set_capability(const QByteArray &name, bool setting);

  void sleep_ms(int ms);

  virtual void configure(); // this should be called by the processor in our new thread.

public slots:
  virtual bool open_port();
  virtual bool close_port();
  virtual bool change_interface(QString interface_name);
  virtual bool set_mode(_interface_mode mode);

  virtual datastream_reply request(const datastream_request &message);
  virtual datastream_reply get_reply(int device_id, int timeout_ms = 500);

  virtual void keepalive();

  virtual void test_function(int n);

signals:
  void state_changed(_comm_state s);
  void log(QString message, int loglevel);

protected:
  datastream_reply reply_error(QString error_message, _reply_errortype errortype);
  void set_state(_comm_state s);
  _comm_state state = COMM_STATE_CLOSED;
  QString interface_id;
  bool get_switch(QString switch_name, bool default_setting);
  int get_int(QString switch_name, int default_setting);

private:
  QString _last_error;
  QHash <QByteArray,bool>_capabilities;
};



#endif // DATASTREAM_INTERFACE_H
