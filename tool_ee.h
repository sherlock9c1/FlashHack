#ifndef TOOL_EE_H
#define TOOL_EE_H

#include <QWidget>
#include "binfile_ee.h"
#include "datastream/datastream_8192.h"
#include "flash/processor_8192dualflash.h"

namespace Ui {
  class tool_ee;
}

class tool_ee : public QWidget {
  Q_OBJECT

public:
  explicit tool_ee(QWidget *parent = nullptr);
  ~tool_ee();
  binfile_ee::ee_format selected_format();

private slots:
  void on_checksum_btn_clicked();
  void on_load_btn_clicked();
  void on_save_btn_clicked();
  void on_clear_btn_clicked();

private:
  Ui::tool_ee *ui;
  binfile_ee bin;
  void update_status();

  void error(QString err_msg);

};

#endif // TOOL_EE_H
