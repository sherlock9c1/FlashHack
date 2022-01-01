#include "datastream_interface.h"
#include <QDebug>
#include <QThread>
#include <QSettings>

datastream_interface::datastream_interface() {

}

QStringList datastream_interface::interface_list() {
  return QStringList();
}

QString datastream_interface::interface_info(QString interface_id) {
  Q_UNUSED(interface_id);
  return QString();
}

QString datastream_interface::interface_name() {
  return QString("Undefined");
}

bool datastream_interface::test_interface() {
  log("No test available for this interface.",LOGLEVEL_STATUS);
  return false;
}

bool datastream_interface::interface_testable() {
  return false;
}

QString datastream_interface::test_interface_conditions() const {
  return QString();
}

bool datastream_interface::get_switch(QString switch_name, bool default_setting) {
  QSettings s;
  return s.value(switch_name,default_setting).toBool();
}

int datastream_interface::get_int(QString switch_name, int default_setting) {
  QSettings s;
  return s.value(switch_name,default_setting).toInt();
}

void datastream_interface::test_function(int n) {
  Q_UNUSED(n);
}

QString datastream_interface::last_error() {
  return _last_error;
}

_comm_state datastream_interface::get_state() {
  return state;
}

bool datastream_interface::get_capability(const QByteArray &name) {
  return _capabilities.value(name,false);
}

bool datastream_interface::open_port() {
  Q_UNUSED(interface_id);
  return false;
}

bool datastream_interface::close_port() {
  return false;
}

datastream_reply datastream_interface::request(const datastream_request &message) {
  Q_UNUSED(message);
  return reply_error("Request function not implemented in this interface.",ERRORTYPE_SYSTEM);
}

datastream_reply datastream_interface::get_reply(int device_id, int timeout_ms) {
  Q_UNUSED(device_id);
  Q_UNUSED(timeout_ms);
  return datastream_reply::fail("Reply function not implemented in this class.");
}

void datastream_interface::keepalive() {
  return;
}

bool datastream_interface::change_interface(QString interface_name) {
  interface_id = interface_name;
  return true;
}

bool datastream_interface::set_mode(datastream_interface::_interface_mode mode) {
  if(mode == MODE_NORMAL) return true;
  return false;
}

datastream_reply datastream_interface::reply_error(QString error_message,
                                       _reply_errortype errortype = ERRORTYPE_UNKNOWN) {
  emit log("Packet error: " + error_message,LOGLEVEL_COMM);
  datastream_reply reply;
  reply.success = false;
  reply.error_message = error_message;
  reply.error_type = errortype;
  return reply;
}

void datastream_interface::set_state(_comm_state s) {
  if(state == s) return;
  emit state_changed(s);
  state = s;
}

void datastream_interface::set_capability(const QByteArray &name, bool setting) {
  _capabilities[name] = setting;
}

void datastream_interface::sleep_ms(int ms) {
  QThread::msleep(ms);
}

void datastream_interface::configure() {

}

//-----------------------

