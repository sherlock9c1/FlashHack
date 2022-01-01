#include <QElapsedTimer>
#include <QFile>
#include "../datastream/datastream_8192.h"
#include "processor_ccm.h"

#include "chip_id.h"

processor_ccm::processor_ccm() :
  bin_data(0x10000) {
  // configuration ..  some of these are unused in this platform.  this is a simple platform with onboard eeprom and no flash kernel.
  config_vpp_device_id = CCM;
}

processor_ccm::~processor_ccm() {

}

void processor_ccm::dbg_a() {
  datastream_request r(CCM,0x04); // mode 3 request
  r.append16(0x0020);
  r.append16(0x0020);
  r.append(0x71);
  datastream_reply repl = interface->request(r);
}

bool processor_ccm::has_capability(QString capability_name) {
  if(capability_name == "TEST_INTERFACE") return interface->interface_testable();
  if(capability_name == "SET_VIN") return false;
  if(capability_name == "SET_CALID") return false;
  if(capability_name == "FLASH_READ") return true;
  if(capability_name == "FLASH_WRITE") return true;
  if(capability_name == "HAS_KERNEL") return false;
  return false;
}

QString processor_ccm::get_info() {
  return "Reprograms the 1994-1995 Corvette CCM's internal EEPROM.\n"
         "Should work wtih 16157364, 16212971, 16230561, 16230686, 16223622.\n"
         "These modules are write locked, and before they can be reprogrammed, they generally require disassembling the module so a certain pin can be grounded.\n"
         "See extras/ccm_unlock.pdf.";
}

QString processor_ccm::get_name() {
  return "CCM EEPROM";
}

QList<parameter_def> processor_ccm::get_parameter_list() {
  QList<parameter_def> out;
  out.append(parameter_def("CCM_BLOCK_SIZE",
                           "Write block size",
                           "This is the size of a write block.  Larger is faster in that it produces less datastream messages, so less overhead.",
                           parameter_def::INT,
                           0x7A,true,1,0x7A));

  out.append(parameter_def("CCM_FULL_READ","Read all CCM memory",
                           "This reads all of the memory of the CCM, including the main PROM and RAM.  This is probably a waste of time, unless "
                           "you're doing disassembly or research.",
                           parameter_def::SWITCH,false));

  return out;
}

bool processor_ccm::load_bin(QString path_to_bin) {
  QFile f(path_to_bin);
  if(f.open( QIODevice::ReadOnly ) == false) {
    errmsg("Could not load bin " + path_to_bin + " for reading.");
    return false;
  }
  QByteArray data = f.readAll();
  f.close();
  if(data.size() != 0x10000) {
    errmsg("Could not load bin " + path_to_bin + " (may be corrupt or unreadable)");
    return false;
  }

  bin_data.clear();
  bin_data.set(0,data); // copy only eeprom region.

  statusmsg("Loaded bin " + path_to_bin);
  return true;
}

bool processor_ccm::reset_ccm() {
  statusmsg("Your CCM will now wipe its RAM and crash to cause a reset because I am lazy.  It wont hurt anything.  It may take a few seconds.");
  return execute_simple_program(CCM,"RESET");
}

bool processor_ccm::save_bin(QString path_to_bin) {
  QFile f(path_to_bin);
  if(f.open( QIODevice::WriteOnly ) == false) {
    errmsg("Could not open bin " + path_to_bin + " for writing.");
    return false;
  }

  f.write(bin_data.to_bytearray());
  statusmsg("Saved bin " + path_to_bin);
  return true;
}

bool processor_ccm::verify_unlock() {
  bool is_unlocked = false;

  { // check for software unlock
    datastream_request r(CCM,0x03); // mode 3 request
    r.append16(0x70CA);
    datastream_reply repl = interface->request(r);
    if(repl.success == false) return false;
    bool x = ( repl.at(0) >> 0 ) & 1;
    if(x == true) {
      statusmsg("CCM Software unlock: YES");
      is_unlocked = true;
    } else {
      statusmsg("CCM Software unlock: NO");
    }
  }

  { // check for software unlock
    datastream_request r(CCM,0x03); // mode 3 request
    r.append16(0x644B);
    datastream_reply repl = interface->request(r);
    if(repl.success == false) return false;
    bool x = ( repl.at(0) >> 1 ) & 1;
    if(x == true) {
      statusmsg("CCM Hardware unlock: YES");
      is_unlocked = true;
    } else {
      statusmsg("CCM Hardware unlock: NO");
    }
  }

  return is_unlocked;
}

