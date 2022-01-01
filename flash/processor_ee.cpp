#include <QElapsedTimer>
#include "../datastream/datastream_8192.h"
#include "processor_ee.h"
#include <QRandomGenerator>
#include "chip_id.h"

processor_ee::processor_ee() {
  // configuration .
  config_write_storage_address = 0x0300;
  config_write_subroutine_address = 0x1BCA;
  config_read_upload_location = 0x0200;
  config_read_subroutine_address = 0x200B; // BRA 0x0B
  config_vpp_device_id = TSIDE;
}

processor_ee::~processor_ee() {
  delete bin;
}

// remove me ...
void processor_ee::dbg_a() {
  statusmsg(QString::number((( ~ bin_for_device(TSIDE)->get_16(0x0E00) ) & 0xFFFF) == (bin_for_device(TSIDE)->get_16(0x0E02) & 0xFFFF)));
}

void processor_ee::dbg_b() {
  write_onboard_eeprom(0xE4);
}

void processor_ee::dbg_c() {
  write_onboard_eeprom(0xF4);

}

bool processor_ee::has_capability(QString capability_name) {
  if(capability_name == "TEST_INTERFACE") return interface->interface_testable();
  if(capability_name == "SET_VIN") return false;
  if(capability_name == "SET_CALID") return false;
  if(capability_name == "FLASH_READ") return true;
  if(capability_name == "FLASH_WRITE") return true;
  if(capability_name == "HAS_KERNEL") return true;
  return false;
}

bool processor_ee::load_kernel() {
  if(_load_kernel(TSIDE) == true) {
    if(_load_kernel(ESIDE) == true) {
      emit kernel_loaded(TSIDE);
      emit kernel_loaded(ESIDE);
      statusmsg("Kernels loaded, ECM executing from RAM.");
      debugmsg("Getting vehicle ID...");
      vehicle_id = get_vehicle_id();
      if(vehicle_id == 0) {
        vehicle_id = set_vehicle_id();
        if(vehicle_id == 0) {
          errmsg("Could not set or retrieve vehicle ID.  The compare functions are not going to work well.");
        }
      }
      return true;
    }
  }
  return false;
}

bool processor_ee::unload_kernel() {
  bool success = true;
  if(_unload_kernel(ESIDE) == false) success = false;
  interface->sleep_ms(350);
  if(_unload_kernel(TSIDE) == false) success = false;
  // FIXME should check to see if ECM is back alive now.
  kernel_version[ESIDE] = 0x00;
  kernel_version[TSIDE] = 0x00;
  return success;
}

QString processor_ee::get_info() {
  return "This is a flash read/write processor for the ECMs used on 1994-1995 LT1 engines.\n"
         "Works with 16188051 and 16181333";
}

QString processor_ee::get_name() {
  return "EE Flash";
}

