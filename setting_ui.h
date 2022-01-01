#ifndef SETTING_UI_H
#define SETTING_UI_H

#include <QWidget>
#include <QVariant>
#include <QListWidgetItem>

namespace Ui {
class setting_ui;
}

class parameter_def {
public:
  enum _parameter_type {
    SWITCH,
    INT
  };

  parameter_def(QString setting_name, QString friendly_name, QString help, _parameter_type parameter_type, QVariant default_setting, bool advanced = false, int min_value = 0, int max_value = 255);
  parameter_def();

  QString setting_name;
  QString friendly_name;
  QString help;
  _parameter_type t;
  QVariant default_setting;
  bool advanced;
  int min;
  int max;
};

class setting_ui : public QWidget {
  Q_OBJECT

public:
  explicit setting_ui(QWidget *parent = nullptr);
  ~setting_ui();

  void clear();
  void add_parameter(const parameter_def &d);

private slots:
  void on_setting_list_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);
  void on_setting_list_itemChanged(QListWidgetItem *item);
  void on_hex_btn_toggled(bool checked);
  void on_value_entry_valueChanged(int arg1);

private:
  Ui::setting_ui *ui;

  QVector <parameter_def>defs;
  void configure_entry_box(const parameter_def &d);
  parameter_def get_item_def(QListWidgetItem *i);

  QListWidgetItem *currently_selected_item = nullptr;

  bool block_update = false;
};

#endif // SETTING_UI_H
