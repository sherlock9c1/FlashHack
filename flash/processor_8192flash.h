#ifndef PROCESSOR_8192FLASH_H
#define PROCESSOR_8192FLASH_H

// This parent class is for mid 90s era ECMs that support mode 5 and mode 6 reprogramming
// (we upload code segments and run them to perform reflashes etc).

#include "processor.h"

#include "../binfile.h"
#include "../binfile_ee.h"

#include <QMap>

class processor_8192flash : public processor {
  Q_OBJECT

public:
  enum _comm_protocol_base {
    COMM_PROGRAM = 0x05, // command to enter programming mode and accept mode 6 commands
    COMM_EXECUTE = 0x06, // command to upload and execute code in ram
    COMM_READ64 = 0x02  // read 64 byte chunk (mode 2)
  };

  processor_8192flash();

protected:

  bool read_device_m2(int device_id, unsigned int read_size, QString file_out);

  // this should point to a chunk of free memory we can use to execute normal instructions
  // when the kernel is running.
  quint16 config_default_exec_location = 0x0200;
};

#endif // PROCESSOR_8192FLASH_H
