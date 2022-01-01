#ifndef DIALOG_INTERFACE_SELECT_H
#define DIALOG_INTERFACE_SELECT_H

#include <QDialog>

namespace Ui {
class dialog_interface_select;
}

class QListWidgetItem;

class dialog_interface_select : public QDialog {
  Q_OBJECT
public:

  enum _interface_type {
    INTERFACE_SERIAL
  };

  explicit dialog_interface_select(_interface_type t, QWidget *parent = nullptr);
  ~dialog_interface_select();
  QString selected_interface;

  static QString serial_interface_info(QString interface_id);
  static QStringList serial_interface_list();

  static QString first_ftdi_interface();

protected:
  static bool ftdi_counterfeit_check(QString serialnumber);

private slots:
  void on_ok_btn_clicked();
  void on_cancel_btn_clicked();
  void on_interface_list_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);

private:
  Ui::dialog_interface_select *ui;

  void populate_list_serial();
  int get_int(QString name, int default_setting);
  void set_int(QString name, int value);
};

#endif // DIALOG_INTERFACE_SELECT_H
