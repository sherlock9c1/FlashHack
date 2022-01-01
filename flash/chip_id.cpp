#include "chip_id.h"

QString flash_chip_name(int mfr, int chip_id) {
  QString out;

  switch(mfr) {
    case 0x89:
      out.append("INTEL ");
      switch(chip_id) {
      case 0xB8:
        out.append("28F512");
        break;
      case 0xB9:
        out.append("28F256A");
        break;
      default:
        out.append(QString::number(chip_id,16));
        break;
      }

    break;
  default:
    out.append("UNKNOWN ");
    out.append(QString::number(chip_id,16));
    break;
  }
  return out;
}