QList<parameter_def> processor_ee::get_parameter_list() {
  QList<parameter_def> out;

  out.append(parameter_def("EE_SELECTIVE_WRITE",
                           "Only write side(s) that have changed",
                           "When this option is selected, we try not to flash unchanged halves of the ECM.\n"
                           "We save bins on successful read/write, then we checksum those bins against the ECM to see if they're the same.\n"
                           "This minimizes risk and flash time by up to 50%.  Highly recomended.\n"
                           "When this option is selected it also stores a small 'Vehicle ID' on the ECM's EEPROM @ 0x0FFC-0x0FFF so if you were flashing multiple vehicles, everything will be okay.",
                           parameter_def::SWITCH,
                           true));

  out.append(parameter_def("EE_WRITE_ESIDE",
                           "Write E-SIDE",
                           "Write the E-SIDE flash memory.  If you don't know what this means leave it checked.\n"
                           "This doesn't force it to write, and can be overridden by the selective write option above.",
                           parameter_def::SWITCH,
                           true));

  out.append(parameter_def("EE_WRITE_TSIDE",
                           "Write T-SIDE",
                           "Write the T-SIDE flash memory.  If you don't know what this means leave it checked.\n"
                           "This doesn't force it to write, and can be overridden by the selective write option above.",
                           parameter_def::SWITCH,
                           true));

  out.append(parameter_def("EE_WRITE_ONBOARD_EEPROM",
                           "Write onboard EEPROMs",
                           "Both sides of the ECM have processors that contain a writeable EEPROM.\n"
                           "They contains the VIN, calibration ID, and some other nonsense, but it has a large blank area as well.\n"
                           "If you want to write this data from your BIN, definitely leave this box checked.  It's very quick and safe to write.",
                           parameter_def::SWITCH,
                           true));

  out.append(parameter_def("EE_PROTECT_EEPROM",
                           "EEPROM Protection",
                           "Level 0: Protect nothing.  Maybe dangerous unless you are hacking, but probably safe as you could just rewrite from a valid bin.\n"
                           "Level 1: Protect the seed/key and some other misc bytes on the TSIDE and the 0x0E60-0x0EAF region on the ESIDE\n"
                           "Level 2: Also protect the VIN/CALID/Siderail Serial on the TSIDE (VIN/CAL never updated from BIN).",
                           parameter_def::INT,
                           1,false,0,2));

  out.append(parameter_def("EE_PATCH_EEPROM_UNLOCK",
                           "Patch bin to allow full EEPROM writes",
                           "The factory bin locks out most of the onboard EEPROM on the E-SIDE and a very small portion of the T-SIDE.\n"
                           "This switch makes a small change to your bin while flashing it that disables that write lock on next reset, "
                           "allowing that area to be utilized.  This affects both the E-SIDE and T-SIDE.",
                           parameter_def::SWITCH,
                           false));

  out.append(parameter_def("EE_RECOVERY_ESIDE",
                           "Install recovery ROM on E-Side",
                           "This option attempts to leave your ECM bootable even if power fails during most parts of a flash write.\n"
                           "It works by changing some areas of code and flashing regions in a specific order.\n"
                           "When programming ends, a check byte is set and things behave as normal.",
                           parameter_def::SWITCH,
                           true));

  out.append(parameter_def("EE_RECOVERY_TSIDE",
                           "Install recovery ROM on T-Side",
                           "This option attempts to leave your ECM bootable even if power fails during most parts of a flash write.\n"
                           "It works by changing some areas of code and flashing regions in a specific order.\n"
                           "When programming ends, a check byte is set and things behave as normal.",
                           parameter_def::SWITCH,
                           true));

  out.append(parameter_def("EE_BLOCK_SIZE",
                           "Transaction block size",
                           "This is the size of a write block.\nSmaller sizes may be better for crappy interfaces, but 128 will be fastest.\n"
                           "This affects both flash and onboard eeprom read and write.",
                           parameter_def::INT,
                           128,true,1,128));

  out.append(parameter_def("EE_VERIFY_PATCHES",
                           "Verify patching regions against reference.",
                           "When applying any patches to the bin, compare reference data from a GM bin to ensure that "
                           "the regions in question have not been modified in a way that may cause the patch to fail.",
                           parameter_def::SWITCH,
                           true));

  out.append(parameter_def("EE_SKIP_EMPTY_REGIONS",
                           "Do not write 0xFF regions",
                           "When erasing flash memory, we first erase and set every bit, making the entire bin 0xFF.\n"
                           "This means bytes that are already 0xFF need not be written to save time.  Highly recommended.",
                           parameter_def::SWITCH,
                           true));

  out.append(parameter_def("EE_PATCH_UNUSED_REGIONS",
                           "Set unused regions to 0xFF",
                           "The write program will allow us to skip over anything that is set to 0xFF to save time.\n"
                           "This option will set unused areas of the bin to 0xFF (after checking if they are actually unused)\n"
                           "It shouldn't hurt anything.",
                           parameter_def::SWITCH,
                           true));

  out.append(parameter_def("EE_CLEAR_AUTO_TRANS",
                           "Clear auto trans tables to 0xFF on manual bin",
                           "If 'Set unused regions to 0xFF' is enabled, and the car is set to manual transmission, "
                           "we can skip a large area of the bin that is full of transmission tables\n"
                           "If you are using this area for something else, disable this option.\n"
                           "The region is on the TSIDE at 0x28A0 and is 4160 bytes long.",
                           parameter_def::SWITCH,
                           true));

  out.append(parameter_def("EE_CLEAR_MAF_TABLE",
                           "Clear MAF table on SD bins",
                           "If the MAF is disabled, we don't need a maf table.  This sets it to 0xFF so it can be skipped.",
                           parameter_def::SWITCH,
                           false));

  out.append(parameter_def("EE_READ_ONBOARD_EEPROM",
                           "Read onboard EEPROM",
                           "The processor contains a small onboard EEPROM that is used to store the VIN and calibration id among other things.\n"
                           "Your bin editor may display the VIN an Cal ID, and those would be blank if this option is disabled.",
                           parameter_def::SWITCH,
                           true));

  out.append(parameter_def("EE_READ_RAM",
                           "Read RAM region",
                           "Read the 8kB of RAM at the beginning of the bin.  Mostly useless to do this.",
                           parameter_def::SWITCH,
                           false));

  out.append(parameter_def("EE_IGNORE_ODD_CHIP_ID",
                           "Ignore odd chip ID",
                           "We make sure the chip we are programming has a normal chip ID.\n"
                           "If you use aftermarket or odd chips, this might cause programming issues.\n"
                           "If you know your chips are okay, set this option so writing can proceed as normal...",
                           parameter_def::SWITCH,
                           false));

  out.append(parameter_def("EE_HANDLE_ESIDE_COMMS_PATCH",
                           "Handle E-Side comms patch",
                           "EEHack/EEX contains a patch to enable extra communications for the E-Side that are mostly useless.\n"
                           "This can interfere with installation of the recovery rom without workarounds.\n"
                           "This option enables those workarounds so those bins also have a functional recovery rom.",
                           parameter_def::SWITCH,
                           true));

  return out;
}

