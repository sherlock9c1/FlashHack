#include "processor.h"
#include <QFile>
#include <QDir>
#include <QCoreApplication>
#include <QSettings>

processor::processor() {

}

processor::~processor() {

}

QString processor::get_name() {
  return QString();
}

void processor::dbg_a() {
  errmsg("This processor hasn't assigned a debug function to this thing yet.");
}

void processor::dbg_b() {
  errmsg("This processor hasn't assigned a debug function to this thing yet.");
}

void processor::dbg_c() {
  errmsg("This processor hasn't assigned a debug function to this thing yet.");
}

void processor::set_interface(datastream_interface *i) {
  interface = i;
  connect(i,SIGNAL(log(QString,int)),this,SIGNAL(log(QString,int)));
}

QString processor::get_info() {
  return QString();
}

QList<parameter_def> processor::get_parameter_list() {
  QList<parameter_def> out;
  return out;
}

bool processor::has_capability(QString capability_name) {
  // expose the testability of the interface here.
  if(capability_name == "TEST_INTERFACE") return interface->interface_testable();

  Q_UNUSED(capability_name);
  return false;
}

QString processor::get_program_dir() {
  return QCoreApplication::applicationDirPath();
}

QByteArray processor::get_datafile(QString program_id, QString subdir) {
  QString file_path = get_program_dir();
  if(subdir.isEmpty() == false) file_path.append("/" + subdir);
  file_path.append("/" + program_id + ".BIN");
  QFile f(file_path);
  if(f.open( QIODevice::ReadOnly ) == false) {
    debugmsg("Could not open file: " + file_path);
    return QByteArray();
  }
  QByteArray b = f.readAll();
  f.close();
  return b;
}

bool processor::set_datafile(QString program_id, QString subdir, const QByteArray &data) {
  QString file_path = get_program_dir();
  if(subdir.isEmpty() == false) file_path.append("/" + subdir);
  file_path.append("/" + program_id + ".BIN");
  QFile f(file_path);
  if(f.open( QIODevice::WriteOnly ) == false) {
    errmsg("Could not open file: " + file_path);
    return false;
  }
  if(f.write(data) == -1) return false;
  f.close();
  return true;
}

QStringList processor::programs_in_subdir(QString subdir) {
  QString file_path = get_program_dir();
  if(subdir.isEmpty() == false) file_path.append("/" + subdir);
  QDir d(file_path);
  QStringList in = d.entryList();
  QStringList out;
  while(in.isEmpty() == false) {
    QString x = in.takeLast();
    if(x.endsWith(".BIN",Qt::CaseInsensitive) == true) {
      x.chop(4);
      out.append(x);
    }
  }
  return out;
}

void processor::start(QString message) {
  emit start_procedure(message);
}

void processor::success(QString message) {
  statusmsg(message);
  emit end_procedure(message, true);
}

void processor::fail(QString message) {
  errmsg(message);
  emit end_procedure(message, false);
}

bool processor::load_program(QString program_id, QString subdir, unsigned int offset, int timeout) {
  QByteArray b = get_datafile(program_id,subdir);
  if(b.isEmpty()) return false;
  add_program(program_id,b,offset,timeout);
  return true;
}

bool processor::load_program_padded(QString program_id, QString subdir, unsigned char pad_byte, int timeout) {
  QByteArray b = get_datafile(program_id,subdir);
  if(b.isEmpty()) return false;
  int offset = 0;

  // locate the offset based on the first char that is not a padding byte.
  for(offset = 0;offset < b.size();offset++) {
    if((unsigned char)b.at(offset) != pad_byte) break;
  }

  // catch an empty (all padding) file
  if(offset + 1 >= b.size()) {
    errmsg("Program " + program_id + " is all padding.");
    return false;
  }

   // relocate code to 0
  b.remove(0,offset);
  add_program(program_id,b,offset,timeout);
  return true;
}

bool processor::get_switch(const QString &option_name) {
  QSettings s;
  return s.value(option_name,false).toBool();
}

