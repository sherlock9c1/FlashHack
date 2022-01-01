#ifndef TOOL_FLASH_H
#define TOOL_FLASH_H

#include <QWidget>
#include <QThread>
#include <QPushButton>
#include "flash/processor.h"
#include "flash/processor_8192dualflash.h"

namespace Ui {
  class tool_flash;
}

class tool_flash : public QWidget {
  Q_OBJECT

public:
  explicit tool_flash(processor *proc, datastream_interface *interface, QWidget *parent = nullptr);
  ~tool_flash(); 
  void quit();

public slots:
  void start_program_operation(int device_id, QString device_name, QString operation_name, int eeprom_size);
  void end_program_operation(int device_id, QString device_name, QString operation_name, bool success);
  void set_region_state(int device_id, int offset, int size, int state);

  void show_stopped_controls();
  void show_running_controls();

protected:
  void reload_parameters();

private slots:
  void on_debug_send_e_clicked();
  void on_cancel_btn_clicked();
  void on_read_btn_clicked();
  void on_write_btn_clicked();
  void on_load_bin_btn_clicked();
  void on_save_bin_btn_clicked();
  void on_load_kernel_btn_clicked();
  void on_reboot_button_clicked();
  void on_dbg_1_clicked();
  void on_dbg_2_clicked();
  void on_dbg_3_clicked();
  void on_vin_btn_clicked();
  void on_calid_btn_clicked();
  void procedure_complete_message(QString message, bool success);
  void procedure_start_message(QString message);
  void on_raw_command_btn_clicked();
  void on_disconnect_btn_clicked();

  void on_testinterface_btn_clicked();

  void on_scanidle_btn_clicked();

  void on_restore_defaults_btn_clicked();

private:
  Ui::tool_flash *ui;

  processor *proc = nullptr;
  QThread *proc_thread = nullptr;

  void run_command(const char *method);
  void run_interface_selector();
  void execute_program(int device_id, QString program_name);
  void start_processor_thread();
  QString region_state_string(_program_region_state region_state);
  void errmsg(QString message);

  void show_button_for_capability(QPushButton *b, const QString &capability);

  QString test_interface_conditions;
};

#endif // TOOL_FLASH_H
