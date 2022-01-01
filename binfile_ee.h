#ifndef BINFILE_EE_H
#define BINFILE_EE_H

//---------------------------------------

// blue/gray plug is called the t-side
// red/black plug is called the e-side

//---------------------------------------

#define EE_ROM_SIZE 0x10000
#define EE_CHECKSUM_LOCATION 0x2015
#define EE_CHECKSUM_START 0x2018
#define EE_RAM_SIZE 0x2000

#include "binfile.h"

class bin_ee : public binfile {
public:
  bin_ee();
  bin_ee(const QByteArray &data);

  void reset_checksum() override;
  unsigned int generate_checksum() override;
  unsigned int current_checksum() override;
  void clear_unused_regions() override;
  bool verify_pointers();

  static QByteArray convert_raw(QByteArray in);
  void from_raw(QByteArray in);
  QByteArray to_raw();

private:
  static unsigned int unscramble_ee_address(unsigned int a);
};

class binfile_ee {
public:
  enum ee_format {
    FORMAT_TUNERCAT,   // the ordinary bin format (also used by all known tunerpro XDFs and eehack)
    FORMAT_LT1EDIT,    // the backwards lt1edit format
    FORMAT_ESIDE,      // standard format (address lines not scrambled)
    FORMAT_TSIDE,      // standard format (address lines not scrambled)
    FORMAT_RAW_ESIDE,  // raw as you would read/write from the chip with an eeprom programmer (lines scrambled)
    FORMAT_RAW_TSIDE   // raw as you would read/write from the chip with an eeprom programmer (lines scrambled)
  };

  binfile_ee();
  binfile_ee(QByteArray data_in, ee_format ftype = FORMAT_TUNERCAT);

  bool is_valid();    // check if both sides are valid.

  // these functions do not return errors on invalid checksums.  use is_valid() after.
  bool load_file(QString path, ee_format ftype = FORMAT_TUNERCAT);
  bool save_file(QString path, ee_format ftype = FORMAT_TUNERCAT);
  bool load_data(QByteArray data, ee_format ftype = FORMAT_TUNERCAT);
  QByteArray save_data(ee_format ftype = FORMAT_TUNERCAT);

  // the actual data
  bin_ee eside;
  bin_ee tside;

  // features
  bool is_maf_enabled();
  bool is_auto_transmission();
  int get_cal_id();
  int get_prom_id_tside();
  int get_prom_id_eside();
  QString get_vin();
  QString bin_info();
  char bool_char(bool b);
  bool is_rev_b();
  bool is_rev_a();
};

#endif // BINFILE_EE_H
