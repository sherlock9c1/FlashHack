#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <QObject>
#include "binfile.h"
#include "../datastream/datastream_interface.h"
#include "setting_ui.h"
#include <QMap>
#include <QVariant>

class program_definition {
public:
  enum _program_type {
    PROGRAM_NORMAL, PROGRAM_SUBROUTINE
  };
  int program_type = PROGRAM_NORMAL;
  unsigned int offset;
  int timeout;
  QByteArray data;
};

class processor : public QObject {
  Q_OBJECT

public:
  processor();
  ~processor();

  void set_interface(datastream_interface *i);

  // set this flag to cancel the current operation when safe, if the operation supports that.
  // we do not use signalling because it's possible we want to cancel a synchronus operation
  // from another thread.
  bool cancel_operation = false;

  // these information methods are called by the ui before the processor is moved to its thread.
  virtual QString get_name();
  virtual QString get_info();

  QString get_program_dir();

  virtual QList <parameter_def>get_parameter_list();
  virtual bool has_capability(QString capability_name);

  // when loading/reading bin, get these IDs.
  QString current_vin;
  int current_cal_id = 0;

signals:
  void start_procedure(QString procedure);
  void end_procedure(QString message, bool success);

  void log(QString message, int loglevel);

  void kernel_loaded(int device_id);
  void kernel_unloaded(int device_id);

  // for programming status.
  void start_program_operation(int device_id, QString device_name, QString operation, int eeprom_size);
  void end_program_operation(int device_id, QString device_name, QString operation, bool success);

  void region_changed(int device_id, int offset, int size, int state);
  void progress_changed(int percentage);
  void bin_read_complete();

public slots:
  // you must reimplement these functions and also has_capability to enable a feature in your subclass.
  virtual void configure();
  virtual bool load_kernel();
  virtual bool unload_kernel();
  virtual bool flash_write();
  virtual bool flash_read();
  virtual bool load_bin(QString path_to_bin);
  virtual bool save_bin(QString path_to_bin);
  virtual bool set_vin(QByteArray vin);
  virtual bool set_calid(QVariant calid);
  virtual datastream_reply execute_program(int device_id, QString program_name);
  bool raw_command(int device_id, int command_id, QByteArray command);
  bool disconnect_bus();

  virtual void dbg_a();
  virtual void dbg_b();
  virtual void dbg_c();

  bool interface_function(int fn);
  bool interface_test();

  bool scan_idle_traffic();
protected:
  datastream_interface *interface = nullptr;

  // executable program managment/storage
  bool load_program(QString program_id, QString subdir, unsigned int offset = 0x0000, int timeout = -1);
  bool load_program_padded(QString program_id, QString subdir, unsigned char pad_byte = 0x00, int timeout = -1); // padded up to offset
  void add_program(QString program_id, QByteArray data, unsigned int offset, int timeout = -1);
  void add_subroutine(QString program_id, unsigned int address, int timeout = -1);
  QMap <QString,program_definition>programs;
  QByteArray get_datafile(QString program_id, QString subdir);
  bool set_datafile(QString program_id, QString subdir, const QByteArray &data);
  QStringList programs_in_subdir(QString subdir);

  // this tends to display a popup message in the interface on success/fail.
  void start(QString message);
  void success(QString message);
  void fail(QString message);

  // configuration
  bool get_switch(const QString &option_name);
  QVariant get_setting(const QString &option_name);

  // get the name for a device id.  reccommended.
  virtual QString device_name(int device_id);

  void errmsg(QString s);
  void detailmsg(QString message);
  void debugmsg(QString s);
  void statusmsg(QString s);

  bool is_cancelled(); // return and clear the cancel_operation bit
};

#endif // PROCESSOR_H
