#ifndef DATASTREAM_MESSAGE_H
#define DATASTREAM_MESSAGE_H

#include "enums.h"
#include <QObject>

class datastream_reply;
class datastream_message;
class datastream_request;

class datastream_message {
public:
  datastream_message();

  int device_id = 0;
  int command_id = 0;
  int priority_id = 0;

  // get functions
  unsigned char at(unsigned int position) const; // returns 0 if out of range
  unsigned char first() const; // return first byte of data
  quint32 get32(unsigned int position);
  quint64 get64(unsigned int position);
  quint16 get16(unsigned int position);
  static quint32 get32(unsigned int position, const QByteArray &b);
  static quint64 get64(unsigned int position, const QByteArray &b);
  static quint16 get16(unsigned int position, const QByteArray &b);
  QByteArray get_data() const;
  QString to_string(); // as hex.  pretty much for debug output.

  // set functions
  void set_first(unsigned char c); // set first byte of data
  void set(unsigned int position, unsigned char c); // 8 bit
  void set16(unsigned int position, quint16 n);
  void set32(unsigned int position, quint32 n);
  void set64(unsigned int position, quint64 n);
  void append(unsigned char c); // append to data
  void append(QByteArray b);
  void append16(quint16 n);
  void append32(quint16 n);
  void append64(quint16 n);
  void set_data(const QByteArray &d);

  void clear(); // clear data (leaves device id and command id alone)
  unsigned int size() const; // get size of data

  // these are convenience functions for external use
  static QByteArray n_to_array16(quint16 n);
  static QByteArray n_to_array32(quint32 n);
  static QByteArray n_to_array64(quint64 n);

protected:
  QByteArray data;

private:
  void upsize(unsigned int position, int bytes); // resize the container if necessary to fit region.
};

class datastream_request : public datastream_message {
public:
  datastream_request();
  datastream_request(unsigned int device, unsigned int command, unsigned int priority = 0x6C);

  // the datastream can take this as an override if != -1
  int override_timeout = -1;
  int override_retries = -1;
};

class datastream_reply : public datastream_message {
public:
  // in case things do not go well.
  bool success = true;
  QString error_message;
  _reply_errortype error_type = ERRORTYPE_UNKNOWN;
  static datastream_reply fail(QString err_msg);
};

#endif // DATASTREAM_MESSAGE_H
