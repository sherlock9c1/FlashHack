#include "processor_p66.h"
#include "chip_id.h"
#include <QFile>

void processor_p66::dbg_a() {
  load_kernel();
  exec_write_program(TSIDE);
  exec_write_program(ESIDE);
}

void processor_p66::dbg_b() {
  build_write_map(ESIDE);
  build_write_map(TSIDE);
  statusmsg("DUMP OF ESIDE WRITE MAP ------");
  statusmsg(eside_bin.map.to_string());

  statusmsg("DUMP OF TSIDE WRITE MAP ------");
  statusmsg(tside_bin.map.to_string());
}

void processor_p66::dbg_c() {
  erase_flash(TSIDE);
}

QList<parameter_def> processor_p66::get_parameter_list() {
  QList<parameter_def> out;

  out.append(parameter_def("P66_WRITE_ESIDE",
                           "Only write side(s) that have changed",
                           "When this option is selected, do not flash unchanged halves of the ECM.\n"
                           "When we know that an entire flash has been written and verified, save that bin for comparison.\n"
                           "Also verifies that bin's checksum against the one on the ECM first.\n"
                           "This minimizes risk and flash time by up to 50%.  Highly recomended.\n"
                           "If you constantly flash multiple vehicles, perhaps turn it off, though.",
                           parameter_def::SWITCH,
                           true));

  out.append(parameter_def("P66_WRITE_ESIDE",
                           "Write E-SIDE",
                           "Enable to write the E-SIDE flash.  If you don't know what this means leave it checked.\n",
                           parameter_def::SWITCH,
                           true));

  out.append(parameter_def("P66_WRITE_TSIDE",
                           "Write T-SIDE",
                           "Write the T-SIDE flash.  If you don't know what this means leave it checked.\n",
                           parameter_def::SWITCH,
                           true));

  out.append(parameter_def("P66_BLOCK_SIZE",
                           "Transaction block size",
                           "This is the size of a write block.\nSmaller sizes may be better for crappy interfaces, but 128 will be fastest.\n"
                           "This affects both read and write.",
                           parameter_def::INT,
                           128,true,1,128));

  out.append(parameter_def("P66_SKIP_EMPTY_REGIONS",
                           "Do not write 0xFF regions",
                           "Before writing the flash memory, the erase routine sets every bit, making the entire bin 0xFF.\n"
                           "This means bytes that are already 0xFF need not be written to save time.  Highly reccommended.",
                           parameter_def::SWITCH,
                           true));

  out.append(parameter_def("P66_READ_RAM",
                           "Read RAM region",
                           "Read the 8kB of RAM at the beginning of the bin.  Mostly useless to do this.",
                           parameter_def::SWITCH,
                           false));

  out.append(parameter_def("P66_IGNORE_ODD_CHIP_ID",
                           "Ignore odd chip ID",
                           "We make sure the chip we are programming has a normal chip ID.\n"
                           "If you use aftermarket or odd chips, this might cause programming issues.\n"
                           "If you know your chips are okay, set this option so writing can proceed as normal...",
                           parameter_def::SWITCH,
                           false));

  return out;
}

bool processor_p66::get_chip_id(int device_id) {
  // get the chip id and make sure its normal
  datastream_reply chip_id_reply = execute_program(device_id,"CHIPID");

  if(chip_id_reply.success == false || chip_id_reply.first() != COMM_SUCCESS)  return false;

  if(chip_id_reply.size() == 3) {
    detailmsg("Identified " + device_name(device_id) + " CHIP " +
              flash_chip_name(chip_id_reply.at(1),chip_id_reply.at(2)));
  } else {
    errmsg("Error getting chip identifier.  Erasing might be a bad idea.");
    return false;
  }

  if(get_switch("P66_IGNORE_ODD_CHIP_ID") == true || chip_id_reply.at(1) != 0x89 || chip_id_reply.at(2) != 0xB9) {
    errmsg("The chip is unusual and it may be unsafe to erase and flash.  You need to set the ignore odd chip id parameter to continue.");
    return false;
  }

  return true;
}

