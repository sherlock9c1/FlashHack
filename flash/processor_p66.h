#ifndef PROCESSOR_P66_H
#define PROCESSOR_P66_H

#include <QObject>

#include "processor_8192dualflash.h"

class processor_p66 : public processor_8192dualflash {
public:
  processor_p66();
  bool has_capability(QString capability_name) override;
  QString get_info() override;
  QString get_name() override;
  QList<parameter_def> get_parameter_list() override;
public slots:
  void configure() override;
  bool flash_read() override;
  bool flash_write() override;
  bool load_bin(QString path_to_bin) override;
  bool save_bin(QString path_to_bin) override;
  bool load_kernel() override;
  bool unload_kernel() override;
  void dbg_a() override;
  void dbg_b() override;
  void dbg_c() override;
  bool get_chip_id(int device_id) override;
protected:
  binfile *bin_for_device(int device_id) override;
  void set_max_block_size(int size);
  bool build_write_map(int device_id);
  bool build_read_map();
  bool build_read_map(int device_id);
  bool verify_write_checksum(int device_id);
  quint16 get_checksum(int device_id, quint16 start_address, quint16 end_address) override;
private:
  binfile tside_bin;
  binfile eside_bin;
  bool check_selective_write();
  bool has_bin_changed(int device_id);
  bool store_local_bin(int device_id, QString name, const binfile &bin_in);
  binfile get_local_bin(int device_id, QString name);
  void store_previously_completed_bin(int device_id);
};

#endif // PROCESSOR_P66_H
