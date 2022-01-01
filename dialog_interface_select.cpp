#include "dialog_interface_select.h"
#include "ui_dialog_interface_select.h"
#include <QSerialPortInfo>
#include <QDebug>
#include <QSettings>

dialog_interface_select::dialog_interface_select(_interface_type t, QWidget *parent) :
  QDialog(parent), ui(new Ui::dialog_interface_select) {
  ui->setupUi(this);

  switch(t) {
  case INTERFACE_SERIAL:
    populate_list_serial();
    break;

  }
}

void dialog_interface_select::populate_list_serial() {
  QStringList if_list = serial_interface_list();
  while(if_list.isEmpty() == false) {
    QString ifname = if_list.takeFirst();
    QListWidgetItem *i = new QListWidgetItem;
    i->setText(ifname);
    i->setData(Qt::UserRole,serial_interface_info(ifname));
    ui->interface_list->addItem(i);
  }
}

QStringList dialog_interface_select::serial_interface_list() {
  QStringList out;
  QSerialPortInfo i;
  QList <QSerialPortInfo>ports = i.availablePorts();
  for(int x=0;x<ports.size();x++) out.append(ports.at(x).portName());
  return out;
}

QString dialog_interface_select::first_ftdi_interface() {
  QSerialPortInfo i;
  QList <QSerialPortInfo>ports = i.availablePorts();
  for(int x=0;x<ports.size();x++) {
    if(ports.at(x).manufacturer() == "FTDI") {
      return ports.at(x).portName();
    }
  }
  return QString();
}

QString dialog_interface_select::serial_interface_info(QString interface_id) {
  QSerialPortInfo info(interface_id);
  QString out;
  out.append(info.description() + " - " + info.manufacturer() + "\n" + "Serial# " + info.serialNumber() +
             "\nVID/PID: "  + QString::number(info.vendorIdentifier(),16) + "/" + QString::number(info.productIdentifier(),16));
  if(ftdi_counterfeit_check(info.serialNumber()) == true) out.append("\nWARNING! Your FTDI interface is probably counterfeit.  This may not be a real problem, but if you have issues, you may have to try another.");
  return out;
}

bool dialog_interface_select::ftdi_counterfeit_check(QString serialnumber) {
  if(serialnumber.startsWith("A5XK3RJ")) return true;
  if(serialnumber.startsWith("A50285BI")) return true;
  if(serialnumber.startsWith("FTB6SPL3")) return true;
  if(serialnumber.startsWith("0000")) return true;
  return false;
}

dialog_interface_select::~dialog_interface_select() {
  delete ui;
}

void dialog_interface_select::on_ok_btn_clicked() {
  if(ui->interface_list->selectedItems().size() != 1) {
    reject();
    return;
  }
  selected_interface = ui->interface_list->selectedItems().at(0)->text();
  accept();
}

void dialog_interface_select::on_cancel_btn_clicked() {
  reject();
}

void dialog_interface_select::on_interface_list_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous) {
  Q_UNUSED(previous);
  ui->interface_info->setPlainText(current->data(Qt::UserRole).toString());
}

