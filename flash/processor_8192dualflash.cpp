#include "processor_8192dualflash.h"

#include <QDebug>
#include <QBitArray>
#include <QElapsedTimer>

#include <QFile>

#include "../datastream/datastream_8192.h"
#include "chip_id.h"

processor_8192dualflash::processor_8192dualflash() {
  kernel_version[ESIDE] = 0x00;
  kernel_version[TSIDE] = 0x00;
}

processor_8192dualflash::~processor_8192dualflash() {

}

bool processor_8192dualflash::set_x_if_y(int device_id, const region &r, unsigned char x, unsigned char y) {
  binfile *e = bin_for_device(device_id);
  if(e->is_filled(r.offset,r.size,y) == true) {
    e->fill(r.offset,r.size,x);
    debugmsg("Filled " + device_name(device_id) + " " + r.to_string() + " with " + QString::number(x,16).toUpper());
    return true;
  } else {
    debugmsg("Found data in " + device_name(device_id) + " " + r.to_string() + " - did not fill.");
    return false;
  }
}

binfile *processor_8192dualflash::bin_for_device(int device_id) {
  Q_UNUSED(device_id);
  return nullptr;
}

bool processor_8192dualflash::flash_write_device(int device_id) {
  if(device_id != TSIDE && device_id != ESIDE) return false;

  emit start_program_operation(device_id,device_name(device_id),"WRITE",0x10000);

  if(_flash_write_device(device_id) == false) {
    errmsg("We've given up trying to write the " + device_name(device_id) +
           ".. This is bad news.  If your ECM still has power, "
           "it's best to check your serial interface and try again.");
    remove_vpp();
    emit end_program_operation(device_id,device_name(device_id),"WRITE",false);
    return false;
  } else {
    emit end_program_operation(device_id,device_name(device_id),"WRITE",true);
    statusmsg(device_name(device_id) + " write complete.");
    return true;
  }
}

bool processor_8192dualflash::write_region(int device_id, const region &r) {
  QByteArray data = bin_for_device(device_id)->get_region(r);
  int offset = r.offset; // write offset.
  int length = r.size; // write length.

  if(length == 0) {
    debugmsg("Skip empty section in write_region");
    emit region_changed(device_id, offset,length,REGION_ERROR);
    return true;
  }

  if(length > 128) {
    debugmsg("Section too large to write: " + r.to_string());
    emit region_changed(device_id,offset,length,REGION_ERROR);
    return false;
  }

  // set a transitional state while we're programming.
  emit region_changed(device_id,offset,length,REGION_PROGRAMMING);

  // build write block request
  datastream_request request(device_id,COMM_EXECUTE);
  request.append16(config_write_storage_address);   // ram location to store the block
  request.append(0xBD);                             // JMP
  request.append16(config_write_subroutine_address);// .. to the exec point of the write program.
  // the rest is non-executable
  request.append16(offset);               // Programming offset for above subroutine.
  request.append(length);                 // Block size for above subroutine.
  request.append(data);

  int max_write_retries = 6;

  for(int x=0;x<max_write_retries;x++) {
    datastream_reply reply = interface->request(request);

    if(reply.success == false) {
      errmsg("Write region communication failure at block " + r.to_string());
      emit region_changed(device_id,offset,data.size(),REGION_ERROR);
    }

    unsigned char response = reply.first();

    // handle responses
    if(response == COMM_SUCCESS) {
      debugmsg("Sucessfully wrote region " + r.to_string());
      emit region_changed(device_id,offset,data.size(),REGION_WRITTEN);
      return true; // packet OK

    } else if(response == COMM_WRITE_FAIL) {
      errmsg("Write region hardware failure at block " + r.to_string());
      emit region_changed(device_id,offset,length,REGION_ERROR);

    } else {
      errmsg("Write region unknown reply at block " + r.to_string());
      emit region_changed(device_id,offset,length,REGION_ERROR);
    }
  }

  errmsg("Too many retry attempts writing block " + r.to_string() + "... giving up.");
  return false;
}

