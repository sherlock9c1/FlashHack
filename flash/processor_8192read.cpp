#include "processor_8192read.h"

#include <QFile>

processor_8192read::processor_8192read() {

}

QList<parameter_def> processor_8192read::get_parameter_list() {
  QList<parameter_def> out;

  out.append(parameter_def("M2_DEVICE_ID",
                           "Device ID",
                           "This is the hexidecimal ID of the device you want to dump.  Most ECMs are F4 (dec 244).",
                           parameter_def::INT,
                           0xF4,true,0,0xFF));

  out.append(parameter_def("M2_MEM_SIZE",
                           "Memory Size",
                           "This is the size of the addressable memory of the device.\n0x10000 would be a 64KB device.",
                           parameter_def::INT,
                           0x10000,true,1,0x10000));

  out.append(parameter_def("M2_MEM_READ_START",
                           "Read Start",
                           "This is the start of useful data, or the specific region that you want to read.",
                           parameter_def::INT,
                           0,true,0,0xFFFF));

  out.append(parameter_def("M2_MEM_READ_END",
                           "Read End",
                           "This is the end of useful data, or the specific region that you want to read.",
                           parameter_def::INT,
                           0xffff,true,1,0xffff));

  out.append(parameter_def("M2_M3",
                           "Use Mode 3 (Single byte)",
                           "This rediculous operation uses mode 3 instead of mode 2 to dump the entire region.\n"
                           "To be clear, this means we're reading one byte at a time.\n"
                           "It's really only for fun, but if you ever found a device that supported mode 3 and not mode 2, "
                           "this would be a way to very slowly dump that device's memory.",
                           parameter_def::SWITCH,
                           false));

  return out;
}

QString processor_8192read::get_name() {
  return "ALDL Universal Read";
}

void processor_8192read::configure() {
  statusmsg("Configured processor " + get_name() + " with interface " + interface->interface_name());
  statusmsg(get_info());
  interface->configure(); // allow the interface to configure itself now.
}

QString processor_8192read::get_info() {
  return "A universal BIN reader that works for many devices, "
         "this tool will dump any mapped memory (including the main ROM) of any ALDL device that supports the Mode 2 command.";
}

bool processor_8192read::has_capability(QString capability_name) {
  if(capability_name == "FLASH_READ") return true;
  return false;
}

bool processor_8192read::save_bin(QString path_to_bin) {
  QFile f(path_to_bin);
  if(f.open( QIODevice::WriteOnly ) == false) {
    errmsg("Could not open bin " + path_to_bin + " for writing.");
    return false;
  }

  f.write(bin.to_bytearray());
  statusmsg("Saved bin " + path_to_bin);
  return true;
}

bool processor_8192read::flash_read() {
  bool mode3 = get_setting("M2_M3").toBool();
  int device_id = get_setting("M2_DEVICE_ID").toInt();
  unsigned int read_size = get_setting("M2_MEM_SIZE").toInt();
  unsigned int read_start = get_setting("M2_MEM_READ_START").toInt();
  unsigned int read_end = get_setting("M2_MEM_READ_END").toInt();

  if(read_size < 1 || read_end <= read_start || read_start >= read_size || read_end > read_size) {
    fail("Some parameters are nonsensical.  Please check your settings in the advanced tab.");
    return false;
  }

  if(mode3 == true) {
    start ("Mode 3 Device Read");
    statusmsg("Warning: This is very slow, you might want to come back tomorrow.");
  } else {
    start("Mode 2 Device Read");
  }

  // prepare bin
  bin.resize(read_size);
  bin.clear();

  // build read map
  debugmsg("Building read map");
  bin.map.skip(0,read_start);
  bin.map.skip(read_end + 1,read_size - read_end);
  if(mode3 == true) {
    bin.map.set_max_block_size(1);
  } else {
    bin.map.set_max_block_size(64);
  }
  bin.map.map_all();
  debugmsg("Mapped " + QString::number(bin.map.count()) + " blocks.");

  emit start_program_operation(device_id,device_name(device_id),"READ",read_size);
  emit region_changed(device_id,0,read_size,REGION_UNKNOWN);

  statusmsg("Starting memory dump for device " + device_name(device_id) + " @ 0x" + QString::number(device_id,16).toUpper());
  statusmsg("Device size 0x" + QString::number(read_size,16).toUpper() +
            " Region 0x" + QString::number(read_start,16).toUpper() + "-0x" + QString::number(read_end,16).toUpper());

  // read entire map until empty
  while(bin.map.is_empty() == false) {
    if(is_cancelled()) {
      fail("Operation cancelled");
      return false;
    }
    if(read_segment(device_id,bin.map.take_first()) == false) {
      fail("Could not complete read procedure.");
      return false;
    }
  }

  success("Device read complete.");
  emit bin_read_complete();

  interface->close_port();

  return true;
}

bool processor_8192read::read_segment(int device_id, const region &r) {
  if(r.size > 64) return false; // should never happen
  if(r.size == 0) return true; // skip 0 size regions

  emit region_changed(device_id,r.offset,r.size,REGION_PROGRAMMING);

  int mode = 0x02;

  // for one byte segments if the setting is enabled, we use mode 2.
  if(r.size == 1 && get_setting("M2_M3") == true) mode = 0x03;

  // make request
  datastream_request req(device_id,mode);
  req.set16(0,r.offset); // append position
  datastream_reply repl = interface->request(req);

  // handle reply
  if(repl.success == true) {
    QByteArray out = repl.get_data();
    for(unsigned int x=0;x<r.size;x++) bin.set(r.offset + x,repl.at(x)); // copy data to bin
    emit region_changed(device_id,r.offset,r.size,REGION_READ);
    debugmsg("Read segment " + r.to_string());
    return true;
  } else {
    emit region_changed(device_id,r.offset,r.size,REGION_ERROR);
    errmsg("Error reading segment " + r.to_string());
    return false;
  }
}