void processor_ee::configure() {
  statusmsg("Configured processor " + get_name() + " with interface " + interface->interface_name());
  statusmsg(get_info());

  interface->configure(); // allow the interface to configure itself now.

  bool valid = true;

  // configure bin file storage------------------
  bin = new binfile_ee();

  // configure block size defaults
  set_max_block_size(128);

  // TEMP
  config_write_eside = true;
  config_write_tside = true;

  // load programs-------------------------------
  QString subdir("6811/EE"); // the subdirectory our code is in.
  QStringList programs = programs_in_subdir(subdir);
  for(int x=0;x<programs.size();x++) {
    // set the entire operation as invalid if any program load fails.
    if(load_program_padded(programs.at(x),subdir) == false) valid = false;
  }

  // load patches into the programs array too----
  QStringList patches = programs_in_subdir(subdir + "/PATCHES");
  for(int x=0;x<patches.size();x++) {
    if(load_program(patches.at(x),subdir + "/PATCHES") == false) valid = false;
  }

  // configure subroutine call locations and timeouts.
  add_subroutine("VPP_APPLY",  0x1A00,5000);
  add_subroutine("VPP_REMOVE", 0x1A80);
  add_subroutine("WRITE",      0x1BB0);
  add_subroutine("CHECKSUM",   0x1C2E,5000);
  add_subroutine("ERASE",      0x1AD0,8000);
  add_subroutine("CHIPID",     0x1C80);

  if(valid == false) {
    fail("Some aspects of configuration have failed.  Do not use this routine until they are resolved.");
  }

  // this preconfigures the display.  there is probably a better way.
  emit start_program_operation(ESIDE,device_name(ESIDE),"INIT",0x10000);
  emit start_program_operation(TSIDE,device_name(TSIDE),"INIT",0x10000);
}

//---------------------------------------------------------
// MAIN PROGRAMS
//---------------------------------------------------------

