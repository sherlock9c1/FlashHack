#include <QDebug>
#include <QFile>
#include <QCryptographicHash>
#include "binfile_ee.h"

bin_ee::bin_ee() : binfile(EE_ROM_SIZE) {

}

bin_ee::bin_ee(const QByteArray &data) : binfile(data) {

}

void bin_ee::clear_unused_regions() {
  // there is a small 68k onboard eeprom on the 68hc11 that is adressed in the middle of ram space.
  // it runs from 0x0E00 to 0x1000.  need to skip that.
  clear(region(0x0000,0x0E00));
  clear(region(0x1001,0x0FFF));
}

bool bin_ee::verify_pointers() {
  quint16 reset_vector = get_16(0xFFFE);
  if(reset_vector > 0xFFFE) return false;
  if(reset_vector < 0x2000) return false;
  return true;
}

unsigned int bin_ee::generate_checksum() {
  return generate_simple_checksum(EE_CHECKSUM_START,EE_ROM_SIZE - 1);
}

unsigned int bin_ee::current_checksum() {
  return get_16(EE_CHECKSUM_LOCATION);
}

void bin_ee::reset_checksum() {
  unsigned int sum = generate_checksum();
  set_16(EE_CHECKSUM_LOCATION,sum);
}

//-------------------------------------------

QByteArray bin_ee::convert_raw(QByteArray in) {
  QByteArray out(in.size(),0x00);
  for(int x=0;x<in.size();x++) out[unscramble_ee_address(x)] = in.at(x);
  return out;
}

void bin_ee::from_raw(QByteArray in) {
  from_byte_array(convert_raw(in));
}

QByteArray bin_ee::to_raw() {
  return convert_raw(to_bytearray());
}

unsigned int bin_ee::unscramble_ee_address(unsigned int a) {
  // the onboard eeprom has the following address pin scrambling:
  // 13 to 1, 12 to 2, 11 to 1
  unsigned int out = a;
  out = swap_bits(out,13,1);
  out = swap_bits(out,12,2);
  out = swap_bits(out,11,3);
  return out;
}

//-----------------------------------------


binfile_ee::binfile_ee() {

}

binfile_ee::binfile_ee(QByteArray data_in, binfile_ee::ee_format ftype) {
  load_data(data_in,ftype);
}

bool binfile_ee::is_valid() {
  if(eside.is_valid() == false) return false;
  if(tside.is_valid() == false) return false;
  if(is_rev_a() == false && is_rev_b() == false) return false;
  if(tside.verify_pointers() == false) return false;
  if(eside.verify_pointers() == false) return false;
  return true;
}

bool binfile_ee::load_file(QString path, binfile_ee::ee_format ftype) {
  QFile f(path);
  if(f.open( QIODevice::ReadOnly ) == false) return false;
  QByteArray data = f.readAll();
  f.close();
  return load_data(data,ftype);
}

bool binfile_ee::save_file(QString path, binfile_ee::ee_format ftype) {
  QFile f(path);
  if(f.open( QIODevice::WriteOnly ) == false) return false;
  if(f.write(save_data(ftype)) == false) return false;
  f.close();
  return true;
}

bool binfile_ee::load_data(QByteArray data, binfile_ee::ee_format ftype) {
  switch(ftype) {
  case FORMAT_LT1EDIT:
    if(data.size() != EE_ROM_SIZE * 2) return false;
    eside.from_byte_array(data.mid(0,EE_ROM_SIZE));
    tside.from_byte_array(data.mid(EE_ROM_SIZE,EE_ROM_SIZE));
    return true;
    break;

  case FORMAT_TUNERCAT:
    if(data.size() != EE_ROM_SIZE * 2) return false;
    tside.from_byte_array(data.mid(0,EE_ROM_SIZE));
    eside.from_byte_array(data.mid(EE_ROM_SIZE,EE_ROM_SIZE));
    return true;
    break;

  case FORMAT_ESIDE:
    if(data.size() != EE_ROM_SIZE) return false;
    eside.from_byte_array(data);
    return true;
    break;

  case FORMAT_TSIDE:
    if(data.size() != EE_ROM_SIZE) return false;
    tside.from_byte_array(data);
    return true;
    break;

  case FORMAT_RAW_ESIDE:
    if(data.size() != EE_ROM_SIZE) return false;
    eside.from_raw(data);
    return true;
    break;

  case FORMAT_RAW_TSIDE:
    if(data.size() != EE_ROM_SIZE) return false;
    tside.from_raw(data);
    return true;
    break;
  }

  return false;
}

QByteArray binfile_ee::save_data(binfile_ee::ee_format ftype) {
  QByteArray out;
  switch(ftype) {
  case FORMAT_LT1EDIT:
    out.append(eside.to_bytearray());
    out.append(tside.to_bytearray());
    break;
  case FORMAT_TUNERCAT:
    out.append(tside.to_bytearray());
    out.append(eside.to_bytearray());
    break;
  case FORMAT_RAW_ESIDE:
    out = eside.to_raw();
    break;
  case FORMAT_RAW_TSIDE:
    out = tside.to_raw();
    break;
  case FORMAT_ESIDE:
    out = eside.to_bytearray();
    break;
  case FORMAT_TSIDE:
    out = tside.to_bytearray();
    break;
  }
  return out;
}

QString binfile_ee::bin_info() {
  QString out;
  out.append("MAF=" + QString::number(is_maf_enabled(),2));
  out.append(" AUTO_TRANS=" + QString::number(is_auto_transmission(),2));
  out.append(" VIN=" + get_vin());
  out.append(" CAL_ID=" + QString::number(get_cal_id()));
  out.append(" PROMID_T=" + QString::number(get_prom_id_tside(),16).toUpper());
  out.append(" PROMID_E=" + QString::number(get_prom_id_eside(),16).toUpper());
  if(is_rev_b() == true) out.append(" REV_B");
  return out;
}

bool binfile_ee::is_maf_enabled() {
  return tside.get_bit(0x2028,1);
}

bool binfile_ee::is_auto_transmission() {
  return tside.get_bit(0x2026,6);
}

QString binfile_ee::get_vin() {
  return tside.get_ascii(0x0E24,17);
}

int binfile_ee::get_cal_id() {
  return tside.get_32(0x0E20);
}

int binfile_ee::get_prom_id_tside() {
  return tside.get_16(0x2017);
}

int binfile_ee::get_prom_id_eside() {
  return eside.get_16(0x2017);
}

bool binfile_ee::is_rev_b() {
  if(eside.get(0xFFFD) != 0x4D) return false;
  if(eside.get(0xFFFF) != 0x53) return false;
  return true;
}

bool binfile_ee::is_rev_a() {
  if(eside.get(0xFFFD) != 0xC7) return false;
  if(eside.get(0xFFFF) != 0xCD) return false;
  return true;
}
