#include <QFileDialog>
#include <QMessageBox>
#include <QThread>
#include <QMetaObject>
#include <QDebug>

#include "datastream/datastream_8192.h"
#include "flash/processor_8192dualflash.h"

#include "tool_ee.h"
#include "ui_tool_ee.h"

#include "dialog_interface_select.h"

tool_ee::tool_ee(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::tool_ee) {
  ui->setupUi(this);
  update_status();
}

tool_ee::~tool_ee() {
  delete ui;
}

binfile_ee::ee_format tool_ee::selected_format() {
  if(ui->format_std_btn->isChecked()) return binfile_ee::FORMAT_TUNERCAT;
  if(ui->format_lt1edit_btn->isChecked()) return binfile_ee::FORMAT_LT1EDIT;
  if(ui->format_tside_btn->isChecked()) return binfile_ee::FORMAT_RAW_TSIDE;
  if(ui->format_eside_btn->isChecked()) return binfile_ee::FORMAT_RAW_ESIDE;
  if(ui->format_tside_btn_2->isChecked()) return binfile_ee::FORMAT_TSIDE;
  if(ui->format_eside_btn_2->isChecked()) return binfile_ee::FORMAT_ESIDE;
  return binfile_ee::FORMAT_TUNERCAT;
}

void tool_ee::update_status() {
  if(bin.tside.is_valid() == true) {
    ui->tside_status->setText("Valid.");
  } else {
    ui->tside_status->setText("Invalid.");
  }

  if(bin.eside.is_valid() == true) {
    ui->eside_status->setText("Valid.");
  } else {
    ui->eside_status->setText("Invalid.");
  }

  ui->bin_info->setPlainText(bin.bin_info());
}

void tool_ee::on_checksum_btn_clicked() {
   bin.tside.reset_checksum();
   bin.eside.reset_checksum();
   update_status();
}

void tool_ee::on_load_btn_clicked() {
  QString name = QFileDialog::getOpenFileName(nullptr,"Select source bin file...");
  if(name.isEmpty() == true) return;
  bool result = bin.load_file(name,selected_format());
  if(result == false) {
    error("The file could not be loaded.");
  }
  update_status();
}

void tool_ee::on_save_btn_clicked() {
  QString name = QFileDialog::getSaveFileName(nullptr,"Select destination bin file...");
  if(name.isEmpty() == true) return;
  bool result = bin.save_file(name,selected_format());
  if(result == false) {
    error("The file could not be saved.");
  }
  update_status();
}

void tool_ee::on_clear_btn_clicked() {
  bin.tside.clear();
  bin.eside.clear();
  update_status();
}

void tool_ee::error(QString err_msg) {
  QMessageBox m;
  m.setText(err_msg);
  m.exec();
}