bool processor_8192dualflash::flash_read_device(int device_id) {
  binfile *b = bin_for_device(device_id);

  emit progress_changed(0); // progress bar restart
  emit start_program_operation(device_id,device_name(device_id),"READ",b->size());
  emit region_changed(device_id,0,b->size(),REGION_UNKNOWN);

  if(exec_read_program(device_id) == false) return false;

  statusmsg("Reading " + device_name(device_id));

  b->clear();

  int n_blocks = b->map.count();

  while(b->map.is_empty() == false) {
    if(is_cancelled()) break;
    region r = b->map.take_first();
    if(read_region(device_id,r) == false) return false;
    update_block_progress(n_blocks,b->map.count());
  }

  emit progress_changed(10000); // progress bar restart

  if(b->test_checksum() == false) {
    errmsg("Checksum for " + device_name(device_id) + " is invalid.");
    emit end_program_operation(device_id,device_name(device_id),"READ",false);
    return false;
  }

  emit end_program_operation(device_id,device_name(device_id),"READ",true);
  return true;
}

bool processor_8192dualflash::get_chip_id(int device_id) {
  Q_UNUSED(device_id);
  return true;
}

bool processor_8192dualflash::erase_flash(int device_id) {
  detailmsg("Uploading erase program to " + device_name(device_id));

  if(get_chip_id(device_id) == false) return false;

  // call erase subprogram
  datastream_reply reply = execute_program(device_id,"ERASE");
  bool progress_notified = false;

  QElapsedTimer t;
  t.start();

  statusmsg("Erasing flash on " + device_name(device_id));

  // the final erase program has multiple responses, so we listen to them indefinitely.
  // it also has a keepalive message of 0x55
  forever {
    // probably a comm error.
    if(reply.success == false) {
      emit region_changed(device_id,0x2000,0xDFFF,REGION_UNKNOWN);
      errmsg("Error in erase procedure: " + reply.error_message);
      return false;

    // erase ping (we are still going..)
    } else if(reply.first() == COMM_ERASE_IN_PROGRESS) {
      // only notify once
      if(progress_notified == false) {
        emit region_changed(device_id,0x2000,0xDFFF,REGION_ERASING);
        statusmsg("Erase in progress...");
        progress_notified = true;
      }

    // lots of attempts later and still no success.
    } else if(reply.first() == COMM_ERASE_FAILURE) {
      emit region_changed(device_id,0x2000,0xDFFF,REGION_ERROR);
      errmsg("Hardware error: Cannot erase flash segment!  This is bad news.");
      return false;

      // done erasing (success)
    } else if(reply.first() == COMM_ERASE_SUCCESS) {
      emit region_changed(device_id,0x2000,0xDFFF,REGION_ERASED);
      statusmsg("Erase success!");
      return true;

      // unknown response.
    } else {
      emit region_changed(device_id,0x2000,0xDFFF,REGION_ERROR);
      errmsg("Unexpected response in flash erase.");
    }

    if(t.elapsed() > 20000) {
      errmsg("Erase procedure timeout.");
      return false;
    }

    // .. get another reply
    reply = interface->get_reply(device_id,5000);
  }
}

bool processor_8192dualflash::read_region(int device_id, const region &r) {
  binfile *b = bin_for_device(device_id);

  // construct request
  datastream_request req(device_id,COMM_EXECUTE);
  req.append16(config_read_upload_location); // write packet to
  req.append16(config_read_subroutine_address);
  req.append16(r.offset);

  int n_read_retries = 6;

  // send requests with a retry loop.
  for(int x=0;x<n_read_retries;x++) {
    emit region_changed(device_id,r.offset,r.size,REGION_PROGRAMMING);
    if(is_cancelled()) return false;
    datastream_reply rep = interface->request(req);

    // handle basic reply errors
    if(rep.success == false || rep.command_id != 0x06 || rep.first() != 0xAA) {
      debugmsg("Retrying read block (reply error) at " + QString::number(r.offset,16) + " from " + device_name(device_id));
      emit region_changed(device_id,r.offset,r.size,REGION_ERROR);

    // handle invalid return offset.
    } else if(req.at(4) != rep.at(1) || req.at(5) != rep.at(2)) {
      debugmsg("Retrying read block (offset error) at " + QString::number(r.offset,16) + " from " + device_name(device_id));
      emit region_changed(device_id,r.offset,r.size,REGION_ERROR);

    // probably okay.
    } else {

      // set bytes in the bin (should be a return offset of 3 bytes (response[1] offset[2])
      // we truncate the reply to length as our read block size is fixed, but our desired regions
      // are variable.
      debugmsg("Read " + device_name(device_id) + r.to_string());
      b->set(r.offset,rep.get_data().mid(3,r.size));
      emit region_changed(device_id,r.offset,r.size,REGION_READ);
      return true;
    }
  }

  emit region_changed(device_id,r.offset,r.size,REGION_ERROR);

  errmsg("Failed to read block at " + QString::number(r.offset,16) + " from " +
         device_name(device_id) + " with " + QString::number(n_read_retries) + " attempts.");
  return false;
}



