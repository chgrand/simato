#include <QColor>
#include "colors.h"

ColorMap::color_map_t ColorMap::CMap = {
  {"green",     Qt::green},
  {"blue",      Qt::blue},
  {"magenta",   Qt::magenta},
  {"cyan",      Qt::cyan},
  {"yellow",    Qt::yellow},
  {"gray",      Qt::gray},
  {"black",     Qt::black},
  {"darkRed",   Qt::darkRed},
  {"darkGreen", Qt::darkGreen},
  {"darkBlue",  Qt::darkBlue},
  {"darkMagenta", Qt::darkMagenta},
  {"darkCyan",   Qt::darkCyan},
  {"darkYellow", Qt::darkYellow},
  {"darkGray",   Qt::darkGray},
  {"lightGray",  Qt::lightGray}
};

ColorMap::ordered_list_t ColorMap::OrderedList = {
  "green",
  "blue",
  "magenta",
  "cyan",
  "yellow",
  "gray",
  "black",
  "darkRed",
  "darkGreen",
  "darkBlue",
  "darkMagenta",
  "darkCyan",
  "darkYellow",
  "darkGray",
  "lightGray"
};

