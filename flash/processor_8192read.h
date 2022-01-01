#ifndef PROCESSOR_8192READ_H
#define PROCESSOR_8192READ_H

#include "processor.h"

class processor_8192read : public processor {
public:
  processor_8192read();
  void configure() override;
  QString get_info() override;
  QList<parameter_def> get_parameter_list() override;
  QString get_name() override;

public slots:
  bool save_bin(QString path_to_bin) override;
  bool has_capability(QString capability_name) override;
  bool flash_read() override;
private:
  bool read_segment(int device_id, const region &r);
  binfile bin;
};

#endif // PROCESSOR_8192READ_H
