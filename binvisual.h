#ifndef BINVISUAL_H
#define BINVISUAL_H

#include <QWidget>
#include <QPaintEvent>
#include "binfile.h"
#include "enums.h"

class binvisual : public QWidget {
  Q_OBJECT
public:
  explicit binvisual(QWidget *parent = nullptr);
  void paintEvent(QPaintEvent *event) override;

  QSize sizeHint() const override;
  QSize minimumSizeHint() const override;

  void clear();
  int bin_size();

  static QColor region_state_color(const _program_region_state region_state);

public slots:
  void set_region_state(int device_id, int offset, int size, int state);
  void start_new_bin(int device_id, unsigned int s, const QString &bin_name); // this does clear the thing.

signals:

private:
  QMap <int,QVector <_program_region_state>>map;
  QMap <int,QString>device_name;
  int device_id;
};

#endif // BINVISUAL_H
