#include "setting_ui.h"
#include "ui_setting_ui.h"
#include <QSettings>
#include <QDebug>

setting_ui::setting_ui(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::setting_ui) {
  ui->setupUi(this);
}

setting_ui::~setting_ui() {
  delete ui;
}

void setting_ui::clear() {
  ui->setting_list->clear();
  ui->setting_help->clear();
}

void setting_ui::add_parameter(const parameter_def &d) {
  defs.append(d);
  int new_def_index = defs.size() - 1;

  QSettings s;
  QListWidgetItem *i = new QListWidgetItem;

  i->setText(d.friendly_name);
  i->setData(Qt::UserRole,new_def_index);

  if(d.t == parameter_def::SWITCH) {
    i->setFlags(i->flags() | Qt::ItemIsUserCheckable);
    bool checked = s.value(d.setting_name,d.default_setting).toBool();
    // save the setting immediately so nobody has to worry about defaults.
    s.setValue(d.setting_name,checked);
    if(checked == true) {
      i->setCheckState(Qt::Checked);
    } else {
      i->setCheckState(Qt::Unchecked);
    }
  } else {
    // save the setting immediately so nobody has to worry about defaults.
    s.setValue(d.setting_name,s.value(d.setting_name,d.default_setting));
  }

  i->setToolTip(d.setting_name + " DEFAULT=" + d.default_setting.toString());
  ui->setting_list->addItem(i);
}

void setting_ui::configure_entry_box(const parameter_def &d) {
  QSettings s;

  // disable value entry when not required.
  if(d.t == parameter_def::SWITCH) {
    ui->value_entry->clear();
    ui->value_entry->setEnabled(false);
    return;
  }

  // get integer base setting
  int intbase = 10;
  if(ui->hex_btn->isChecked()) intbase = 16;

  block_update = true;

  // configure box
  ui->value_entry->setEnabled(true);
  ui->value_entry->setDisplayIntegerBase(intbase);

  // min/max
  ui->value_entry->setRange(d.min,d.max);

  // set value
  int v = s.value(d.setting_name,d.default_setting).toInt();
  ui->value_entry->setValue(v);

  block_update = false;
}

parameter_def::parameter_def(QString setting_name, QString friendly_name, QString help, _parameter_type parameter_type, QVariant default_setting, bool advanced, int min_value, int max_value) {
  this->setting_name = setting_name;
  this->friendly_name = friendly_name;
  this->help = help;
  this->t = parameter_type;
  this->default_setting = default_setting;
  this->advanced = advanced;
  this->min = min_value;
  this->max = max_value;
}

parameter_def::parameter_def() {

}

parameter_def setting_ui::get_item_def(QListWidgetItem *i) {
  int x = i->data(Qt::UserRole).toInt();
  if(x > defs.size() - 1) {
    qDebug() << "ITEM DEF INDEX OUT OF RANGE";
    return parameter_def();
  }
  return defs.at(x);
}

void setting_ui::on_setting_list_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous) {
  Q_UNUSED(previous);

  currently_selected_item = current;
  if(current == nullptr) return;

  if(block_update == true) return;

  parameter_def d = get_item_def(current);
  ui->setting_help->setPlainText(d.help); // print item help

  configure_entry_box(d);
}

void setting_ui::on_setting_list_itemChanged(QListWidgetItem *item) { 
  parameter_def d = get_item_def(item);
  if(d.t != parameter_def::SWITCH) return;

  if(block_update == true) return;

  bool checked = false;
  if(item->checkState() == Qt::Checked) checked = true;

  QSettings s;
  s.setValue(d.setting_name,checked);
}

void setting_ui::on_hex_btn_toggled(bool checked) {
  Q_UNUSED(checked);

  if(currently_selected_item == nullptr) return;
  parameter_def d = get_item_def(currently_selected_item);
  if(d.t == parameter_def::SWITCH) return;

  configure_entry_box(d);
}

void setting_ui::on_value_entry_valueChanged(int arg1) {
  Q_UNUSED(arg1);
  if(currently_selected_item == nullptr) return;
  if(block_update == true) return;

  parameter_def d = get_item_def(currently_selected_item);
  if(d.t == parameter_def::SWITCH) return;

  QSettings s;
  s.setValue(d.setting_name,ui->value_entry->value());
}