bool processor_8192dualflash::_flash_write_device(int device_id) {
   emit progress_changed(0); // progress bar restart

   if(erase_flash(device_id) == false) return false;

   if(exec_write_program(device_id) == false) return false;

   binfile *b = bin_for_device(device_id);

   statusmsg("Writing flash on " + device_name(device_id));

   // save original map size
   int n_blocks = b->map.count();

   while(b->map.is_empty() == false) {
     region r = b->map.take_first();
     if(write_region(device_id,r) == false) {
       b->map.prepend(r); // put the region back (not used yet but maybe we want some resume logic later)
       return false; // write this region, abort on fail ?
     }
     update_block_progress(n_blocks,b->map.count());
   }

   emit progress_changed(10000); // progress bar finish

   return true;
}

bool processor_8192dualflash::_unload_kernel(int device_id) {
  if(device_id != TSIDE && device_id != ESIDE) return false;

  if(is_kernel_loaded(device_id) == false) return true;

  detailmsg("Rebooting " + device_name(device_id));
  bool result = execute_simple_program(device_id,"REBOOT");

  emit region_changed(device_id,0,0x2000,REGION_UNKNOWN);

  if(result == false) {
    errmsg("Could not reboot " + device_name(device_id));
    return false;
  } else {
    statusmsg("Rebooted " + device_name(device_id));
    return true;
  }
}

bool processor_8192dualflash::is_booted(int device_id) {
  datastream_request r(device_id,0x0D);
  datastream_reply repl = interface->request(r);
  return repl.success;
}

quint16 processor_8192dualflash::get_checksum(int device_id, quint16 start_address, quint16 end_address) {
  Q_UNUSED(device_id);
  Q_UNUSED(start_address);
  Q_UNUSED(end_address);
  debugmsg("Checksum routine not implemented for this processor.");
  return 0x0000;
}

//---------------------------------------------------------
// CONFIG
//---------------------------------------------------------

QString processor_8192dualflash::device_name(int device) {
  // don't mess with this as it's used for program bin filenames.
  if(device == ESIDE) return QStringLiteral("ESIDE");
  if(device == TSIDE) return QStringLiteral("TSIDE");
  if(device == CCM) return QStringLiteral("CCM");
  return "UNKNOWN_" + QString::number(device,16).toUpper();
}

//---------------------------------------------------------
// VPP (PROGRAMMING VOLTAGE APPLY)
//---------------------------------------------------------

bool processor_8192dualflash::apply_vpp() {
  datastream_reply r = execute_program(config_vpp_device_id,"VPP_APPLY");
  if(r.success == false || r.command_id != 0x06) {
    errmsg("VPP apply program upload fail.");
    return false;
  }

  unsigned char result = r.first(); // get result byte
  if(result == COMM_SUCCESS) {
    detailmsg("Applied VPP voltage.");
    return true;
  } else if(result == COMM_VPP_IGNVOLTS_RANGE_ERROR) {
    errmsg("Ignition voltage out of range.  Please check battery connections and charge.");
    return false;
  } else if(result == COMM_VPP_VPPVOLTS_RANGE_ERROR) {
    errmsg("VPP voltage applied but out of range.  Please check battery connections and charge.");
    return false;
  } else {
    errmsg("Failed to apply VPP voltage, unknown response code.");
    return false;
  }
}

bool processor_8192dualflash::remove_vpp() {
  bool result = execute_simple_program(config_vpp_device_id,"VPP_REMOVE");
  if(result == true) {
    detailmsg("Removed VPP voltage.");
  } else {
    errmsg("Failed to remove VPP voltage for some reason.");
  }
  return result;
}

