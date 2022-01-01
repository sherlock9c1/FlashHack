#ifndef LOG_WIDGET_H
#define LOG_WIDGET_H

#include <QWidget>
#include <QDateTime>
#include "enums.h"
#include <QElapsedTimer>
#include <QTimer>

namespace Ui {
class log_widget;
}

class log_entry {
public:
  QString text;
  int time;
  int level;
};

class log_widget : public QWidget {
  Q_OBJECT

public:
  explicit log_widget(QWidget *parent = nullptr);
  ~log_widget();

protected:
  void scroll_to_bottom();

public slots:
  void add_entry(const QString &message, int loglevel);
  void notify_off();

private slots:
  void on_err_btn_toggled(bool checked);
  void on_status_btn_toggled(bool checked);
  void on_debug_btn_toggled(bool checked);
  void on_comms_btn_toggled(bool checked);

  void change_level(int level);
  void refresh();

  void on_timestamp_box_toggled(bool checked);
  void on_clear_btn_clicked();
  void on_scroll_btn_toggled(bool checked);

  void notify_error(const QString &error_string);

private:
  Ui::log_widget *ui;
  int display_level;

  QVector <log_entry>log;
  void append_entry(int i);
  bool show_timestamps = true;
  QString level_string(int level);
  QString get_entry(int i);
  QStringList get_entries(int loglevel, int max);

  QElapsedTimer timestamp_time;
  QTimer error_notify_timer;

  void print_header();
};

#endif // LOG_WIDGET_H
