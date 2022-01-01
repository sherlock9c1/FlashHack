#ifndef USEFUL_H
#define USEFUL_H

#include <QByteArray>

class vin {
public:
  static bool validate(const QByteArray &data);
private:
  static int numerize(char in);
  static int weight(int position);
};

#endif // USEFUL_H