bool processor_ccm::flash_write() {

  // parameters
  unsigned int eeprom_size = 0x0200;
  unsigned int offset = 0xB600;

  int blksize = get_setting("CCM_BLOCK_SIZE").toInt();

  start("EEPROM Write");

  // make sure we're writing something sane
  if(bin_data.is_valid() == false) {
    fail("EEProm data not loaded or invalid.");
    return false;
  }

  if(verify_unlock() == false) {
    fail("The CCM is still locked.  You will need to open the CCM and ground a pin.  See included instructions in extras/ccm_unlock.pdf");
    return false;
  }

  if(enter_programming_mode(CCM) == false) {
    fail("Could not enter programming mode.");
    return false;
  }

  emit start_program_operation(CCM,device_name(CCM),"PROGRAM",0x10000);
  emit region_changed(CCM,0,0x10000,REGION_UNKNOWN);

  execute_simple_program(CCM,"CCM_WRITE_NEW");

  // build map
  bin_data.map.clear();
  bin_data.map.set_max_block_size(blksize);
  bin_data.map.skip(0,0x10000);
  bin_data.map.unskip(offset,eeprom_size);
  bin_data.map.append(offset,eeprom_size);
  if(bin_data.map.is_complete() == false ) {
    errmsg("The mapping algorithm has failed, and the map is incomplete.");
    return false;
  }

  // iterate over map
  while(bin_data.map.is_empty() == false) {
    if(is_cancelled() == true) {
      fail("Operation cancelled.  If any data was written, state may be inconsistent.");
    }

    region r = bin_data.map.take_first();
    log("PROGRAMMING" + r.to_string(),LOGLEVEL_DEBUG);

    datastream_request request(CCM,COMM_EXECUTE);
    request.append16(0x61C0);   // ram location to store the block
    request.append(0x7E);                             // JMP
    request.append16(0x624C);  // .. to the exec point of the write program.
    request.append(r.size);
    request.append16(r.offset);
    request.append(bin_data.get_region(r));
    request.override_timeout = (r.size * 35) + 150; // could take a bit o' extra time if writing lots of stuff.

    emit region_changed(CCM,r.offset,r.size,REGION_PROGRAMMING);
    datastream_reply repl = interface->request(request);

    if(repl.success == true) {
      emit region_changed(CCM,r.offset,r.size,REGION_WRITTEN);
    } else {
      emit region_changed(CCM,r.offset,r.size,REGION_ERROR);
      errmsg("Error programming EEPROM.");
      fail("Failed to write EEPROM.");
      return false;
    }
  }

  success("EEPROM write complete.");

  reset_ccm();

  return true;
}

bool processor_ccm::flash_read() {
  // determine read size
  unsigned int read_size = 0x10000;
  unsigned int offset = 0x0000;
  if(get_switch("CCM_FULL_READ") == false) {
    read_size = 0x0200;
    offset = 0xB600;
  }

  QByteArray out = m2_read(offset,read_size);
  if(out.isEmpty() == true) {
    fail("Could not finish reading EEPROM.");
    return false;
  } else {
    bin_data.clear();
    bin_data.set(offset,out);
    success("EEPROM read complete.");
    return true;
  }
}

QByteArray processor_ccm::m2_read(unsigned int offset, unsigned int eeprom_size) {
  QByteArray out;

  start("Mode 2 device read");

  statusmsg("Reading EEPROM data from " + device_name(CCM));

  emit start_program_operation(CCM,device_name(CCM),"READ",eeprom_size);
  emit region_changed(CCM,offset,eeprom_size,REGION_UNKNOWN);

  for(unsigned int x=0;x<eeprom_size;x+=64) {
    datastream_request req(CCM,COMM_READ64);
    req.set16(0,offset + x); // append position
    datastream_reply repl = interface->request(req);
    if(repl.success == true) {
      emit region_changed(CCM,offset + x,64,REGION_READ);
      out.append(repl.get_data());
    } else {
      emit region_changed(CCM,offset + x,64,REGION_ERROR);
      errmsg("Error reading mode2 segment " + QString::number(x,16).toUpper());
      fail("Failed to dump device ROM.");
      return QByteArray();
    }
    if(is_cancelled() == true) {
      fail("Failed to dump device ROM, operation cancelled.");
      return QByteArray();
    }
  }

  out.resize(eeprom_size); // kill the end if it's too big .

  return out;
}

void processor_ccm::configure() {
  statusmsg("Configured processor " + get_name() + " with interface " + interface->interface_name());
  statusmsg(get_info());

  interface->configure(); // allow the interface to configure itself now.

  bool valid = true;

  // load programs-------------------------------
  QString subdir("6811/CCM"); // the subdirectory our code is in.
  QStringList programs = programs_in_subdir(subdir);
  for(int x=0;x<programs.size();x++) {
    // set the entire operation as invalid if any program load fails.
    if(load_program_padded(programs.at(x),subdir) == false) valid = false;
  }

  if(valid == false) {
    fail("Some aspects of configuration have failed.  Do not use this routine until they are resolved.");
  }

  // this preconfigures the display.  there is probably a better way.
  emit start_program_operation(CCM,device_name(CCM),"INIT",0x10000);
  //emit start_program_operation(TSIDE,device_name(TSIDE),"INIT",0x10000);
}

QString processor_ccm::device_name(int device) {
  if(device == CCM) return QStringLiteral("CCM");
  return "UNKNOWN_" + QString::number(device,16).toUpper();
}

//---------------------------------------------------------
// MAIN PROGRAMS
//---------------------------------------------------------