bool processor_ee::flash_write() {
  if(is_cancelled()) return false; // -----safe abort point

  start("EE write procedure");

  //------------------------------------------------------

  config_write_eside = get_switch("EE_WRITE_ESIDE");
  config_write_tside = get_switch("EE_WRITE_TSIDE");

  if(( bin->eside.is_valid() == false && config_write_eside ) ||
     ( bin->tside.is_valid() == false && config_write_tside)) {
    fail("Loaded bin is invalid.");
    return false;
  }

  //------------------------------------------------------

  if(is_cancelled()) {
    fail("Operation cancelled.");
    return false;
  }

  //------------------------------------------------------

  bin->tside.map.clear();
  bin->eside.map.clear();

  set_max_block_size(get_setting("EE_BLOCK_SIZE").toInt());

  if(get_switch("EE_RECOVERY_ESIDE") == true && install_recovery_rom(ESIDE) == false) {
    fail("Could not install recovery ROM on ESIDE.");
    return false;
  }

  if(get_switch("EE_RECOVERY_TSIDE") == true && install_recovery_rom(TSIDE) == false) {
    fail("Could not install recovery ROM on TSIDE.");
    return false;
  }

  patch_full_eeprom_write(get_switch("EE_PATCH_EEPROM_UNLOCK"));

  if(build_write_map(TSIDE) == false || build_write_map(ESIDE) == false) {
    fail("Could not build write map.");
    return false;
  }

  //------------------------------------------------------

  statusmsg("Starting EE write procedure.");

  if(load_kernel() == false) {
    fail("Kernel could not be loaded");
    return false;
  }

  if(get_switch("EE_SELECTIVE_WRITE") == true) check_selective_write();

  if(is_cancelled()) {
    fail("Operation cancelled.");
    return false;
  }

  if(config_write_tside == true) {
    if(apply_vpp() == false) {
      fail("Could not apply VPP voltage to the TSIDE flash chip.");
      return false;
    }
    if(flash_write_device(TSIDE) == false) {
      fail("Could not complete the write procedure on " + device_name(TSIDE) + "\n"
           "You will want to run the procedure again.  Do not turn your ignition off until this is resolved!");
      return false;
    }
    remove_vpp();
    if(verify_write_checksum(TSIDE) == true) {
      config_write_tside = false;
      store_previously_completed_bin(TSIDE);
    } else {
      fail("The write for the TSIDE operation completed but appears to be corrupt, "
           "you should retry the procedure before rebooting the ECM.");
      return false;
    }
  }

  if(is_cancelled()) {
    fail("Operation cancelled.");
    return false;
  }

  if(config_write_eside == true) {
    if(apply_vpp() == false) {
      fail("Could not apply VPP voltage to the ESIDE flash chip");
      return false;
    }
    if(flash_write_device(ESIDE) == false) {
      fail("Could not complete the write procedure on " + device_name(ESIDE) + "\n"
           "You will want to run the procedure again.  Do not turn your ignition off until this is resolved!");
      return false;
    }
    remove_vpp();
    if(verify_write_checksum(ESIDE) == true) {
      config_write_eside = false;
      store_previously_completed_bin(ESIDE);
    } else {
      fail("The write operation for the ESIDE completed but appears to be corrupt, "
           "you should retry the procedure before rebooting the ECM.");
      return false;
    }
  }

  // see if there is actually valid onboard eeprom data.  we can see if the challenge/response is valid.
  if((( ~ bin_for_device(TSIDE)->get_16(0x0E00) ) & 0xFFFF) == (bin_for_device(TSIDE)->get_16(0x0E02) & 0xFFFF)) {

    if(get_switch("EE_WRITE_ONBOARD_EEPROM") == true) {
      if(write_onboard_eeprom(TSIDE) == false) {
        errmsg("Could not write onboard EEPROM on TSIDE.  It is unlikely that this has damaged your ECM but it should be rewritten at some point.");
      }
      if(write_onboard_eeprom(ESIDE) == false) {
        errmsg("Could not write onboard EEPROM on ESIDE.  It is unlikely that this has damaged your ECM but it should be rewritten at some point.");
      }
    }

  } else {
    statusmsg("Bin contains no or valid data for the onboard EEPROM, skipping onboard EEPROM write routine for safety.");
  }

  success("Flash write procedure complete!");

  unload_kernel(); // only if successful.

  interface->close_port();

  return true;
}

bool processor_ee::patch_full_eeprom_write(bool enable) {
  if(enable == true) {
    statusmsg("Installing patch to enable EEPROM writes on reboot");
    bin_for_device(TSIDE)->set(0x428F,0x10);
    bin_for_device(ESIDE)->set(0x3469,0x10);
  } else { // back to factory defaults
    bin_for_device(TSIDE)->set(0x428F,0x11);
    bin_for_device(ESIDE)->set(0x3469,0x1B);
  }
  bin_for_device(TSIDE)->reset_checksum();
  bin_for_device(ESIDE)->reset_checksum();
  return true;
}

bool processor_ee::build_write_map(int device_id) {
  debugmsg("Building write region map for " + device_name(device_id));

  binfile *b = bin_for_device(device_id);

  // configuration
  unsigned int checkbyte_location = 0xFF84;
  int min_skip_ff_bytes = 4;

  // skip ram
  b->map.skip(0x0000,0x2000);

  // now would be a great place to skip unused bytes...
  if(get_switch("EE_PATCH_UNUSED_REGIONS") == true) ff_unused_regions(device_id,false);

  // skip regions with 0xFF.
  if(get_switch("EE_SKIP_EMPTY_REGIONS") == true) b->map.skip_areas_with_byte(0xFF,min_skip_ff_bytes,b->to_bytearray());

  // complete the map.  this maps everything we haven't already mapped or ignored...
  b->map.map_all();

  // now we have to write the check byte last if we're using the safety patch.
  if(get_switch("EE_RECOVERY_" + device_name(device_id)) == true) {
    b->map.unskip(checkbyte_location,1); // we 'unskip' it, so add_region works again.
    b->map.append(checkbyte_location,1); // .. and remap it at the end so it's the last thing written.
  }

  if(b->map.is_complete() == false ) {
    errmsg("The mapping algorithm has failed, and the map is incomplete on " + device_name(device_id));
    return false;
  }

  debugmsg(device_name(device_id) + " mapping algorithm skipped " + QString::number(b->map.skip_count()) + " bytes.");

  return true;
}