//---------------------------------------------------------
// ONBOARD EEPROM READ/WRITE
//---------------------------------------------------------

bool processor_8192dualflash::set_vin(QByteArray vin) {
  if(is_kernel_loaded(TSIDE) == true) {
    fail("Cannot program VIN while kernel is loaded.");
    return false;
  }
  if(vin.length() != 17) {
    fail("Vin must be 17 chars long.");
    return false;
  }
  start("Setting VIN number in onboard EEPROM");
  datastream_request req(TSIDE,COMM_CONFIGURE);
  req.set(0,SUBCOMM_SET_VIN);
  for(int x=0;x<17;x++) req.set(x + 1,vin.at(x));
  req.override_timeout = 1500;
  datastream_reply reply = interface->request(req);
  if(reply.success == true && reply.command_id == COMM_CONFIGURE && reply.first() == SUBCOMM_SET_VIN) {
    success("Set VIN to " + QString(vin));
  } else {
    fail("Failed to set VIN");
  }
  interface->close_port();
  return reply.success;
}

bool processor_8192dualflash::set_calid(QVariant calid) {
  if(is_kernel_loaded(TSIDE) == true) {
    errmsg("Cannot program CALID while kernel is loaded.");
    return false;
  }
  start("Setting calibration ID in onboard EEPROM");

  // validate
  bool ok;
  int calid_in = calid.toInt(&ok);
  if(ok == false) {
    fail("Calibration ID must be numeric.");
    return false;
  }

  datastream_request req(TSIDE,COMM_CONFIGURE);
  req.set(0,SUBCOMM_SET_CALID);
  req.set32(1,calid_in);
  req.override_timeout = 1500;
  datastream_reply reply = interface->request(req);
  if(reply.success == true && reply.command_id == COMM_CONFIGURE && reply.first() == SUBCOMM_SET_CALID) {
    success("Set CALID to " + QString::number(calid.toInt()));
  } else {
    fail("Failed to set CALID");
  }
  interface->close_port();
  return reply.success;
}

//---------------------------------------------------------
// FLASH READ/WRITE
//---------------------------------------------------------

bool processor_8192dualflash::exec_write_program(int device_id) {
  bool result = execute_simple_program(device_id,"WRITE");
  if(result == false) {
    errmsg("Flash write sub-routine upload failure.");
  } else {
    detailmsg("Uploaded flash write routine.");
  }
  return result;
}

bool processor_8192dualflash::exec_read_program(int device_id) {
  bool result = execute_simple_program(device_id,device_name(device_id) + "_READ");
  if(result == false) {
    errmsg("Flash read sub-routine upload failure.");
  } else {
    statusmsg("Uploaded flash read routine.");
  }
  return result;
}

void processor_8192dualflash::update_block_progress(int map_size, int blocks_remaining) {
  double percentage_remaining = ( 100.00 - ( (double)blocks_remaining / (double)map_size * 100.00) ) * 100.00;
  emit progress_changed(percentage_remaining); // update progress bar.
}

//---------------------------------------------------------
// PATCHES AND MAPPING
//---------------------------------------------------------

bool processor_8192dualflash::is_kernel_loaded(int device_id) {
  if(has_capability("HAS_KERNEL") == false) return false;
  return kernel_version.value(device_id,0) > 0;
}

bool processor_8192dualflash::_load_kernel(unsigned char device_id) {
  // this will detect an already running kernel and fill its version....
  if(enter_programming_mode(device_id) == false) return false;

  // ....because we definitely do not want to reload the kernel.  it'll crash.
  if(is_kernel_loaded(device_id) == true) return true;

  statusmsg("Uploading kernel to " + device_name(device_id));

  bool success = execute_simple_program(device_id,device_name(device_id) + "_KERNEL");
  if(success == false) {
    kernel_version[device_id] = 0;
    errmsg("Failed to load kernel for " + device_name(device_id));
    return false;
  } else {
    kernel_version[device_id] = 0x01;
    return upload_subroutines(device_id);
  }
}

