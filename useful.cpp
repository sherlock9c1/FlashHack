#include "useful.h"

bool vin::validate(const QByteArray &data) {
  if(data.size() != 17) return false;
  if(data == QByteArray("11111111111111111")) return false;

  // add and check each digit
  int sum = 0;
  for(int x=0;x<17;x++) {
    int w = weight(x);
    int n = numerize(data.at(x));
    if(n == -1 || w == -1) return false;
    sum += ( w * n );
  }

  // calculate
  int calculated_checksum = sum % 11;
  int vin_checksum = numerize(data.at(8));

  // validate
  if(calculated_checksum == 10 && data.at(8) == 'X') return true;
  if(calculated_checksum == vin_checksum) return true;

  // fail
  return false;
}

inline int vin::numerize(char in) {
  const char char_table[34] = {"1234567890ABCDEFGHJKLMNPRSTUVWXYZ"}; // has LF ending
  const int n_table[33] = {1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6,7,8,1,2,3,4,5,7,9,2,3,4,5,6,7,8,9};
  for(int x=0;x<33;x++) {
    if(char_table[x] == in) return n_table[x];
  }
  return -1; // err
}

inline int vin::weight(int position) {
  const int weight_table[17] = {8,7,6,5,4,3,2,10,0,9,8,7,6,5,4,3,2};
  if(position < 0 || position >= 17) return -1;
  return weight_table[position];
}
