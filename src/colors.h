//-*-c++-*-
#ifndef COLORS_H
#define COLORS_H

#include <string>
#include <vector>
#include <map>
#include <QColor>

class ColorMap
{
public:
  typedef std::map<std::string, Qt::GlobalColor> color_map_t;
  static color_map_t CMap;
  
  typedef std::vector<std::string> ordered_list_t;
  static ordered_list_t OrderedList;
};


#endif



