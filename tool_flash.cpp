#include "tool_flash.h"
#include "ui_tool_flash.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include "datastream/datastream_8192.h"
#include "dialog_interface_select.h"
#include "binvisual.h"
#include "useful.h"

tool_flash::tool_flash(processor *proc, datastream_interface *interface, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::tool_flash) {

  ui->setupUi(this);

  proc_thread = new QThread();

  this->proc = proc;

  // move the interface to the proc thread first.
  // allow null interfaces for processors that might not require one.
  if(interface != nullptr) {
    test_interface_conditions = interface->test_interface_conditions(); // copy this static data before relocating interface.
    //... after this we don't really have/want direct access to the interface.
    proc->set_interface(interface);
    connect(proc_thread,SIGNAL(finished()),interface,SLOT(deleteLater())); // delete the interface when the processor dies
    interface->moveToThread(proc_thread);
  }

  // bring in parameters from the processor before moving it over to its new thread,
  // this prevents calling methods in another thread .
  reload_parameters();

  // move the interface to its own thread
  start_processor_thread();

  // setup interface for 'procedure stopped'
  show_stopped_controls();
  ui->tabber->setCurrentIndex(0);
}

void tool_flash::reload_parameters() {
  // import the parameters list from the module
  ui->parameters->clear();
  QList <parameter_def>p = proc->get_parameter_list();
  while(p.isEmpty() == false) ui->parameters->add_parameter(p.takeFirst());

  // show/hide controls based on the capabillities of the module
  show_button_for_capability(ui->vin_btn,"SET_VIN");
  show_button_for_capability(ui->calid_btn,"SET_CALID");
  show_button_for_capability(ui->write_btn,"FLASH_WRITE");
  show_button_for_capability(ui->read_btn,"FLASH_READ");
  show_button_for_capability(ui->save_bin_btn,"FLASH_READ"); // no need to save if we can't read
  show_button_for_capability(ui->load_bin_btn,"FLASH_WRITE"); // no need to load if we can't write
  show_button_for_capability(ui->load_kernel_btn,"HAS_KERNEL");
  show_button_for_capability(ui->reboot_button,"HAS_KERNEL");
  show_button_for_capability(ui->testinterface_btn,"TEST_INTERFACE");
  show_button_for_capability(ui->testcomm_btn,"TEST_COMMS");
}

void tool_flash::show_button_for_capability(QPushButton *b, const QString &capability) {
  b->setHidden(!proc->has_capability(capability));
}

void tool_flash::quit() {
  proc_thread->quit();
}

void tool_flash::start_processor_thread() {
  // configure the processor when it is started in its own thread
  connect(proc_thread,SIGNAL(started()),proc,SLOT(configure()));

  // connection(s) necessary for logging to work
  connect(proc,SIGNAL(log(QString, int)),ui->log,SLOT(add_entry(QString, int)));

  // murder-suicide teardown.  first kill the processor when the thread dies then kill ourselves.
  connect(proc_thread,SIGNAL(finished()),proc,SLOT(deleteLater()));
  connect(proc_thread,SIGNAL(finished()),this,SLOT(deleteLater()));

  // status and context signalling.  these are for contextual updates of the ui and displaying operation progress.
  connect(proc,SIGNAL(start_program_operation(int, QString, QString, int)),this,SLOT(start_program_operation(int, QString, QString, int)));
  connect(proc,SIGNAL(end_program_operation(int, QString, QString, bool)),this,SLOT(end_program_operation(int, QString, QString, bool)));
  connect(proc,SIGNAL(region_changed(int,int,int,int)),ui->program_map,SLOT(set_region_state(int,int,int,int)));
  connect(proc,SIGNAL(region_changed(int,int,int,int)),this,SLOT(set_region_state(int,int,int,int)));
  connect(proc,SIGNAL(progress_changed(int)),ui->progress_bar,SLOT(setValue(int)));
  connect(proc,SIGNAL(bin_read_complete()),this,SLOT(on_save_bin_btn_clicked()));
  connect(proc,SIGNAL(start_procedure(QString)),this,SLOT(procedure_start_message(QString)));
  connect(proc,(SIGNAL(end_procedure(QString,bool))),this,SLOT(procedure_complete_message(QString,bool)));
  connect(proc,SIGNAL(start_procedure(QString)),this,SLOT(show_running_controls()));
  connect(proc,(SIGNAL(end_procedure(QString,bool))),this,SLOT(show_stopped_controls()));

  // move to thread and start.
  proc->moveToThread(proc_thread);
  proc_thread->start(QThread::TimeCriticalPriority); // the processor thread should be as fast as possible.
}

