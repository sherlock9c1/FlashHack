#include "datastream_message.h"
#include <QDebug>

Q_DECLARE_METATYPE(datastream_reply)
Q_DECLARE_METATYPE(datastream_message)
Q_DECLARE_METATYPE(datastream_request)

datastream_message::datastream_message() {

}

void datastream_message::upsize(unsigned int position, int bytes) {
  // append zeros until we are at the desired size.
  while(size() < position + bytes) data.append((char)0);
}

void datastream_message::set(unsigned int position, unsigned char c) {
  upsize(position,1);
  data[position] = c;
}

void datastream_message::set16(unsigned int position, quint16 n) {
  unsigned int s = sizeof(n);
  upsize(position,s);
  for(unsigned int x=0;x<s;x++) data[position + s - 1 - x] = ( n >> ( 8 * x ) ) & 0xFF;
}

quint16 datastream_message::get16(unsigned int position) {
  quint16 out = 0;
  unsigned int s = sizeof(quint16);
  for(unsigned int x=0;x<s;x++) out |= at(position + s - 1 - x) << ( 8 * x );
  return out;
}

quint32 datastream_message::get32(unsigned int position, const QByteArray &b) {
  quint32 out = 0;
  unsigned int s = sizeof(quint32);
  for(unsigned int x=0;x<s;x++) out |= (unsigned char)b.at(position + s - 1 - x) << ( 8 * x );
  return out;
}

quint64 datastream_message::get64(unsigned int position, const QByteArray &b) {
  quint32 out = 0;
  unsigned int s = sizeof(quint64);
  for(unsigned int x=0;x<s;x++) out |= (unsigned char)b.at(position + s - 1 - x) << ( 8 * x );
  return out;
}

quint16 datastream_message::get16(unsigned int position, const QByteArray &b) {
  quint32 out = 0;
  unsigned int s = sizeof(quint16);
  for(unsigned int x=0;x<s;x++) out |= (unsigned char)b.at(position + s - 1 - x) << ( 8 * x );
  return out;
}

quint32 datastream_message::get32(unsigned int position) {
  quint32 out = 0;
  unsigned int s = sizeof(quint32);
  for(unsigned int x=0;x<s;x++) out |= at(position + s - 1 - x) << ( 8 * x );
  return out;
}

quint64 datastream_message::get64(unsigned int position) {
  quint64 out = 0;
  unsigned int s = sizeof(quint64);
  for(unsigned int x=0;x<s;x++) out |= at(position + s - 1 - x) << ( 8 * x );
  return out;
}

void datastream_message::set32(unsigned int position, quint32 n) {
  unsigned int s = sizeof(n);
  upsize(position,s);
  for(unsigned int x=0;x<s;x++) data[position + s - 1 - x] = ( n >> ( 8 * x ) ) & 0xFF;
}

void datastream_message::set64(unsigned int position, quint64 n) {
  unsigned int s = sizeof(n);
  upsize(position,s);
  for(unsigned int x=0;x<s;x++) data[position + s - 1 - x] = ( n >> ( 8 * x ) ) & 0xFF;
}

void datastream_message::append(unsigned char c) {
  data.append(c);
}

void datastream_message::append(QByteArray b) {
  data.append(b);
}

void datastream_message::append16(quint16 n) {
  set16(size(),n);
}

void datastream_message::append32(quint16 n) {
  set32(size(),n);
}

void datastream_message::append64(quint16 n) {
  set64(size(),n);
}

void datastream_message::clear() {
  data.clear();
}

unsigned char datastream_message::at(unsigned int position) const {
  if(position > size() - 1) return 0x00;
  return (unsigned char)data.at(position);
}

unsigned char datastream_message::first() const {
  return at(0);
}

void datastream_message::set_first(unsigned char c) {
  set(0,c);
}

unsigned int datastream_message::size() const {
  return data.size();
}

QByteArray datastream_message::get_data() const {
  return data;
}

void datastream_message::set_data(const QByteArray &d) {
  data.clear();
  data = d;
}

datastream_request::datastream_request() {

}

datastream_request::datastream_request(unsigned int device, unsigned int command, unsigned int priority) {
  this->device_id = device;
  this->command_id = command;
  this->priority_id = priority;
}

QByteArray datastream_message::n_to_array16(quint16 n) {
  QByteArray out;
  out.resize(sizeof(n));
  for(unsigned int x=0;x<sizeof(n);x++) out[(int)sizeof(n) - x - 1] = ( n >> ( 8 * x ) ) & 0xFF;
  return out;
}

QByteArray datastream_message::n_to_array32(quint32 n) {
  QByteArray out;
  out.resize(sizeof(n));
  for(unsigned int x=0;x<sizeof(n);x++) out[(int)sizeof(n) - x - 1] = ( n >> ( 8 * x ) ) & 0xFF;
  return out;
}

QByteArray datastream_message::n_to_array64(quint64 n) {
  QByteArray out;
  out.resize(sizeof(n));
  for(unsigned int x=0;x<sizeof(n);x++) out[(int)sizeof(n) - x - 1] = ( n >> ( 8 * x ) ) & 0xFF;
  return out;
}

QString datastream_message::to_string() {
  QByteArray out = data;
  out.prepend(command_id);
  return "[DEV:" + QString::number(device_id,16) + "] " + out.toHex(' ').toUpper();
}

datastream_reply datastream_reply::fail(QString err_msg) {
  datastream_reply r;
  r.success = false;
  r.error_message = err_msg;
  r.error_type = ERRORTYPE_UNKNOWN;
  return r;
}
