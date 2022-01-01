#include "settings_dialog.h"
#include "ui_settings_dialog.h"
#include <QSettings>

settings_dialog::settings_dialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::settings_dialog) {
  ui->setupUi(this);

  ui->aldl_fastconnect->setChecked(get_switch("ALDL_FAST_CONNECTION",false));
  ui->aldl_idle_traffic->setChecked(get_switch("ALDL_DISPLAY_IDLE_TRAFFIC",false));
  ui->aldl_slowcomm->setChecked(get_switch("ALDL_SLOW_MODE",false));
  ui->aldl_disconnect_properly->setChecked(get_switch("ALDL_DISCONNECT_PROPERLY",false));
}

void settings_dialog::on_aldl_fastconnect_toggled(bool checked) {
  set_switch("ALDL_FAST_CONNECTION",checked);
}

void settings_dialog::on_aldl_idle_traffic_toggled(bool checked) {
  set_switch("ALDL_DISPLAY_IDLE_TRAFFIC",checked);
}


settings_dialog::~settings_dialog() {
  delete ui;
}

int settings_dialog::get_int(QString name, int default_setting) {
  QSettings s;
  return s.value(name,default_setting).toInt();
}

void settings_dialog::set_int(QString name, int value) {
  QSettings s;
  s.setValue(name,value);
}

void settings_dialog::set_switch(QString name, bool value) {
  QSettings s;
  s.setValue(name,value);
}

bool settings_dialog::get_switch(QString name, bool default_setting) {
  QSettings s;
  return s.value(name,default_setting).toBool();
}

void settings_dialog::on_aldl_slowcomm_toggled(bool checked) {
  set_switch("ALDL_SLOW_MODE",checked);
}

void settings_dialog::on_aldl_disconnect_properly_toggled(bool checked) {
  set_switch("ALDL_DISCONNECT_PROPERLY",checked);
}