void tool_flash::show_stopped_controls() {
  ui->control_stacker->setCurrentIndex(0);
}

void tool_flash::show_running_controls() {
  ui->control_stacker->setCurrentIndex(1);
}

void tool_flash::procedure_start_message(QString message) {
  Q_UNUSED(message);
}

void tool_flash::procedure_complete_message(QString message, bool success) {
  QMessageBox *m = new QMessageBox();

  if(success == true) {
    m->setIcon(QMessageBox::Information);
    m->setWindowTitle("flashack: Complete");
      m->setText("Procedure complete:\n" + message);
  } else {
    m->setIcon(QMessageBox::Critical);
    m->setWindowTitle("flashack: Fail");
    m->setText("Procedure failed:\n" + message);
  }

  m->setAttribute(Qt::WA_DeleteOnClose);

  m->show();
}

QString tool_flash::region_state_string(_program_region_state region_state) {
  switch(region_state) {
  case REGION_ERROR:
    return QStringLiteral("ERR");
    break;
  case REGION_ERASING:
    return QStringLiteral("ERASING");
    break;
  case REGION_ERASED:
    return QStringLiteral("ERASED");
    break;
  case REGION_UNKNOWN:
    return QStringLiteral("UNKNOWN");
    break;
  case REGION_WRITTEN:
    return QStringLiteral("WRITTEN");
    break;
  case REGION_READ:
    return QStringLiteral("READ");
    break;
  case REGION_PROGRAMMING:
    return QStringLiteral("PROCESSING");
    break;
  case REGION_EXEC:
    return QStringLiteral("EXEC");
  default:
    return QString();
  }
}

void tool_flash::set_region_state(int device_id, int offset, int size, int state) {
  Q_UNUSED(device_id);
  ui->program_status->setText("0x" + QString::number(offset,16).toUpper() + "[" +
                              QString::number(size) + "] " +
                              region_state_string((_program_region_state)state));
}

void tool_flash::start_program_operation(int device_id, QString device_name, QString operation_name, int eeprom_size) {
  // set status
  Q_UNUSED(operation_name);
  ui->program_map->start_new_bin(device_id,eeprom_size,device_name);

}

void tool_flash::end_program_operation(int device_id, QString device_name, QString operation_name, bool success) {
  Q_UNUSED(device_id);
  Q_UNUSED(device_name);
  Q_UNUSED(operation_name);
  if(success == true) {
    ui->program_status->setText("COMPLETE");
  } else {
    ui->program_status->setText("FAIL");
  }
}

void tool_flash::run_command(const char* method) {
  proc->cancel_operation = false;
  QMetaObject::invokeMethod(proc,method);
}

void tool_flash::execute_program(int device_id, QString program_name) {
  proc->cancel_operation = false;
  QMetaObject::invokeMethod(proc,"execute_program",Q_ARG(int,device_id),Q_ARG(QString,program_name));
}

tool_flash::~tool_flash() {
  delete ui;
}

void tool_flash::on_debug_send_e_clicked() {
  bool ok;
  int device_id = ui->debug_run_device->text().toInt(&ok,16);
  if(ok == false) {
    errmsg("Invalid device.");
    return;
  }
  execute_program(device_id,ui->debug_run_program->text());
}

void tool_flash::on_cancel_btn_clicked() {
  proc->cancel_operation = true;
}