quint32 processor_ee::get_vehicle_id() {
  datastream_reply r = execute_program(TSIDE,"GET_VID");
  if(r.success == false) {
    debugmsg("Could not get vehicle ID.");
    return 0;
  }
  quint32 id = r.get32(0x01);
  if(id == 0 || id == 0xFFFFFFFF) {
    statusmsg("Vehicle ID not programmed yet. " + QString::number(id,16));
    return 0;
  }
  debugmsg("Got vehicle id " + QString::number(id,16));
  return id;
}

quint32 processor_ee::set_vehicle_id() {
  quint32 id = 0;
  while(id == 0) id = QRandomGenerator::securelySeeded().generate();
  if(execute_simple_program(TSIDE,"ONBOARD_EEPROM") == false) return false;
  region r(0x0FFC,4);
  datastream_request req;
  req.set32(0,id);
  statusmsg("Programming new Vehicle ID " + QString::number(id,16));
  bool success = program_eeprom_block(TSIDE,r,req.get_data());
  if(success == true) return id;
  return 0;
}

bool processor_ee::write_onboard_eeprom(int device_id) {

  // parameters.  the actual eeprom is 0x0E00 and 0x200 long but we only write a subset of that for now
  unsigned int eeprom_size = 0x0200;
  unsigned int offset = 0x0E00;

  int blksize = get_setting("EE_BLOCK_SIZE").toInt();

  binfile *b = bin_for_device(device_id);
  if(b->is_valid() == false) {
    errmsg("Bin data invalid.");
    return false;
  }

  emit region_changed(device_id,offset,eeprom_size,REGION_UNKNOWN);

  // build map.  this does kill any existing map.  should not be an issue

  b->map.clear();
  b->map.set_max_block_size(blksize);

  // first set the correct overall region by skipping everything but the eeprom
  b->map.skip(0,0x10000);  
  b->map.unskip(offset,eeprom_size);

  // protect some areas. based on protection level.
  int protection_level = get_setting("EE_PROTECT_EEPROM").toInt();
  if(device_id == TSIDE) {
    b->map.skip(0x0FFC,0x04); // last 4 bytes are our programmed vehicle ID.  never write those with bin data.
    if(protection_level >= 1) {
      b->map.skip(0x0E00,0x04); // SEED/KEY
      b->map.skip(0x0E14,0x0C); // UNKNOWN TABLE
    }
    if(protection_level >= 2) {
      b->map.skip(0x0E00,0x35); // skip entire tside config area
    }
  } else if(device_id == ESIDE) {
    if(protection_level >= 1) {
      b->map.skip(0x0E60,0x50); // random section that the e-side seems to use
    }
  }

  b->map.map_all(); // map remainer.

  if(b->map.is_complete() == false ) {
    errmsg("The mapping algorithm has failed, and the map is incomplete.");
    return false;
  }

  statusmsg("Programming onboard EEPROM for " + device_name(device_id));

  // iterate over map
  while(b->map.is_empty() == false) {
    region r = b->map.take_first();
    log("PROGRAMMING" + r.to_string(),LOGLEVEL_DEBUG);
    if(program_eeprom_block(device_id,r,b->get_region(r)) == false) return false;
  }

  return true;
}

bool processor_ee::program_eeprom_block(int device_id, region r, QByteArray data) {
  log("PROGRAMMING" + r.to_string(),LOGLEVEL_DEBUG);

  datastream_request request(device_id,COMM_EXECUTE);
  request.append16(0x0300);   // ram location to store the block
  request.append(0x7E);                             // JMP
  request.append16(0x1CB4);  // .. to the exec point of the write program.
  request.append(r.size);
  request.append16(r.offset);
  data.resize(r.size);
  request.append(data);
  request.override_timeout = (r.size * 35) + 150; // could take a bit o' extra time if writing lots of stuff.

  emit region_changed(device_id,r.offset,r.size,REGION_PROGRAMMING);
  datastream_reply repl = interface->request(request);

  if(repl.success == true) {
    emit region_changed(device_id,r.offset,r.size,REGION_WRITTEN);
    return true;
  } else {
    emit region_changed(device_id,r.offset,r.size,REGION_ERROR);
    errmsg("Error programming EEPROM.");
    return false;
  }
}

