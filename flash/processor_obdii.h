#ifndef PROCESSOR_OBDII_H
#define PROCESSOR_OBDII_H

#include <QObject>
#include "processor.h"

// this is the reflash processor for generic obd-ii devices.

class processor_obdii : public processor {
public:
  processor_obdii();
  QString get_info() override;
  QString get_name() override;

public slots:
  void configure() override;

  void dbg_a() override;
  void dbg_b() override;
  void dbg_c() override;
protected:
  void become_bus_master();
};

#endif // PROCESSOR_OBDII_H
