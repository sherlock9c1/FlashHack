#include "log_widget.h"
#include "ui_log_widget.h"
#include "config.h"

#define NOTIFY_DURATION 3000

log_widget::log_widget(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::log_widget) {
  ui->setupUi(this);

  // timestamps
  show_timestamps = ui->timestamp_box->isChecked();
  timestamp_time.start();

  print_header();

  // default log level
  change_level(LOGLEVEL_STATUS);

  // configure annoying red error bar thing
  error_notify_timer.setSingleShot(true);
  error_notify_timer.setInterval(NOTIFY_DURATION);
  connect(&error_notify_timer,SIGNAL(timeout()),this,SLOT(notify_off()));
  notify_off(); // default to no error displayed, obviously.
}

log_widget::~log_widget() {
  delete ui;
}

void log_widget::refresh() {
  QString out;
  for(int x=0;x<log.size();x++) {
    if(log.at(x).level <= display_level) out.append(get_entry(x) + '\n');
  }
  out.chop(1); // remove trailing line feed
  ui->log_output->setPlainText(out);
  scroll_to_bottom();
}

QString log_widget::get_entry(int i) {
  QString out;
  if(show_timestamps == true) out.append(QString::number((float)log.at(i).time / 1000.00,'f',3) + ": ");
  out.append(level_string(log.at(i).level));
  out.append(log.at(i).text);
  return out;
}

void log_widget::scroll_to_bottom() {
  QTextCursor c = ui->log_output->textCursor();
  c.movePosition(QTextCursor::End);
  ui->log_output->setTextCursor(c);
}

void log_widget::append_entry(int i) {
  ui->log_output->appendPlainText(get_entry(i));
}

QString log_widget::level_string(int level) {
  switch(level) {
  case LOGLEVEL_DEBUG:
    return QStringLiteral("DEBUG::");
  case LOGLEVEL_COMM:
    return QStringLiteral("COMM::");
  case LOGLEVEL_ERROR:
    return QStringLiteral("ERROR! ");
  default:
    return QStringLiteral("");
  }
}

void log_widget::change_level(int level) {
  if(level == display_level) return;
  display_level = level;
  refresh();
}

void log_widget::on_err_btn_toggled(bool checked) {
  if(checked == true) change_level(LOGLEVEL_ERROR);
}

void log_widget::on_status_btn_toggled(bool checked) {
  if(checked == true) change_level(LOGLEVEL_STATUS);
}

void log_widget::on_debug_btn_toggled(bool checked) {
  if(checked == true) change_level(LOGLEVEL_DEBUG);
}

void log_widget::on_comms_btn_toggled(bool checked) {
  if(checked == true) change_level(LOGLEVEL_COMM);
}

void log_widget::add_entry(const QString &message, int loglevel) {
  log_entry l;
  l.text = message;
  l.level = loglevel;
  l.time = timestamp_time.elapsed();
  log.append(l);
  if(loglevel <= display_level) {
    append_entry(log.size() - 1);
    if(ui->scroll_btn->isChecked() == true) scroll_to_bottom();
  }

  if(loglevel == LOGLEVEL_ERROR) notify_error(message);
}

void log_widget::notify_error(const QString &error_string) {
  QApplication::alert(this); // flash the taskbar or whatever
  error_notify_timer.start(); // make sure the error expires
  ui->error_display->setText(error_string);
  ui->error_display->setHidden(false);
}

void log_widget::notify_off() {
  ui->error_display->setHidden(true);
}


void log_widget::on_timestamp_box_toggled(bool checked) {
  show_timestamps = checked;
  refresh();
}

void log_widget::on_clear_btn_clicked() {
  timestamp_time.restart();
  log.clear();
  print_header();
  refresh();
}

void log_widget::print_header() {
  add_entry("flashhack Version " + QString(FLASHHACK_VERSION),LOGLEVEL_STATUS);
}

void log_widget::on_scroll_btn_toggled(bool checked) {
  if(checked == true) scroll_to_bottom();
}
