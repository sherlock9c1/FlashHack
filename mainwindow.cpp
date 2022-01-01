#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "binfile_ee.h"
#include "tool_ee.h"
#include "tool_flash.h"
#include "dialog_interface_select.h"
#include "flash/processor_ee.h"
#include "flash/processor_p66.h"
#include "flash/processor_obdii.h"
#include "flash/processor_8192read.h"
#include "flash/processor_ccm.h"
#include "datastream/datastream_obdxpro.h"

#include <QDebug>
#include <QSettings>
#include <QStyleFactory>
#include <QUrl>
#include <QDesktopServices>
#include "settings_dialog.h"
#include "config.h"

#include <QCloseEvent>

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent)
  , ui(new Ui::MainWindow) {

  QCoreApplication::setOrganizationName("fbodytech");
  QCoreApplication::setOrganizationDomain("fbodytech.com");
  QCoreApplication::setApplicationName("flashhack");

  set_ui_style(false);

  ui->setupUi(this);

  // set version .
  QString text_in = ui->title_text->text();
  text_in.replace("[VERSION]",FLASHHACK_VERSION);
  ui->title_text->setText(text_in);

  //-----------------------------------------------------------------------
  // set up our processor list here.  when adding a processor you must also ammend the run_processor() switch statement

  add_processor(PROCESSOR_EE_FLASH,INTERFACE_ALDL,"EE Flash",
                "This is a flash read/write processor for the ECMs used on 1994-1995 LT1 engines.\n"
                "Works with 16188051 and 16181333",false);

  add_processor(PROCESSOR_P66_FLASH,INTERFACE_ALDL,"P66 Flash",
                "This is a flash read/write processor for P66 V6 ECMs used on 3100 and 3.4L engines from 1993-1995.\n"
                "Should work with 16172693, 16184164, 16184737, 16196397",false);

  add_processor(PROCESSOR_8192READ,INTERFACE_ALDL,"ALDL Universal Read",
                "A universal BIN reader that works for many devices, "
                "this tool will dump any mapped memory (including the main ROM) of any ALDL device that supports the Mode 2 command.",false);

  add_processor(PROCESSOR_CCM,INTERFACE_ALDL,"CCM EEPROM",
                "Reprograms the 1994-1995 Corvette CCM's internal EEPROM.\n"
                "Should work wtih 16157364, 16212971, 16230561, 16230686, 16223622.",false);

  add_processor(PROCESSOR_OBDIITESTING,INTERFACE_VPW,"OBD-II Development",
                "This doesn't do anything yet unless you're helping me test interfaces.",true);

  //-----------------------------------------------------------------------

  // run the last processor, default to none
  QSettings s;
  run_processor(s.value("LAST_PROCESSOR",PROCESSOR_NONE).toInt());
}

void MainWindow::closeEvent(QCloseEvent *event) {
  event->accept();
}

void MainWindow::remember_last_processor(int processor_id) {
  QSettings s;
  s.setValue("LAST_PROCESSOR",processor_id);
}

void MainWindow::add_processor(int processor_id, int interface_group, QString processor_name, QString processor_detail, bool advanced) {
  QListWidgetItem *i = new QListWidgetItem;
  i->setText(processor_name);
  i->setTextAlignment(Qt::AlignCenter);
  i->setToolTip(processor_detail);
  i->setData(Qt::UserRole,processor_id);
  i->setData(Qt::UserRole + 1,interface_group);
  i->setData(Qt::UserRole + 2,advanced);
  ui->processor_list->addItem(i);
  if(ui->advanced_btn->isChecked() == false) i->setHidden(advanced);
}

void MainWindow::on_processor_list_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous) {
  Q_UNUSED(previous);
  if(current == nullptr) {
    ui->processor_info->clear();
  } else {
    ui->processor_info->setPlainText(current->toolTip());
  }
}

void MainWindow::on_processor_list_itemDoubleClicked(QListWidgetItem *item) {
  run_processor(item->data(Qt::UserRole).toInt());
}

void MainWindow::run_processor(int processor_id) {
  if(processor_id == PROCESSOR_NONE) return;

  remember_last_processor(processor_id);

  processor *p = nullptr;
  datastream_interface *i = nullptr;

  //-------------------------------------------------
  // ADD NEW PROCESSORS AND THEIR INTERFACES HERE ...
  switch(processor_id) {
  case PROCESSOR_EE_FLASH:
    p = new processor_ee();
    i = new datastream_8192();
    break;
  case PROCESSOR_P66_FLASH:
    p = new processor_p66();
    i = new datastream_8192();
    break;
  case PROCESSOR_OBDIITESTING:
    p = new processor_obdii();
    i = new datastream_obdxpro();
    break;
  case PROCESSOR_8192READ:
    p = new processor_8192read();
    i = new datastream_8192();
    break;
  case PROCESSOR_CCM:
    p = new processor_ccm();
    i = new datastream_8192();
    break;
  }
  //-------------------------------------------------

  start_flash_tool(p,i);
}

