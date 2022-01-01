#ifndef PROCESSOR_DUALP_H
#define PROCESSOR_DUALP_H

// This parent class is for 1993-1995 dual board ECMs like 8051-EE and the P66 V6.
// Each of the two boards in these ecms contains one main programmable eeprom,
// and one 68hc11 main processor.

// This requires a modified kernel that sends [$EE] [KERNELVERSION] when mode 0x0D is used.
// It's important to be able to detect a running kernel for recovery.

#include "processor.h"

#include "../binfile.h"
#include "../binfile_ee.h"

#include <QMap>

class processor_dualp : public processor {
  Q_OBJECT
public:
  processor_dualp();
  ~processor_dualp();

  // enumerate all of our bytecode so things are more readable.
  enum _comm_protocol {
    TSIDE = 0xF4, // the device specifier for one half of the ecm
    ESIDE = 0xE4, // .. and the other half
    CCM = 0xF1,
    COMM_PROGRAM = 0x05, // command to enter programming mode and accept mode 6 commands
    COMM_EXECUTE = 0x06, // command to upload and execute code in ram
    COMM_UNLOCK = 0x0D,  // command to unlock mode 5
      SUBCOMM_GET_CHALLENGE = 0x01, // unlock command 'challenge'
      SUBCOMM_SEND_RESPONSE = 0x02, // unlock command 'response'
    COMM_KERNEL = 0xEE,    // a special response from the kernel to mode 5.
    COMM_SUCCESS = 0xAA,   // usually means command success
    COMM_WRITE_FAIL = 0x08,        // write routine hw fail
    COMM_ERASE_IN_PROGRESS = 0x55, // erase ping
    COMM_ERASE_FAILURE = 0x06,     // erase fail
    COMM_ERASE_SUCCESS = 0xAA,     // erase success
    COMM_VPP_IGNVOLTS_RANGE_ERROR = 0x02, // response from vpp routine for ignvolts range
    COMM_VPP_VPPVOLTS_RANGE_ERROR = 0x03, // for vpp voltage range
    COMM_CONFIGURE = 0x0C, // configuration eeprom
      SUBCOMM_SET_CALID = 0x02, // set calibration id
      SUBCOMM_SET_VIN = 0x01,   // set vin
    COMM_READ64 = 0x02  // read 64 byte chunk (mode 2)
  };

public slots:

  datastream_reply execute_program(int device_id, QString program_name) override;
  bool execute_simple_program(int device_id, QString program_name);

  bool set_vin(QByteArray vin) override;
  bool set_calid(QVariant calid) override;

  // this thing is for rom dumping devices that support mode 2 but we don't have a working
  // kernel for (yet)...
  bool read_device_m2(int device_id, unsigned int read_size, QString file_out);

protected:

  // configuration - must set per subprocessor.  be sure to set all of them properly....
  //----------------------------------------------------
  // this is the location to store the flash write data.
  quint16 config_write_storage_address;

  // this is the program to execute after the data is placed.
  quint16 config_write_subroutine_address;

  // where to store the read routine
  quint16 config_read_upload_location;

  // the location of the subroutine for read instructions
  quint16 config_read_subroutine_address;

  // which device contains the circuity to apply VPP
  quint16 config_vpp_device_id;

  // this should point to a chunk of free memory we can use to execute normal instructions
  // when the kernel is running.
  quint16 config_default_exec_location = 0x0200;

  //----------------------------------------------------

  virtual binfile *bin_for_device(int device_id);

  QString device_name(int device) override;

  bool unlock_security(unsigned char device);
  bool _unlock_security(unsigned char device);
  bool enter_programming_mode(unsigned char device);

  datastream_reply _execute_program(unsigned char device, const program_definition &def);
  datastream_reply upload_and_execute(unsigned char device, quint16 offset, QByteArray code, int timeout = -1);

  bool _load_kernel(unsigned char device_id);
  bool is_kernel_loaded(int device_id);
  bool _unload_kernel(int device_id);
  bool upload_subroutines(unsigned char device_id);

  bool exec_write_program(int device_id);
  bool exec_read_program(int device_id);
  bool erase_eeprom(int device_id);

  void ff_unused_regions(int device_id);
  bool set_ff_if_zero(int device_id, const region &r);

  bool apply_vpp();
  bool remove_vpp();

  QMap <int,int>kernel_version;

  bool config_write_eside = true;
  bool config_write_tside = true;

  bool flash_write_device(int device_id);
  bool _flash_write_device(int device_id);

  bool write_region(int device_id, const region &r);
  bool read_region(int device_id, const region &r);

  bool is_booted(int device_id);

  virtual quint16 get_checksum(int device_id, quint16 start_address, quint16 end_address);

  void update_block_progress(int map_size, int blocks_remaining);

  datastream_reply jump_to_subroutine(int device_id, const program_definition &def);

  bool flash_read_device(int device_id);

  virtual bool get_chip_id(int device_id);

  bool set_x_if_y(int device_id, const region &r, unsigned char x, unsigned char y);

private slots:

};

#endif