processor_p66::processor_p66() : tside_bin(0x10000),eside_bin(0x10000) {
  config_write_storage_address = 0x0300;
  config_write_subroutine_address = 0x021A;
  config_read_upload_location = 0x0200;
  config_read_subroutine_address = 0x200B;
  config_vpp_device_id = TSIDE;
}

bool processor_p66::load_kernel() {
  if(_load_kernel(ESIDE) == true) {
    if(_load_kernel(TSIDE) == true) {
      emit kernel_loaded(TSIDE);
      emit kernel_loaded(ESIDE);
      statusmsg("Kernels loaded, ECM executing from RAM.");
      return true;
    }
  }
  return false;
}

bool processor_p66::unload_kernel() {
  bool success = true;
  if(_unload_kernel(TSIDE) == false) success = false;
  interface->sleep_ms(1000);
  if(_unload_kernel(ESIDE) == false) success = false;
  // FIXME should check to see if ECM is back alive now.
  // this throws an error since the T-Side doesn't respond to the reset
  // request, it just reboots.
  kernel_version[ESIDE] = 0x00;
  kernel_version[TSIDE] = 0x00;
  return success;
}

bool processor_p66::has_capability(QString capability_name) {
  if(capability_name == "TEST_INTERFACE") return interface->interface_testable();
  if(capability_name == "SET_VIN") return true;
  if(capability_name == "SET_CALID") return true;
  if(capability_name == "FLASH_READ") return true;
  if(capability_name == "FLASH_WRITE") return true;
  if(capability_name == "HAS_KERNEL") return true;
  return false;
}

bool processor_p66::load_bin(QString path_to_bin) {
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

  // split tside/eside
  tside_bin.set(0x8000,data.mid(0x0000,0x8000));
  eside_bin.set(0x8000,data.mid(0x8000,0x8000));

  statusmsg("Loaded bin " + path_to_bin);
  return true;
}

bool processor_p66::save_bin(QString path_to_bin) {
  QFile f(path_to_bin);
  if(f.open( QIODevice::WriteOnly ) == false) {
    errmsg("Could not open bin " + path_to_bin + " for writing.");
    return false;
  }

  QByteArray out;
  out.append(tside_bin.to_bytearray().mid(0x8000,0x8000));
  out.append(eside_bin.to_bytearray().mid(0x8000,0x8000));

  f.write(out);
  statusmsg("Saved bin " + path_to_bin);
  return true;
}

QString processor_p66::get_info() {
  return "This is a flash read/write processor for P66 V6 ECMs used on 3100 and 3.4L engines from 1993-1995.";
}

QString processor_p66::get_name() {
  return "This is a flash read/write processor for P66 V6 ECMs used on 3100 and 3.4L engines from 1993-1995.\n"
         "Should work with 16172693, 16184164, 16184737, 16196397";
}

binfile *processor_p66::bin_for_device(int device_id) {
  if(device_id == TSIDE) return & tside_bin;
  if(device_id == ESIDE) return & eside_bin;
  return nullptr; // should not happen.
}

void processor_p66::set_max_block_size(int size) {
  // silently enforce max block sizes.
  if(size < 1) size = 1;
  if(size > 128) size = 128;
  bin_for_device(TSIDE)->map.set_max_block_size(size);
  bin_for_device(ESIDE)->map.set_max_block_size(size);
}

void processor_p66::configure() {
  statusmsg("Configured processor " + get_name() + " with interface " + interface->interface_name());
  statusmsg(get_info());

  interface->configure(); // allow the interface to configure itself now.

  bool valid = true;

  // configure block size defaults
  set_max_block_size(128);

  // load programs-------------------------------
  QString subdir("6811/P66V6"); // the subdirectory our code is in.
  QStringList program_names = programs_in_subdir(subdir);
  for(int x=0;x<program_names.size();x++) {
    // set the entire operation as invalid if any program load fails.
    if(load_program_padded(program_names.at(x),subdir) == false) valid = false;
  }

  // override timeouts
  programs["CHECKSUM"].timeout = 8000;
  programs["VPP_APPLY"].timeout = 5000;
  programs["ERASE"].timeout = 8000;

  if(valid == false) {
    fail("Some aspects of configuration have failed.  Do not use this routine until they are resolved.");
  }

  // this preconfigures the display.  there is probably a better way.  FIXME these are totally wrong.
  emit start_program_operation(ESIDE,device_name(ESIDE),"INIT",0x10000);
  emit start_program_operation(TSIDE,device_name(TSIDE),"INIT",0x10000);
}

