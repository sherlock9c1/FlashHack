#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>

class tool_ee;
class tool_flash;
class processor;
class datastream_interface;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

  enum _processor_id {
    PROCESSOR_NONE,
    PROCESSOR_P66_FLASH,
    PROCESSOR_EE_FLASH,
    PROCESSOR_OBDIITESTING,
    PROCESSOR_8192READ,
    PROCESSOR_CCM
  };

  enum _interface_group {
    INTERFACE_ALDL,
    INTERFACE_VPW
  };

  void closeEvent(QCloseEvent *event) override;

public slots:
  void run_interface_selector_serial();
  void launch_url(QString url);
  void quit_flash_tool();

private slots:
  void on_actionSelect_Serial_Port_triggered();
  void on_actionEE_Bin_Converter_triggered();
  void on_actionWritten_by_Steve_Haslin_resfilter_resfilter_net_triggered();
  void on_actionfbodytech_com_triggered();
  void on_actiongearhead_efi_org_triggered();
  void on_actionDonate_Beer_Money_triggered();
  void on_actionConfiguration_triggered();
  void on_actionChange_ECM_Type_triggered();
  void remember_last_processor(int processor_id);
  void run_processor(int processor_id);
  void on_processor_list_itemDoubleClicked(QListWidgetItem *item);

  void on_processor_list_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);

  void on_start_tool_btn_clicked();

  void on_advanced_btn_toggled(bool checked);

private:
  Ui::MainWindow *ui;
  tool_ee *eetool = nullptr;
  tool_flash *flash = nullptr;
  void set_ui_style(bool dark);

  void start_flash_tool(processor *p, datastream_interface *i);

  void add_processor(int processor_id, int interface_group, QString processor_name, QString processor_detail, bool advanced);
};
#endif // MAINWINDOW_H
