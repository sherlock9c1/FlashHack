#include "processor_8192flash.h"
#include <QFile>

processor_8192flash::processor_8192flash()
{

}

bool processor_8192flash::read_device_m2(int device_id, unsigned int read_size, QString file_out) {
  QByteArray out;

  start("Mode 2 device read");

  statusmsg("Starting memory map dump for device " + device_name(device_id));

  emit start_program_operation(device_id,device_name(device_id),"READ",read_size);
  emit region_changed(device_id,0,read_size,REGION_UNKNOWN);

  for(unsigned int x=0;x<read_size;x+=64) {
    emit region_changed(device_id,x,64,REGION_PROGRAMMING);
    datastream_request req(device_id,COMM_READ64);
    req.set16(0,x); // append position
    datastream_reply repl = interface->request(req);
    if(repl.success == true) {
      emit region_changed(device_id,x,64,REGION_READ);
      out.append(repl.get_data());
    } else {
      emit region_changed(device_id,x,64,REGION_ERROR);
      errmsg("Error reading mode2 segment " + QString::number(x,16).toUpper());
      fail("Failed to dump device ROM.");
      return false;
    }
    if(is_cancelled() == true) {
      fail("Failed to dump device ROM, operation cancelled.");
      return false;
    }
  }

  out.resize(read_size); // kill the end if it's too big .

  QFile f(file_out);
  if(f.open( QIODevice::WriteOnly ) == false) {
    errmsg("Could not open bin " + file_out + " for writing.");
    fail("Failed to dump device ROM.");
    return false;
  }

  f.write(out);
  statusmsg("Saved bin " + file_out);
  success("ROM dump success.");
  return true;
}