bool processor_8192dualflash::upload_subroutines(unsigned char device_id) {
  if(programs.contains(device_name(device_id) + "_SUBROUTINES") == false) {
    return true; // handle no subroutines.
  }

  statusmsg("Uploading subroutines to " + device_name(device_id));

  bool success = execute_simple_program(device_id,device_name(device_id) + "_SUBROUTINES");

  if(success == false) {
    kernel_version[device_id] = 0;
    errmsg("Failed to load subroutines for " + device_name(device_id));
    return false;
  } else {
    return true;
  }
}

//---------------------------------------------------------
// ECM PROGRAM EXECUTION/HANDLING
//---------------------------------------------------------

datastream_reply processor_8192dualflash::jump_to_subroutine(int device_id, const program_definition &def) {
  datastream_request m(device_id,0x06);
  m.append16(config_default_exec_location); // address for temporary jump instruction
  m.append(0x7E); // jump
  m.append16(def.offset);
  m.override_timeout = def.timeout;
  return interface->request(m);
}

bool processor_8192dualflash::execute_simple_program(int device_id, QString program_name) {
  datastream_reply r = execute_program(device_id,program_name);

  // if failed to upload
  if(r.success == false || r.command_id != 0x06) {
    errmsg("Failed to upload program " + program_name + " to " + device_name(device_id));
    return false;
  }

  // just return a yes or no answer.
  unsigned char result = r.first(); // get result byte
  if(result == COMM_SUCCESS) {
    debugmsg("Executed " + program_name + " on " + device_name(device_id));
    return true;
  } else {
    errmsg("Simple program " + program_name + " to " + device_name(device_id) +
             " replied with unexpected success code " + QString::number(result,16));
    return false;
  }
}

datastream_reply processor_8192dualflash::execute_program(int device_id, QString program_name) {
  if(programs.contains(program_name) == false) {
    errmsg("Cannot upload code: program " + program_name + " is not loaded.");
    return datastream_reply::fail("Program not loaded."); // program
  }

  program_definition def = programs[program_name];

  if(def.program_type == program_definition::PROGRAM_NORMAL) {
    debugmsg("Executing prepared program " + program_name + " on " + device_name(device_id));
    return _execute_program(device_id,programs[program_name]);
  } else {
    debugmsg("Executing subroutine " + program_name + " on " + device_name(device_id));
    return jump_to_subroutine(device_id,programs[program_name]);
  }
}

datastream_reply processor_8192dualflash::_execute_program(unsigned char device, const program_definition &def) {
  // this code can be prepended to a mode 6 request to make it not actually execute (jumps back
  // to our mode 5 loop).
  // one goal of this function is to seamlessly load a program regardless of how much larger or smaller
  // it is than the maximum ALDL packet size.

  // select proper loader for eeprom or kernel execution.
  QByteArray loader;
  if(is_kernel_loaded(device) == false) {
    loader = programs["LOADER_EEPROM"].data;
  } else {
    loader = programs["LOADER_KERNEL"].data;
  }

  QByteArray code = def.data;
  unsigned int offset = def.offset;

  // constant packet size limitation for execution.  8 bits minus 0x52 (aldl protocol limit) minus
  // 6 bytes of overhead.
  const int max_packet_size = ( 0xFF - 0x52 ) - 0x06; // how much we can load and execute in one shot
  const int max_packet_nx_size = max_packet_size - loader.size(); // .. or not execute,

  // we now load, backwards, any code that we have to due to packet size limitations.  this routine is
  // never executed for a routine smaller than max size.
  while(code.size() > max_packet_size) {

    // get the right side of the data array and remove it from the original array.
    QByteArray chunk = code.right(max_packet_nx_size);
    code.chop(max_packet_nx_size);

    // start a new program segment with the load extension.
    QByteArray out(loader);

    // append the payload itself.
    out.append(chunk);

    // calculate a new offset that places that code payload at the desired location considering
    // the presense of our new header
    unsigned int new_offset = offset + code.size() - loader.size();

    // load this code, completely bail on failure.
    datastream_reply r = upload_and_execute(device, new_offset, out, -1);
    if(r.success == false) {
      return r;
    }
    if(r.first() != COMM_SUCCESS) {
      r.success = false;
      return r;
    }
  }

  // now load the first (or only) chunk normally, and execute it.
  return upload_and_execute(device,offset,code,def.timeout);
}

