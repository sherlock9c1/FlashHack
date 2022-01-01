#include "binfile.h"
#include <QDebug>
#include <QFile>

binfile::binfile() {

}

binfile::binfile(unsigned int bin_size) {
  resize(bin_size);
}

binfile::binfile(const QByteArray &data_in) {
  resize(data_in.size());
  data = data_in;
}

//----------------------------------------------


QByteArray binfile::get_region(const region &r) {
  return data.mid(r.offset,r.size);
}

binfile::binfile(QString path) {
  QFile f(path);
  if(f.open( QIODevice::ReadOnly ) == false) {
    data.clear();
    return;
  }
  QByteArray b = f.readAll();
  f.close();
  resize(b.size());
  data = b;
}

//-------------------------------------------------

void binfile::resize(int bin_size) {
  data.resize(bin_size);
  map.resize(size());
  data.fill(0x00);
}

void binfile::clear() {
  data.fill(0x00);
}

void binfile::clear(const region &r) {
  if(r.is_valid() == false) {
    qDebug() << "CLEAR REGION ERROR: REGION OUT OF RANGE";
    return;
  }
  data.fill(r.offset,r.size);
}

void binfile::clear(unsigned int position) {
  if(position_valid(position) == false) return;
  data[position] = 0x00;
}

//------------------------------------------------

int binfile::size() const {
  return data.size();
}

bool binfile::position_valid(const int offset) {
  if(offset < 0) {
    qDebug() << "offset" << offset << " < 0 !!";
    return false;
  }
  if(offset > size() - 1) {
    qDebug() << "offset" << offset << " > bin_size" << size();
    return false;
  }
  return true;
}

bool binfile::region_valid(const int offset, const int _size) {
  if(position_valid(offset) == false) return false; // first byte
  if(position_valid(offset + _size - 1) == false) return false;
  return true;
}

bool binfile::region_valid(const region &r) {
  return region_valid(r.offset,r.size);
}

bool binfile::is_empty() {
  for(int x=0;x<size();x++) if(get(x) != 0x00) return false;
  return true;
}

bool binfile::is_valid() {
  if(is_empty() == true) return false;
  return test_checksum();
}

bool binfile::get_bit(int offset, unsigned int bit) {
  return get(offset) & ( 1 << bit);
}

void binfile::set_bit(int offset, unsigned int bit, bool setting) {
  if(setting == true) {
    set(offset,get(offset) | ( 1 << bit));
  } else {
    set(offset,get(offset) & ~ ( 1 << bit));
  }
}

//------------------------------------------------

byte binfile::get(int offset) {
  if(position_valid(offset) ==  false) return 0x00;
  return (unsigned char)data.at(offset);
}

void binfile::set(int offset, byte b) {
  if(position_valid(offset) ==  false) return;
  data[offset] = b;
}

void binfile::set(int offset, const QByteArray &in) {
  for(int x=0;x<in.size();x++) set(offset + x,in[x]);
}

bool binfile::is_filled(int offset, int size, byte b) {
  if(region_valid(offset,size) == false) return false; // check range
  for(int x=0;x<size;x++) if((unsigned char)data.at(offset + x) != b) return false;
  return true;
}

void binfile::fill(int offset, int size, byte b) {
  if(region_valid(offset,size) == false) return;
  for(int x=0;x<size;x++) set(offset + x,b);
}

QString binfile::get_ascii(int offset, int size) {
  if(region_valid(offset,size) == false) return QString();
  QString out;
  for(int x=0;x<size;x++) out.append(get(offset+ x));
  return out;
}

void binfile::patch(const QByteArray &data, int size, int offset) {
  if(region_valid(size,offset) == false) return;
  for(int x=0;x<size;x++) set(offset + x,data.at(x));
  reset_checksum();
}

void binfile::patch(const QByteArray &data, const region &r) {
  patch(data,r.size,r.offset);
}

void binfile::patch(unsigned char *patch_data, int size, int offset) {
  if(region_valid(offset,size) == false) return;
  for(int x=0;x<size;x++) set(offset + x,patch_data[x]);
  reset_checksum();
}

void binfile::set_16(int offset, unsigned int n) {
  set(offset,( n & 0xFF00 ) >> 8);
  set(offset + 1,n & 0xFF);
}

unsigned int binfile::get_16(int offset) {
  unsigned int out = get(offset + 1) | (get(offset) << 8);
  out &= 0xFFFF;
  return out;
}

unsigned int binfile::get_32(int offset) {
  unsigned int out = get(offset + 3) | (get(offset + 2) << 8) | (get(offset + 1) << 16) | (get(offset) << 24);
  out &= 0xFFFFFFFF;
  return out;
}

//------------------------------------------------

bool binfile::test_checksum() {
  unsigned int desired_checksum = generate_checksum();
  unsigned int actual_checksum = current_checksum();
  if(desired_checksum == actual_checksum) return true;
  return false;
}

void binfile::reset_checksum() {
  qDebug() << "RESET CHECKSUM FUNCTION NOT IMPLEMENTED";
}

unsigned int binfile::generate_checksum() {
  qDebug() << "GENERATE CHECKSUM FUNCTION NOT IMPLEMENTED";
  return 0;
}

unsigned int binfile::current_checksum() {
  qDebug() << "GET CHECKSUM FUNCTION NOT IMPLEMENTED";
  return 0;
}

//------------------------------------------------

void binfile::clear_unused_regions() {
  // does nothing by default...override me
}

//------------------------------------------------

QByteArray binfile::to_bytearray() const {
  return data;
}

bool binfile::from_byte_array(const QByteArray &data) {
  if(data.size() != size()) {
    qDebug() << "INPUT DATA IS INCORRECT SIZE IN from_byte_array()";
    return false;
  }
  this->data = data;
  return true;
}

//------------------------------------------------

bool binfile::is_identical_to(const binfile &other, const region_map &m, bool ff_equals_zero) {
  Q_UNUSED(ff_equals_zero);
  if(other.size() != size()) return false;
  if(m.bin_size() != size()) return false;
  for(int x=0;x<m.count();x++) {
    region r = m.at(x);
    if(region_valid(r) == false) return false;
    for(unsigned int y=0;y<r.size;y++) {
      if(data[r.offset + y] != other.data[r.offset + y]) {
        qDebug() << "found differing data at " << QString::number(r.offset + y,16) << ":"
                  <<  QString::number((unsigned char)data[r.offset + y],16)
            << QString::number((unsigned char)other.data[r.offset + y],16);
        return false;
      }
    }
  }
  return true;
}

//------------------------------------------------

unsigned int binfile::swap_bits(unsigned int n, unsigned int p1, unsigned int p2) {
  return (((n >> p1) & 1) == ((n >> p2) & 1) ? n : ((n ^ (1 << p2)) ^ (1 << p1)));
}

//-------------------------------------------


unsigned int binfile::generate_simple_checksum(int start, int end) {
  unsigned int sum = 0;
  for(int x=start;x<=end;x++) {
    unsigned char b = get(x);
    sum += b;
  }
  sum &= 0xFFFF;
  return sum;
}

unsigned int binfile::generate_simple_checksum(int start, int end, const QByteArray &d) {
  unsigned int sum = 0;
  for(int x=start;x<=end;x++) {
    unsigned char b = (unsigned char)d.at(x);
    sum += b;
  }
  sum &= 0xFFFF;
  return sum;
}