bool processor_ee::install_recovery_rom(int device_id) {

  // thank kur4o for this great peice of work.  we patch in a safety switch so that very early on
  // in the routine, we have enough written material to re-execute a mode 0x05 loop from the flash rom.
  // when bin is finished writing we flip the switch and it behaves as normal.  this means we have
  // an incredibly small window for failure, and anything outside that window should withstand both
  // ECM and programmer failures of any kind.......

  if(device_id == ESIDE && bin->is_rev_b()) {
    errmsg("This is an EEB revision bin file which cannot run the ESIDE recovery patch.\n"
           "We will temporarily disable this feature for you.\n"
           "It would be better and safer to switch to a regular EE bin...");
    return true;
  }

  binfile *b = bin_for_device(device_id);

  // load the recovery rom datafile
  binfile recrom(programs.value(device_name(device_id) + "_RECOVERY").data);
  if(recrom.size() != 0x10000) {
    errmsg("Could not load recovery rom datafile for " + device_name(device_id));
    return false;
  }

  // let the mapping routine find our patch regions.  the recovery rom is layout is continuous blocks of
  // FF of at least 4 bytes where no changes are made, so we abuse the skip function to build the map.
  recrom.map.clear();
  recrom.map.skip_areas_with_byte(0xFF,4,recrom.to_bytearray());
  recrom.map.map_all();

  // load the reference bin to compare against
  binfile_ee refbin(programs.value("REFERENCE_BIN").data);
  if(refbin.is_valid() == false) {
    errmsg("Could not load reference bin or data invalid.");
    return false;
  }

  // we need to patch our patch if the eside comms patch is being used.  we also need to patch the
  // reference bin to make the below compare work properly.
  if(device_id == ESIDE && get_switch("EE_HANDLE_ESIDE_COMMS_PATCH") == true) {
    quint16 b7237 = b->get_16(0x7237);
    quint16 b82dd = b->get_16(0x82DD);
    recrom.set_16(0x7237,b7237);
    recrom.set_16(0x82DD,b82dd);
    refbin.eside.set_16(0x7237,b7237);
    refbin.eside.set_16(0x82DD,b82dd);
  }

  // check if we've already installed the recovery rom and continue.
  if(b->is_identical_to(recrom,recrom.map) == true) {
    debugmsg("Recovery rom has already been installed.");
    return true;
  }

  // verify the integrity of the patch regions before patching, this will save us if some unknown
  // code has been altered in the patch region, as we're dealing with comms code here, it would be
  // really shitty if our patch caused the ECM to not communicate.
  if(get_switch("EE_VERIFY_PATCHES") == true) {

    // this workaround is for people that went ahead and read back a bin that has already been FF filled
    // in a certain region in EE, which is something i can't imagine why you would do.
    // anyway now we have to un-FF that region in the ESIDE or compare will fail...
    if(device_id == ESIDE) {
      for(int x=0xFF80;x<=0xFF9F;x++) if(b->get(x) == 0xFF) b->set(x,0x00);
    }

    // compare to the reference bin to see if anything in those regions is odd
    if((device_id == TSIDE && b->is_identical_to(refbin.tside,recrom.map) == false) ||
       (device_id == ESIDE && b->is_identical_to(refbin.eside,recrom.map) == false)) {
      errmsg("Your bin has been altered in a patch region, making applying this patch unsafe.");
      return false;
    }
  }

  int mapped_bytes = recrom.map.mapped_bytes(); // save for info later as we're eating the list.

  detailmsg("Installing recovery rom for " + device_name(device_id) + " (recovery window " + QString::number(recrom.map.mapped_bytes()) + " bytes)");

  // now patch and map everything.
  while(recrom.map.is_empty() == false) {
    region r = recrom.map.take_first();
    b->patch(recrom.get_region(r),r);
    b->map.append(r);
  }

  // set the check byte and 'skip it' for now.
  b->set(0xFF84,0x00);
  b->reset_checksum();
  b->map.skip(0xFF84,1);

  debugmsg("Recovery bin installed for " + device_name(device_id) + " size " + QString::number(mapped_bytes));
  return true;
}


bool processor_ee::build_read_map() {
  if(build_read_map(TSIDE) == false) return false;
  if(build_read_map(ESIDE) == false) return false;
  return true;
}

bool processor_ee::build_read_map(int device_id) {
  debugmsg("Building read region map for " + device_name(device_id));
  binfile *b = bin_for_device(device_id);
  b->map.clear();

  // read onboard eeprom
  if(get_switch("EE_READ_ONBOARD_EEPROM") == true) b->map.append(0x0E00,512);

  // skip ram
  if(get_switch("EE_READ_RAM") == false) b->map.skip(0x0000,0x2000);

  // map remainder
  b->map.map_all();

  if(b->map.is_complete() == false) {
    errmsg("The mapping algorithm has failed, and the map is incomplete.");
    return false;
  }

  return true;
}