datastream_reply processor_8192dualflash::upload_and_execute(unsigned char device, quint16 offset, QByteArray code, int timeout) {
  // empty code will crash the ECM
  if(code.isEmpty()) return datastream_reply::fail("Empty code section in upload_and_execute");

  // prepend the memory location to execute from.
  code.prepend(datastream_message::n_to_array16(offset));

  // create the request itself
  datastream_request request(device,COMM_EXECUTE);
  request.set_data(code);
  request.override_timeout = timeout;

  // submit the request, and get the reply.
  datastream_reply reply = interface->request(request);
  if(reply.success == false) {
    return reply;
  }

  // the ECM replies with the unlock command code if we are not unlocked.
  if(reply.command_id == COMM_UNLOCK) {
    // ... so unlock and try again.
    if(enter_programming_mode(device) == true) {
      reply = interface->request(request);
    } else {
      return datastream_reply::fail("Could not enter programming mode.");
    }
  }

  if(reply.success == true) emit region_changed(device,offset,code.size(),REGION_EXEC);

  return reply;
}

//---------------------------------------------------------
// PROGRAMMING/UNLOCK MODES
//---------------------------------------------------------

bool processor_8192dualflash::enter_programming_mode(unsigned char device) {
  statusmsg("Entering programming mode on " + device_name(device));
  // request
  datastream_request request(device,COMM_PROGRAM);
  datastream_reply reply = interface->request(request);
  if(reply.success == false) {
    errmsg("Failed to enter programming mode for " + device_name(device) + ": Communication problem");
    return false;
  }

  kernel_version[device] = 0; // assume no kernel loaded ...

  // check response
  if(reply.first() == COMM_SUCCESS) {
    return true;
  } else if(reply.command_id == COMM_KERNEL) {
    detailmsg("Detected already running kernel for " + device_name(device));
    kernel_version[device] = (unsigned char)reply.at(0);
    return true;
  } else if(reply.command_id == COMM_UNLOCK) {
    if(unlock_security(device) == true) {
      reply = interface->request(request);
      if(reply.first() == COMM_SUCCESS) {
        return true;
      } else {
        errmsg("Failed to place " + device_name(device) + " in programming mode");
        return false;
      }
    } else {
      errmsg("Failed to place " + device_name(device) + " in programming mode (cannot disable security)");
    }
  } else {
    errmsg("Unknown response when entering programming mode.");
  }
  return false;
}

bool processor_8192dualflash::unlock_security(unsigned char device) {
  detailmsg("Unlocking " + device_name(device));
  int retry_attempts = 10;
  for(int x=0;x<retry_attempts;x++) {
    if(_unlock_security(device) == true) {
      return true;
    }
    interface->sleep_ms(1000);
  }
  return false;
}

bool processor_8192dualflash::_unlock_security(unsigned char device) {
  // 00 TIMER ACTIVE
  // AA SUCCESS
  // CC KEY INCORRECT
  // 05 TOO MANY KEY REQUESTS.

  // request
  datastream_request request(device,COMM_UNLOCK);

  // send request for key.
  request.set_first(SUBCOMM_GET_CHALLENGE);
  datastream_reply reply = interface->request(request);
  if(reply.success == false) return false;

  // expect a payload of 3 bytes.
  if(reply.size() != 3) return false;

  // the reply should contain COMM_GET_CHALLENGE just like our request.
  if(reply.first() != request.first()) return false;

  // forumulate response to challenge....
  request.set_first(SUBCOMM_SEND_RESPONSE);

  if(reply.get16(1) == 0x0000) {
    // 0x0000 seems to mean already unlocked, we can reply with 0x0000 to make sure.
    request.set16(1,0x0000);
  } else {
    // ... just perform a binary NOT on the two 'challenge' bytes from the above reply.
    for(int x=1;x<=2;x++) request.set(x,~ reply.at(x));
  }

  // send response
  reply = interface->request(request);
  if(reply.success == false) return false;

  // 0xAA means we win and the thing should be unlocked....
  if(reply.at(1) == COMM_SUCCESS) return true;

  // otherwise fail.
  return false;
}

