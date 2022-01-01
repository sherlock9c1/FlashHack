#include "region_map.h"

#include <QDebug>

// this slows down mapping but tries to make sure we aren't double mapping regions or anything.
#define DEBUG_MAPPING

region::region(unsigned int offset, unsigned int length) {
  this->offset = offset;
  this->size = length;
}

region::region() {
  offset = 0;
  size = 0;
}

unsigned int region::last() const {
  return offset + size - 1;
}

bool region::operator < (const region &r) {
  if(last() < r.offset) return true;
  return false;
}

bool region::operator > (const region &r) {
  if(offset > r.last()) return true;
  return false;
}

bool region::operator ==(const region &r) {
  if(offset == r.offset && r.last() == last()) return true;
  return false;
}

bool region::is_valid() const {
  if(size == 0) return false;
  return true;
}

QString region::to_string() const {
  return QString::number(offset,16).toUpper() + "[" + QString::number(size,16).toUpper() + "]";
}

bool region::contains(int offset, int size) const {
  return contains(region(offset,size));
}

bool region::contains(const region &r) const {
  if(r.offset >= offset && r.last() <= last()) return true;
  return false;
}

bool region::is_inside(const region &r) const {
  return r.contains(offset,size);
}

region_map::region_map(int bin_size) {
  resize(bin_size);
}

region_map::region_map() {

}

void region_map::clear() {
  resize(_bin_size);
}

void region_map::set_max_block_size(int size) {
  max_block_size = size;
}

void region_map::resize(int bin_size) {
  this->_bin_size = bin_size;
  log.clear();
  log.resize(bin_size);
  list.clear();
}

int region_map::mapped_bytes() const {
  int total = 0;
  for(int x=0;x<list.size();x++) total += at(x).size;
  return total;
}

int region_map::count() const {
  return list.size();
}

int region_map::bin_size() const {
  return _bin_size;
}

void region_map::append(int offset, int size) {
  int max_byte = offset + size - 1; // the last possible byte we can map.

  // limits.
  if(_bin_size < 1) return;
  if(size < 1) return;
  if(max_byte > _bin_size - 1) max_byte = _bin_size - 1;
  if(max_byte < offset) return;

  // determine our actual first byte that's not on an existing region.
  while(offset < max_byte && log.at(offset) == true) offset++;

  int max_block_byte = offset + max_block_size - 1;
  if(max_block_size < 1) max_block_byte = max_byte;

  // handle nothing to map condition.
  if(log.at(offset) == true) return;

  // now we have at least one byte.  lets determine our actual end point.  if we are less than maximum but our
  // next byte is not set, then we can advance the cursor.
  int last_byte = offset;
  while(last_byte < max_byte && last_byte < max_block_byte && log.at(last_byte + 1) == false) last_byte++;

  int block_size = last_byte - offset + 1;

  // update the log
  for(int x=0;x<block_size;x++) {
    log.setBit(offset + x);
  }

  // add the region
  list.append(region(offset,block_size));

  // recurse if required to complete remainder.
  if(size - block_size > 0) append(offset + block_size, size - block_size);
}

void region_map::append(const region &r) {
  append(r.offset,r.size);
}

void region_map::append(const region_map &m) {
  for(int x=0;x<m.list.size();x++) {
    append(m.at(x));
  }
}

void region_map::skip(int offset, int size) {
  for(int x=0;x<size;x++) log.setBit(offset + x);
}

void region_map::unskip(int offset, int size) {
  // FIXME check range here
  for(int x=0;x<size;x++) log.clearBit(offset + x);
}

void region_map::map_all() {
  append(0,_bin_size);
}

region region_map::take_first() {
  if(list.isEmpty() == true) return region();
  return list.takeFirst();
}

void region_map::prepend(const region &r) {
  list.prepend(r);
}

region region_map::at(int position) const {
  return list.at(position);
}

bool region_map::is_empty() const {
  return list.isEmpty();
}

void region_map::sort() {
  std::sort(list.begin(),list.end());
}

void region_map::skip_areas_with_byte(unsigned char skip_byte, int min_section_length, const QByteArray &data) {
  map_areas_with_byte(skip_byte,min_section_length,data,true);
}

void region_map::map_areas_with_byte(unsigned char skip_byte, int min_section_length, const QByteArray &data, bool skip_not_map) {
  int count = 0; // tmp
  int x = 0;
  while(x < _bin_size) {
    // handle already mapped or non skip bytes.
    if(log.at(x) == true || (unsigned char)data.at(x) != skip_byte) {
      x++;
      continue;
    }
    // region is now on a skip byte, find the length of the region.
    int y = x;
    while(y < _bin_size && (unsigned char)data.at(y) == skip_byte) y++;
    int region_size = y - x;
    if(region_size >= min_section_length) {
      if(skip_not_map == true) {
        skip(x,region_size);
      } else {
        append(x,region_size);
      }
      count += region_size;
    }
    x += region_size;
  }
}

QString region_map::to_string() const {
  QString out;
  for(int x=0;x<list.size();x++) {
    out.append(list.at(x).to_string() + '\n');
  }
  return out;
}

bool region_map::is_complete() const {
  for(int x=0;x<log.size();x++) if(log.at(x) == false) return false;
  return true;
}

bool region_map::is_actually_complete() const {
  return mapped_bytes() == bin_size();
}

int region_map::skip_count() const {
  return bin_size() - mapped_bytes();
}