bool processor_ee::flash_read() {
  cancel_operation = false;
  start("EE read procedure.");

  set_max_block_size(get_setting("EE_BLOCK_SIZE").toInt());

  if(build_read_map() == false) {
    fail("Could not build a read map.");
    return false;
  }

  if(load_kernel() == false) {
    fail("Could not connect or load the kernel.");
    return false;
  }

  if(is_cancelled()) {
    fail("Operation cancelled.");
    return false;
  }

  if(flash_read_device(TSIDE) == true) {
    if(flash_read_device(ESIDE) == true) {
      success("Read procedure complete.");

      store_previously_completed_bin(ESIDE);
      store_previously_completed_bin(TSIDE);

      emit bin_read_complete();
      unload_kernel();
      interface->close_port();
      return true;
    }
  }
  fail("Read procedure failed.  Review the log.");
  unload_kernel();
  interface->close_port();
  return false;
}

bool processor_ee::store_local_bin(int device_id, QString name, const bin_ee &bin_in) {
  return set_datafile(device_name(device_id) + "_" + name,"6811/EE/STORAGE",bin_in.to_bytearray());
}

bin_ee processor_ee::get_local_bin(int device_id, QString name) {
  bin_ee out(get_datafile(device_name(device_id) + "_" + name,"6811/EE/STORAGE"));
  return out;
}

void processor_ee::store_previously_completed_bin(int device_id) {
  bin_ee b(bin_for_device(device_id)->to_bytearray());
  store_local_bin(device_id,QString::number(vehicle_id,16) + "PREVIOUS",b);
}

void processor_ee::store_invalid_previous_bin(int device_id) {
  bin_ee b;
  store_local_bin(device_id,QString::number(vehicle_id,16) + "PREVIOUS",b);
}

bool processor_ee::has_bin_changed(int device_id) {
  bin_ee last_bin = get_local_bin(device_id,QString::number(vehicle_id,16) + "PREVIOUS");

  if(last_bin.is_valid() == false) return true; // no previous valid bin
  bin_ee *this_bin = (bin_ee*)bin_for_device(device_id);

  // compare
  region compare_region(0x2000,0xE000);
  region_map compare_map(0x10000);
  compare_map.append(compare_region);
  if(this_bin->is_identical_to(last_bin,compare_map) == false) {
    detailmsg(device_name(device_id) + " does not match the new bin.");
    return true;
  }

  // verify checksum
  quint16 bin_checksum = this_bin->generate_simple_checksum(0x2000,0xFFFF);
  quint16 ecm_checksum = get_checksum(device_id,0x2000,0xFFFF);
  if(bin_checksum == ecm_checksum) return false; // unchanged according to checksum
  detailmsg(device_name(device_id) + " checksum mismatch, considering altered.");
  return true;
}

QByteArray processor_ee::get_vin() {
  // construct request
  datastream_request req(TSIDE,0x01); // mode 1
  req.set_first(0x04); // message 4

  // submit request
  datastream_reply repl = interface->request(req); // send request

  // handle errors by returning empty array
  if(repl.success == false) return QByteArray();
  if(repl.size() < 17) return QByteArray();

  // extract vin
  QByteArray vin = repl.get_data().mid(0,17); // first 17 bytes

  // validate vin here ?
  return vin;
}

void processor_ee::ff_unused_regions(int device_id, bool reverse) {
  unsigned char x = 0xFF;
  unsigned char y = 0x00;
  if(reverse) std::swap(x,y);
  if(device_id == ESIDE) {
    set_x_if_y(ESIDE,region(0x8330,3279),x,y); // large unused region
    set_x_if_y(ESIDE,region(0x9401,0x6B7F),0xFF,0x00); // very large unused region
    set_x_if_y(ESIDE,region(0x9000,1024),0xFF,0x00); // tunercat notes field
    if(get_switch("EE_CLEAR_MAF_TABLE") && bin->is_maf_enabled() == false) bin->eside.fill(0x241A,154,0xFF);
    bin->eside.reset_checksum();
  } else if(device_id == TSIDE) { //2A0
    if(get_switch("EE_CLEAR_AUTO_TRANS") && bin->is_auto_transmission() == false) bin->tside.fill(0x28A0,4160,0xFF);
    bin->tside.reset_checksum();
  }
}

bool processor_ee::check_selective_write() {
  if(config_write_tside == true) {
    if(has_bin_changed(TSIDE) == false) {
      statusmsg("TSIDE is unchanged since last write.  Skipping flash write.");
      config_write_tside = false;
    }
  }
  if(config_write_eside == true) {
    if(has_bin_changed(ESIDE) == false) {
      statusmsg("ESIDE is unchanged since last write.  Skipping flash write..");
      config_write_eside = false;
    }
  }
  return true;
}

