#ifndef PROCESSOR_CCM_H
#define PROCESSOR_CCM_H

#include <QObject>
#include "processor_8192dualflash.h"
#include "processor.h"

class processor_ccm : public processor_8192dualflash {
  Q_OBJECT
public:
  processor_ccm();
  ~processor_ccm();
  bool has_capability(QString capability_name) override;
  QString get_info() override;
  QString get_name() override;
  QList<parameter_def> get_parameter_list() override;

  bool load_bin(QString path_to_bin) override;
  bool save_bin(QString path_to_bin) override;

  bool flash_write() override;
  bool flash_read() override;

public slots:
  void configure() override;
  bool reset_ccm();

  void dbg_a() override;
protected:

  QString device_name(int device) override;
  QByteArray m2_read(unsigned int offset, unsigned int eeprom_size);
private:

  binfile bin_data;

  bool verify_unlock();
};

#endif // PROCESSOR_EE_H
