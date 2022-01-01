#ifndef MAP_H
#define MAP_H

#include <QVector>
#include <QString>
#include <QBitArray>

class region {
public:
  region(unsigned int offset, unsigned int size);
  region();

  unsigned int offset;
  unsigned int size;
  unsigned int last() const;

  bool operator < (const region &r);
  bool operator > (const region &r);
  bool operator == (const region &r);

  bool is_valid() const;

  QString to_string() const;

  bool contains(const region &r) const;
  bool is_inside(const region &r) const;
  bool contains(int offset, int size = 1) const;
};

class region_map {
public:
  region_map(int _bin_size);
  region_map();

  void clear();
  void resize(int _bin_size);

  int bin_size() const;
  int mapped_bytes() const;

  void set_max_block_size(int size); // adding regions larger than this will split and recurse.

  void append(int offset, int size);
  void append(const region &r);
  void append(const region_map &m);
  void prepend(const region &r);
  void skip(int offset, int size); // consider a region mapped
  void unskip(int offset, int size); // consider a region unmapped
  void map_all();  // recursively map everything not mapped already.

  int count() const;
  region take_first();
  region at(int position) const;
  bool is_empty() const;

  void sort();

  QString to_string() const; // dump all regions offsets/sizes for debugging

  bool is_complete() const; // is everything mapped
  bool is_actually_complete() const; // is everything actually mapped (and none skipped)
  int skip_count() const; // how many bytes have we skipped

  // skip or map regions filled with a byte of min_section_length
  void skip_areas_with_byte(unsigned char skip_areas_with_byte, int min_section_length, const QByteArray &data);
  void map_areas_with_byte(unsigned char skip_areas_with_byte, int min_section_length, const QByteArray &data, bool skip_not_map = false);

private:

  QBitArray log;
  QVector <region>list;

  int _bin_size = 0;
  int max_block_size = 0;
};

#endif // MAP_H