QVariant processor::get_setting(const QString &option_name) {
  QSettings s;
  return s.value(option_name,false);
}

void processor::add_program(QString program_id, QByteArray data, unsigned int offset, int timeout) {
  if(programs.contains(program_id) == true) {
    errmsg("Warning: Loading a program twice, this overwrites the old program: " + program_id);
  }
  program_definition d;
  d.data = data;
  d.offset = offset;
  d.timeout = timeout;
  d.program_type = program_definition::PROGRAM_NORMAL;
  programs[program_id] = d;
  debugmsg("Added program " + program_id + " length " +
           QString::number(data.size()) + " bytes with offset " + QString::number(offset,16));
}

void processor::add_subroutine(QString program_id, unsigned int address, int timeout) {
  if(programs.contains(program_id) == true) {
    errmsg("Warning: Loading a program twice, this overwrites the old program: " + program_id);
  }
  program_definition d;
  d.offset = address;
  d.timeout = timeout;
  d.program_type = program_definition::PROGRAM_SUBROUTINE;
  programs[program_id] = d;
  debugmsg("Defined subroutine " + program_id + " called at offset " + QString::number(address,16));
}

void processor::configure() {

}

datastream_reply processor::execute_program(int device_id, QString program_name) {
  Q_UNUSED(device_id);
  Q_UNUSED(program_name);
  datastream_reply r;
  r.success = false;
  return r;
}

bool processor::raw_command(int device_id, int command_id, QByteArray command) {
  datastream_request r(device_id,command_id);
  r.set_data(command);
  debugmsg("Sending raw command: " + r.to_string());
  datastream_reply repl = interface->request(r);
  if(repl.success == true) {
    statusmsg("Got reply to command: " + repl.to_string());
  } else {
    errmsg("Raw command not successful: " + r.to_string());
  }
  return repl.success;
}

bool processor::disconnect_bus() {
  return interface->close_port();
}

bool processor::load_kernel() {
  return false;
}

bool processor::unload_kernel() {
  return false;
}

bool processor::flash_write() {
  errmsg("This processor does not support the flash write function.");
  return false;
}

bool processor::flash_read() {
  errmsg("This processor does not support the flash read function.");
  return false;
}

bool processor::load_bin(QString path_to_bin) {
  Q_UNUSED(path_to_bin);
  errmsg("This processor does not support the load bin function.");
  return false;
}

bool processor::save_bin(QString path_to_bin) {
  Q_UNUSED(path_to_bin);
  errmsg("This processor does not support the save bin function.");
  return false;
}

bool processor::set_vin(QByteArray vin) {
  Q_UNUSED(vin);
  errmsg("This processor does not support the set vin function.");
  return false;
}

bool processor::set_calid(QVariant calid) {
  Q_UNUSED(calid);
  errmsg("This processor does not support the set calid function.");
  return false;
}

bool processor::interface_test() {
  start_program_operation(0x00,"INTERFACE","Test",0xff);
  start_procedure("Interface test");
  bool status = interface->test_interface();
  end_procedure("Interface test",status);
  return status;
}

bool processor::interface_function(int fn) {
  interface->test_function(fn);
  return true;
}

bool processor::scan_idle_traffic() {
  interface->test_function(datastream_interface::TESTFUNCTION_SCAN_IDLE);
  return false;
}

void processor::statusmsg(QString message) {
  emit log(message,LOGLEVEL_STATUS);
}

void processor::detailmsg(QString message) {
  emit log(message,LOGLEVEL_DETAIL);
}

bool processor::is_cancelled() {
  if(cancel_operation == true) {
    cancel_operation = false;
    return true;
  }
  return false;
}

void processor::debugmsg(QString message) {
  emit log(message,LOGLEVEL_DEBUG);
}

void processor::errmsg(QString message) {
  emit log(message,LOGLEVEL_ERROR);
}

QString processor::device_name(int device_id) {
  Q_UNUSED(device_id);
  return "UNKNOWN";
}
