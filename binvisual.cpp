#include "binvisual.h"
#include <QPainter>
#include <QDebug>
#include <cmath>

binvisual::binvisual(QWidget *parent) : QWidget(parent) {

}

QColor binvisual::region_state_color(const _program_region_state region_state) {
  switch(region_state) {
  case REGION_ERROR:
    return QColor(255,0,0);
    break;
  case REGION_ERASING:
    return QColor(68,28,227);
    break;
  case REGION_ERASED:
    return QColor(88,102,67);
    break;
  case REGION_UNKNOWN:
    return QColor(38,43,79);
    break;
  case REGION_WRITTEN:
    return QColor(79,225,30);
    break;
  case REGION_READ:
    return QColor(250,35,244);
    break;
  case REGION_PROGRAMMING:
    return QColor(217,255,0);
    break;
  case REGION_EXEC:
    return QColor(230,204,26);
    break;
  default:
    return QColor(0x000001);
  }
}

void binvisual::paintEvent(QPaintEvent *event) {
  Q_UNUSED(event);

  // this iteratively maps a an array of blocks into a square matrix,
  // then scales its aspect ratio and draws it.  think of the windows
  // defragmenter display.

  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing,true);

  int w = width();
  int h = height();
  int count = bin_size();

  // blank with 'unknown'
  painter.fillRect(0,0,w,h,QBrush(region_state_color(REGION_UNKNOWN)));
  if(map.contains(device_id) == false) return; // invalid device

  // size of the square matrix
  int bin_square = sqrt(count);

  // display aspect ratios for mapping the square matrix onto our display surface.
  double width_aspect = (double)w / (double)bin_square;
  double height_aspect = (double)h / (double)bin_square;

  // block drawing size.  we round that float up a bit so the blocks can overlap a little.
  double pixel_width = width_aspect * 1.1;
  double pixel_height = height_aspect * 1.1;

  // floor pixel width to 1.1
  if(pixel_width < 1.1) pixel_width = 1.1;
  if(pixel_height < 1.1) pixel_height = 1.1;

   // track current state for pen switching
  _program_region_state current_state = REGION_UNKNOWN;

  // iterate every pixel.
  for(int y=0;y<bin_square;y++) {
    for(int x=0;x<bin_square;x++) {

      // calculate current cell.
      int cell = ( y * bin_square ) + x;

      // handle overrun
      if(cell > count - 1) continue;

      // handle pen changes
      _program_region_state new_state = map[device_id].at(cell);
      if(new_state != current_state) {
        painter.setBrush(QBrush(region_state_color(new_state),Qt::SolidPattern));
        painter.setPen(region_state_color(new_state));
        current_state = new_state;
      }

      // skip areas already blanked .
      if(new_state == REGION_UNKNOWN) continue;

      // optimize for width
      int r_width = 1;
      while(x + r_width < bin_square &&
           map[device_id].at(cell + r_width) == current_state) r_width++;

      // draw as a set of rectangles with aspect transformation.
      painter.fillRect(QRectF(x * width_aspect, y * height_aspect,
                              pixel_width * r_width, pixel_height),
                       painter.brush());

      // advance the extra width if there is any.
      x += r_width - 1;
    }
  }

  // draw transparent FLASHHACK
  painter.setPen(QColor(255,255,255,100));

  QFont f("Arial Black",20);
  QFontMetrics metric(f);
  painter.setFont(f);

  QPoint text_point = rect().center();
  text_point.setY(text_point.y());
  text_point.setX(20);

  painter.drawText(text_point,"FLASHHACK");

  if(device_name.contains(device_id) == true) {
    f.setPointSize(14);
    painter.setFont(f);
    painter.setPen(QColor(255,255,255,100));
    QPoint device_point = text_point;
    device_point.setY(device_point.y() + 6 + ( metric.height() / 2));
    painter.drawText(device_point,device_name[device_id] + " 0x" + QString::number(device_id,16).toUpper());
  }
}

QSize binvisual::sizeHint() const {
  QSize out;
  out.setWidth(400);
  out.setHeight(150);
  return out;
}

QSize binvisual::minimumSizeHint() const {
  QSize out;
  out.setWidth(400);
  out.setHeight(150);
  return out;
}

void binvisual::clear() {
  map.clear();
  device_id = -1;
  update();
}

int binvisual::bin_size() {
  return map[device_id].size();
}

void binvisual::set_region_state(int device_id, int offset, int size, int state) {
  if(map.contains(device_id) == false) return;
  this->device_id = device_id;
  int map_size = map[device_id].size();

  for(int x=0;x<size;x++) {
    if((offset + x > map_size - 1) == false) { // if in range
      map[device_id][offset + x] = (_program_region_state)state;
    }
  }
  update();
}

void binvisual::start_new_bin(int device_id, unsigned int s, const QString &bin_name) {
  if(map.contains(device_id) == false) {
    map[device_id].resize(s);
    map[device_id].fill(REGION_UNKNOWN);
  }
  this->device_id = device_id;
  device_name[device_id] = bin_name;
  update();
}