bool processor_ee::verify_write_checksum(int device_id) {
  if(device_id != TSIDE && device_id != ESIDE) return false;

  detailmsg("Verifying checksum for " + device_name(device_id));
  bin_ee *b = (bin_ee*)bin_for_device(device_id);
  quint16 bin_checksum = b->generate_simple_checksum(0x2000,0xFFFF);
  quint16 ecm_checksum = get_checksum(device_id,0x2000,0xFFFF);
  debugmsg("CHECKSUM " + device_name(device_id) + " BIN=" +
           QString::number(bin_checksum,16) + " ECM=" +
           QString::number(ecm_checksum,16));
  if(ecm_checksum == bin_checksum) {
    return true;
  } else {
    errmsg("Bad checksum on " + device_name(device_id) + ": Written data is likely corrupt.");
    return false;
  }
}

quint16 processor_ee::get_checksum(int device_id, quint16 start_address, quint16 end_address) {
  if(device_id != TSIDE && device_id != ESIDE) return 0x0000;

  datastream_request r(device_id,COMM_EXECUTE);

  r.append16(0x1C75);
  r.append(0x7E);  // JMP
  r.append16(programs["CHECKSUM"].offset);
  r.append16(start_address);
  r.append16(end_address);
  r.append16(0x0000);

  datastream_reply reply = interface->request(r);

  if(reply.success == false || reply.at(0) != COMM_SUCCESS) {
    errmsg("Failed to get checksum for " + device_name(device_id));
    return 0x0000;
  }

  quint16 out = reply.at(2) & 0x00FF;
  out |= ( reply.at(1) << 8 ) & 0xFF00;
  return out;
}

bool processor_ee::load_bin(QString path_to_bin) {
  bool success = bin->load_file(path_to_bin);
  if(success == true) {
    statusmsg("Loaded bin " + path_to_bin);
    statusmsg(bin->bin_info());
    current_vin = bin->get_vin();
    current_cal_id = bin->get_cal_id();
  } else {
    errmsg("Could not load bin " + path_to_bin + " (may be corrupt or unreadable)");
    current_vin.clear();
    current_cal_id = 0;
  }
  return success;
}

bool processor_ee::save_bin(QString path_to_bin) {
  bool success = bin->save_file(path_to_bin);
  if(success == true) {
    statusmsg("Saved bin " + path_to_bin);
  } else {
    errmsg("Could not save bin " + path_to_bin);
  }
  return success;
}

bool processor_ee::erase_flash(int device_id) {
  // we now invalidate the 'previous' bin stored on disk.
  store_invalid_previous_bin(device_id);
  return processor_8192dualflash::erase_flash(device_id); // call the generic erase routine .
}

void processor_ee::set_max_block_size(int size) {
  // silently enforce max block sizes.
  if(size < 1) size = 1;
  if(size > 128) size = 128;
  bin_for_device(TSIDE)->map.set_max_block_size(size);
  bin_for_device(ESIDE)->map.set_max_block_size(size);
}

bool processor_ee::get_chip_id(int device_id) {
  // get the chip id and make sure its normal
  datastream_reply chip_id_reply = execute_program(device_id,"CHIPID");

  if(chip_id_reply.success == false || chip_id_reply.first() != COMM_SUCCESS)  return false;

  if(chip_id_reply.size() == 3) {
    detailmsg("Identified " + device_name(device_id) + " FLASH CHIP " +
              flash_chip_name(chip_id_reply.at(1),chip_id_reply.at(2)));
  } else {
    errmsg("Error getting chip identifier.  Erasing might be a bad idea.");
    return false;
  }

  if(get_switch("EE_IGNORE_ODD_CHIP_ID") == true || chip_id_reply.at(1) != 0x89 || chip_id_reply.at(2) != 0xB8) {
    errmsg("The chip is unusual and it may be unsafe to erase and flash.  You need to set the ignore odd chip id parameter to continue.");
    return false;
  }

  return true;
}

binfile *processor_ee::bin_for_device(int device_id) {
  if(device_id == TSIDE) return & bin->tside;
  if(device_id == ESIDE) return & bin->eside;
  return nullptr; // should not happen.
}

bool processor_ee::tside_eeprom_data_valid() {
  return ((~ bin_for_device(TSIDE)->get_16(0x0E00) & 0xFFFF) == (bin_for_device(TSIDE)->get_16(0x0E02) & 0xFFFF));
}