void MainWindow::start_flash_tool(processor *p, datastream_interface *i) {
  if(p == nullptr || i == nullptr) return;
  if(flash != nullptr) quit_flash_tool();
  flash = new tool_flash(p,i);
  ui->main_switcher->addWidget(flash);
  ui->main_switcher->setCurrentWidget(flash);
}

void MainWindow::quit_flash_tool() {
  if(flash == nullptr) return;
  flash->quit();
  flash = nullptr;
  ui->main_switcher->setCurrentIndex(0);
}

void MainWindow::set_ui_style(bool dark) {
  qApp->setStyle(QStyleFactory::create("fusion"));
  static QPalette light_palette;
  QPalette dark_palette;
  // stolen from the internets somewhere ..?
  if(dark == true) {
    dark_palette.setColor(QPalette::Window, QColor(53,53,53));
    dark_palette.setColor(QPalette::WindowText, Qt::white);
    dark_palette.setColor(QPalette::Base, QColor(15,15,15));
    dark_palette.setColor(QPalette::AlternateBase, QColor(53,53,53));
    dark_palette.setColor(QPalette::ToolTipBase, Qt::white);
    dark_palette.setColor(QPalette::ToolTipText, Qt::white);
    dark_palette.setColor(QPalette::Text, Qt::white);
    dark_palette.setColor(QPalette::Button, QColor(53,53,53));
    dark_palette.setColor(QPalette::ButtonText, Qt::white);
    dark_palette.setColor(QPalette::BrightText, Qt::red);
    dark_palette.setColor(QPalette::Highlight, QColor(43,90,197).lighter());
    dark_palette.setColor(QPalette::HighlightedText, Qt::black);
    dark_palette.setColor(QPalette::Disabled, QPalette::Text, Qt::darkGray);
    dark_palette.setColor(QPalette::Disabled, QPalette::ButtonText, Qt::darkGray);
    qApp->setPalette(dark_palette);
  } else {
    qApp->setPalette(light_palette);
  }
}

MainWindow::~MainWindow() {
  delete flash;
  delete ui;
}

void MainWindow::run_interface_selector_serial() {
  dialog_interface_select d(dialog_interface_select::INTERFACE_SERIAL);
  if(d.exec() == dialog_interface_select::Accepted &&
    d.selected_interface.isEmpty() == false) {
    QSettings s;
    s.setValue("SERIAL_PORT",d.selected_interface);
  }
}

void MainWindow::on_actionSelect_Serial_Port_triggered() {
   run_interface_selector_serial();
}

void MainWindow::on_actionEE_Bin_Converter_triggered() {
   if(eetool == nullptr) eetool = new tool_ee();
   eetool->show();
}

void MainWindow::on_actionWritten_by_Steve_Haslin_resfilter_resfilter_net_triggered() {
  launch_url("mailto:resfilter@resfilter.net");
}

void MainWindow::on_actionfbodytech_com_triggered() {
  launch_url("http://fbodytech.com/");
}

void MainWindow::on_actiongearhead_efi_org_triggered() {
  launch_url("http://www.gearhead-efi.com/");
}

void MainWindow::on_actionDonate_Beer_Money_triggered() {
  launch_url("https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=resfilter%40resfilter.net&lc=CA&item_name=fbodytech.com&currency_code=USD&bn=PP-DonationsBF%3Abtn_donateCC_LG.gif%3ANonHosted");
}

void MainWindow::launch_url(QString url) {
  QDesktopServices::openUrl(QUrl(url));
}

void MainWindow::on_actionConfiguration_triggered() {
   settings_dialog d;
   d.exec();
}

void MainWindow::on_actionChange_ECM_Type_triggered() {
  remember_last_processor(PROCESSOR_NONE); // invalidate processor setting
  quit_flash_tool();
}

void MainWindow::on_start_tool_btn_clicked() {
  if(ui->processor_list->selectedItems().isEmpty()) return;
  int id = ui->processor_list->selectedItems().first()->data(Qt::UserRole).toInt();
  run_processor(id);
}

void MainWindow::on_advanced_btn_toggled(bool checked) {
  for(int x=0;x<ui->processor_list->count();x++) {
    bool advanced = ui->processor_list->item(x)->data(Qt::UserRole + 2).toBool();
    if(checked == false && advanced == true) {
      ui->processor_list->item(x)->setHidden(true);
    } else {
      ui->processor_list->item(x)->setHidden(false);
    }
  }
}
