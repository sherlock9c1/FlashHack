#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <QDialog>

namespace Ui {
class settings_dialog;
}

class settings_dialog : public QDialog {
  Q_OBJECT

public:
  explicit settings_dialog(QWidget *parent = nullptr);
  ~settings_dialog();

private slots:
  void on_aldl_fastconnect_toggled(bool checked);
  void on_aldl_idle_traffic_toggled(bool checked);

  void on_aldl_slowcomm_toggled(bool checked);

  void on_aldl_disconnect_properly_toggled(bool checked);

private:
  Ui::settings_dialog *ui;
  int get_int(QString name, int default_setting);
  void set_int(QString name, int value);
  void set_switch(QString name, bool value);
  bool get_switch(QString name, bool default_setting);
};

#endif // SETTINGS_DIALOG_H
