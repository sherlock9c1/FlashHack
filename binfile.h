#ifndef BINFILE_H
#define BINFILE_H

#include <QByteArray>
#include <QVector>
#include <QBitArray>
#include "region_map.h"

typedef unsigned char byte; // use this for all single byte operations with the eeprom

class binfile {
public:
  binfile(unsigned int bin_size); // initialize a new generic eeprom container of bin_size
  binfile(const QByteArray &data_in); // .. with data
  binfile(QString path); // from file
  binfile(); // must be resized before use

  // ------------------
  region_map map;
  // ------------------
  void clear(); // zero the entire thing.
  void clear(const region &r); // zero a region
  void clear(unsigned int position); // zero one byte

  int size() const; // size of the eeprom

  bool is_empty();
  bool is_valid();

  unsigned int offset = 0; // offset to write this bin with (optional)

  // single bit operators
  bool get_bit(int offset, unsigned int bit);
  void set_bit(int offset, unsigned int bit, bool setting);

  // 8 bit operators
  byte get(int offset);
  void set(int offset, byte b);

  // 16 bit operators
  void set_16(int offset, unsigned int n);
  unsigned int get_16(int offset);
  unsigned int get_32(int offset);

  // array operators
  void patch(const QByteArray &data, int size, int offset); // overwrite a chunk of bin.
  void patch(const QByteArray &data, const region &r); // overwrite a chunk of bin.
  void patch(unsigned char *patch_data, int size, int offset); // overwrite a chunk of bin.
  void set(int offset, const QByteArray &in);
  bool is_filled(int offset, int size, byte b); // is filled with b
  void fill(int offset, int size, byte b);
  QString get_ascii(int offset, int size);

  // ------------------
  bool test_checksum();
  virtual void reset_checksum();
  virtual unsigned int generate_checksum();
  virtual unsigned int current_checksum();
  virtual void clear_unused_regions();

  // simple checksums
  unsigned int generate_simple_checksum(int start, int end);
  static unsigned int generate_simple_checksum(int start, int end, const QByteArray &d);

  // to work with QT stuff.
  QByteArray to_bytearray() const;
  bool from_byte_array(const QByteArray &data);

  // useful bit manipulation functions--------------------------------------------
  static unsigned int swap_bits(unsigned int n, unsigned int p1, unsigned int p2); // swap the bits at p1 and p2

  QByteArray get_region(const region &r);

  bool is_identical_to(const binfile &other, const region_map &m, bool ff_equals_zero = false);
  void resize(int bin_size);

  // bounds checking
  bool position_valid(const int offset);
  bool region_valid(const int offset, const int _size); // ensure the data is in range.
  bool region_valid(const region &r);

protected:

  QByteArray data;

};

#endif // BINFILE_H