bool processor_p66::build_write_map(int device_id) {
  debugmsg("Building write region map for " + device_name(device_id));

  binfile *b = bin_for_device(device_id);

  int min_skip_ff_bytes = 4;

  // skip ram
  b->map.skip(0x0000,0x8000);

  // now would be a great place to skip unused bytes...
  //if(get_switch("P66_PATCH_UNUSED_REGIONS") == true) ff_unused_regions(device_id);

  // skip regions with 0xFF.
  if(get_switch("P66_SKIP_EMPTY_REGIONS") == true) b->map.skip_areas_with_byte(0xFF,min_skip_ff_bytes,b->to_bytearray());

  // complete the map.  this maps everything we haven't already mapped or ignored...
  b->map.map_all();

  if(b->map.is_complete() == false ) {
    errmsg("The mapping algorithm has failed, and the map is incomplete on " + device_name(device_id));
    return false;
  }

  debugmsg(device_name(device_id) + " mapping algorithm skipped " + QString::number(b->map.skip_count()) + " bytes.");

  return true;
}

bool processor_p66::build_read_map() {
  if(build_read_map(TSIDE) == false) return false;
  if(build_read_map(ESIDE) == false) return false;
  return true;
}

bool processor_p66::build_read_map(int device_id) {
  debugmsg("Building read region map for " + device_name(device_id));
  binfile *b = bin_for_device(device_id);
  b->map.clear();

  // read onboard eeprom
  //if(get_switch("P66_READ_ONBOARD_EEPROM") == true) b->map.append(0x0E00,512);

  // skip ram
  if(get_switch("P66_READ_RAM") == false) b->map.skip(0x0000,0x8000);

  // map remainder
  b->map.map_all();

  if(b->map.is_complete() == false) {
    errmsg("The mapping algorithm has failed, and the map is incomplete.");
    return false;
  }

  return true;
}

bool processor_p66::flash_read() {
  cancel_operation = false;
  start("P66 read procedure.");

  set_max_block_size(get_setting("P66_BLOCK_SIZE").toInt());

  if(build_read_map() == false) {
    fail("Could not build a read map.");
    return false;
  }
  if(load_kernel() == false) {
    fail("Could not connect or load the kernel.");
    return false;
  }
  if(is_cancelled()) return false;
  if(flash_read_device(TSIDE) == true) {
    store_previously_completed_bin(TSIDE);
    if(flash_read_device(ESIDE) == true) {
      store_previously_completed_bin(ESIDE);
      success("Read procedure complete.");
      emit bin_read_complete();
      unload_kernel(); // only if successful.
      interface->close_port();
      return true;
    }
  }
  fail("Read procedure failed.  Review the log.");
  unload_kernel();
  interface->close_port();
  return false;
}