void tool_flash::on_read_btn_clicked() {
  run_command("flash_read");
}

void tool_flash::on_write_btn_clicked() {
  run_command("flash_write");
}

void tool_flash::on_load_bin_btn_clicked() {
  QString name = QFileDialog::getOpenFileName(nullptr,"Select source bin file...",QString(),"Bin File (*.bin)");
  if(name.isEmpty() == true) return;
  QMetaObject::invokeMethod(proc,"load_bin",Q_ARG(QString,name));
}

void tool_flash::on_save_bin_btn_clicked() {
  QString name = QFileDialog::getSaveFileName(nullptr,"Select destination",QString(),"Bin File (*.bin)");
  if(name.isEmpty() == true) return;
  QMetaObject::invokeMethod(proc,"save_bin",Q_ARG(QString,name));
}

void tool_flash::on_load_kernel_btn_clicked() {
  run_command("load_kernel");
}

void tool_flash::on_reboot_button_clicked() {
  run_command("unload_kernel");
}

void tool_flash::on_dbg_1_clicked() {
  //errmsg("This is a function to show a really long error message to see how the thing behaves when a really long error message is shown.");
  run_command("dbg_a");
}

void tool_flash::on_dbg_2_clicked() {
  run_command("dbg_b");
}

void tool_flash::on_dbg_3_clicked() {
  procedure_complete_message("Test",true);
  run_command("dbg_c");
}

void tool_flash::on_vin_btn_clicked() {
  bool ok;
  QByteArray v = QInputDialog::getText(this, "Input VIN","VIN#:", QLineEdit::Normal,
                                      proc->current_vin, &ok).toLatin1().toUpper();
  if (ok && !v.isEmpty()) {
    if(vin::validate(v) == false) {
      errmsg("Beware, the VIN# " + QString(v) + " is not valid.  Programming it anyway...");
    }
    QMetaObject::invokeMethod(proc,"set_vin",Q_ARG(QByteArray,v));
  }
}

void tool_flash::on_calid_btn_clicked() {
  bool ok;
  QString calid = QInputDialog::getText(this, "Input CAL ID",
                                        "CAL ID:", QLineEdit::Normal,
                                        QString::number(proc->current_cal_id), &ok);
  if (ok && !calid.isEmpty()) {
    QMetaObject::invokeMethod(proc,"set_calid",Q_ARG(QVariant,calid));
  }
}

void tool_flash::errmsg(QString message) {
  ui->log->add_entry(message,LOGLEVEL_ERROR);
}

void tool_flash::on_raw_command_btn_clicked() {
  bool conv;

  int device_id = ui->raw_command_device->text().toUInt(&conv,16);
  if(conv == false) {
    errmsg("Invalid device ID.  Input in hex.");
    return;
  }

  int command_id = ui->raw_command_id->text().toUInt(&conv,16);
  if(conv == false) {
    errmsg("Invalid command ID.  Input in hex.");
    return;
  }

  QByteArray payload = QByteArray::fromHex(ui->raw_command_payload->text().toLocal8Bit());

  QMetaObject::invokeMethod(proc,"raw_command",Q_ARG(int,device_id),Q_ARG(int,command_id),Q_ARG(QByteArray,payload));
}

void tool_flash::on_disconnect_btn_clicked() {
  run_command("disconnect_bus");
}

void tool_flash::on_testinterface_btn_clicked() {
  if(test_interface_conditions.isEmpty() == false) {
    QMessageBox m;
    m.setText("Please ensure the following test conditions are met before continuing:");
    m.setInformativeText(test_interface_conditions);
    m.setWindowTitle("Interface testing conditions");
    m.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    int ret = m.exec();
    if(ret == QMessageBox::No) return;
  }
  run_command("interface_test");
}

void tool_flash::on_scanidle_btn_clicked() {
  proc->cancel_operation = false;
  QMetaObject::invokeMethod(proc,"scan_idle_traffic");
}

void tool_flash::on_restore_defaults_btn_clicked() {

}
