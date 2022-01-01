#include "processor_obdii.h"

processor_obdii::processor_obdii() {

}

QString processor_obdii::get_name() {
  return "OBDII Generic";
}

QString processor_obdii::get_info() {
  return "This is a processor for testing and development of generic OBD-II devices.";
}

void processor_obdii::configure() {
  statusmsg("Configured processor " + get_name() + " with interface " + interface->interface_name());
  statusmsg(get_info());
  fail("This processor doesn't actually do anything yet.  Pressing buttons would be bad.\n"
       "Please change processors and try again in a few versions.");
  interface->configure(); // allow the interface to configure itself now.
}

void processor_obdii::dbg_a() {
  interface->open_port();
  interface->close_port();
}

void processor_obdii::dbg_b() {

}

void processor_obdii::dbg_c() {

}