bool processor_p66::flash_write() {
  if(is_cancelled()) return false; // -----safe abort point

  start("P66 write procedure");

  //------------------------------------------------------

  config_write_eside = get_switch("P66_WRITE_ESIDE");
  config_write_tside = get_switch("P66_WRITE_TSIDE");

  // this processor doesn't support checksums but leave this here in case we find a better validator later.
  if(( eside_bin.is_valid() == false && config_write_eside ) ||
     ( tside_bin.is_valid() == false && config_write_tside)) {
    fail("Loaded bin is invalid.");
    return false;
  }

  //------------------------------------------------------

  if(is_cancelled()) {
    fail("Operation cancelled.");
    return false;
  }

  //------------------------------------------------------

  eside_bin.map.clear();
  tside_bin.map.clear();

  set_max_block_size(get_setting("P66_BLOCK_SIZE").toInt());

  if(build_write_map(TSIDE) == false || build_write_map(ESIDE) == false) {
    fail("Could not build write map.");
    return false;
  }

  //------------------------------------------------------

  statusmsg("Starting P66 write procedure.");

  if(load_kernel() == false) {
    fail("Kernel could not be loaded");
    return false;
  }

  if(get_switch("P66_SELECTIVE_WRITE") == true) check_selective_write();

  if(is_cancelled()) {
    fail("Operation cancelled.");
    return false;
  }

  if(config_write_tside == true) {
    if(apply_vpp() == false) {
      fail("Could not apply VPP voltage to the flash chip.");
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
      fail("Could not apply VPP voltage to the flash chip.");
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

  success("Flash write procedure complete!");
  unload_kernel(); // only if successful.

  interface->close_port();

  return true;
}

bool processor_p66::verify_write_checksum(int device_id) {
  if(device_id != TSIDE && device_id != ESIDE) return false;

  detailmsg("Verifying checksum for " + device_name(device_id));
  binfile *b = bin_for_device(device_id);
  quint16 bin_checksum = b->generate_simple_checksum(0x8000,0xFFFF);
  quint16 ecm_checksum = get_checksum(device_id,0x8000,0xFFFF);
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

void processor_p66::store_previously_completed_bin(int device_id) {
  binfile b(bin_for_device(device_id)->to_bytearray());
  store_local_bin(device_id,"PREVIOUS",b);
}

bool processor_p66::check_selective_write() {
  if(config_write_tside == true) {
    if(has_bin_changed(TSIDE) == false) {
      statusmsg("TSIDE is unchanged since last write.  Skipping bin.");
      config_write_tside = false;
    }
  }
  if(config_write_eside == true) {
    if(has_bin_changed(ESIDE) == false) {
      statusmsg("ESIDE is unchanged since last write.  Skipping bin.");
      config_write_eside = false;
    }
  }
  return true;
}

bool processor_p66::has_bin_changed(int device_id) {
  binfile last_bin = get_local_bin(device_id,"PREVIOUS");
  if(last_bin.is_valid() == false) return true; // no previous valid bin
  binfile *this_bin = bin_for_device(device_id);

  // compare
  region compare_region(0x8000,0x8000);
  region_map compare_map(0x10000);
  compare_map.append(compare_region);
  if(this_bin->is_identical_to(last_bin,compare_map) == false) {
    detailmsg(device_name(device_id) + " does not match the new bin.");
    return true;
  }

  // verify checksum
  quint16 bin_checksum = this_bin->generate_simple_checksum(0x8000,0xFFFF);
  quint16 ecm_checksum = get_checksum(device_id,0x8000,0xFFFF);
  if(bin_checksum == ecm_checksum) return false; // unchanged according to checksum
  detailmsg(device_name(device_id) + " checksum mismatch, considering altered.");
  return true;
}

bool processor_p66::store_local_bin(int device_id, QString name, const binfile &bin_in) {
  return set_datafile(device_name(device_id) + "_" + name,"6811/P66/STORAGE",bin_in.to_bytearray());
}

binfile processor_p66::get_local_bin(int device_id, QString name) {
  binfile out(get_datafile(device_name(device_id) + "_" + name,"6811/P66/STORAGE"));
  return out;
}

quint16 processor_p66::get_checksum(int device_id, quint16 start_address, quint16 end_address) {

  // right now this routine uses a fixed checksum pattern
  Q_UNUSED(start_address);
  Q_UNUSED(end_address);

  if(device_id != TSIDE && device_id != ESIDE) return 0x0000;

  datastream_reply reply = execute_program(device_id,"CHECKSUM");

  if(reply.success == false || reply.at(0) != COMM_SUCCESS) {
    errmsg("Failed to get checksum for " + device_name(device_id));
    return 0x0000;
  }

  quint16 out = reply.at(2) & 0x00FF;
  out |= ( reply.at(1) << 8 ) & 0xFF00;
  return out;
}
