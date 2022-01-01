#ifndef PROCESSOR_EE_H
#define PROCESSOR_EE_H

#include <QObject>
#include "processor_8192dualflash.h"
#include "processor.h"

class processor_ee : public processor_8192dualflash {
  Q_OBJECT
public:
  processor_ee();
  ~processor_ee();
  bool has_capability(QString capability_name) override;
  QString get_info() override;
  QString get_name() override;
  QList<parameter_def> get_parameter_list() override;

public slots:
  void configure() override;
  bool flash_write() override;
  bool flash_read() override;
  bool load_bin(QString path_to_bin) override;
  bool save_bin(QString path_to_bin) override;
  bool load_kernel() override;
  bool unload_kernel() override;
  void dbg_a() override;
  void dbg_b() override;
  void dbg_c() override;

protected:
  bool store_local_bin(int device_id, QString name, const bin_ee &bin_in);
  bin_ee get_local_bin(int device_id, QString name);
  void store_previously_completed_bin(int device_id);
  void store_invalid_previous_bin(int device_id);
  bool has_bin_changed(int device_id);
  void ff_unused_regions(int device_id, bool reverse = false);
  bool check_selective_write();
  bool build_read_map(int device_id);
  bool build_read_map();
  bool build_write_map(int device_id);
  bool install_recovery_rom(int device_id);
  bool verify_write_checksum(int device_id);
  void set_max_block_size(int size);
  bool write_region(int device_id, const region &r);
  binfile *bin_for_device(int device_id) override;
  bool get_chip_id(int device_id) override;
  bool erase_flash(int device_id);
  quint16 get_checksum(int device_id, quint16 start_address, quint16 end_address) override;
  QByteArray get_vin();
  quint32 vehicle_id;

private:
  binfile_ee *bin;
  bool write_onboard_eeprom(int device_id);
  bool patch_full_eeprom_write(bool enable);
  bool program_eeprom_block(int device_id, region r, QByteArray data);
  quint32 set_vehicle_id();
  quint32 get_vehicle_id();
  bool tside_eeprom_data_valid();
};

#endif // PROCESSOR_EE_H
