// -*-c++-*-

#ifndef MISSION_VIEWER_H
#define MISSION_VIEWER_H

#include <QtGui>
#include "map_viewer.h"
#include "mission_model.h"
//#include "mission_data.hpp"

class MissionViewer: public QWidget
{
Q_OBJECT

public:
  MissionViewer(MissionModel *model, QWidget *parent = 0);
  void updateMap();
  MapViewer *getMapViewer() const
  {
    return map_viewer;
  }
  ;
  void updateControl();
  void setFilter(std::string name);

private slots:
  void onMouseOverMap(QPointF pos);

private:
  MissionModel *mission_model;
  MapViewer *map_viewer;
  QLabel *pos_msg;

  QCheckBox *map_view_check;
  QCheckBox *grid_view_check;
  //QLabel *alpha_label;
  QSpinBox *alpha_spin;
  //QLabel *grid_label;
  QSpinBox *grid_spin;

  QCheckBox *observations_filter;
  QComboBox *agent_filter;
  QComboBox *wp_group_filter;
  QComboBox *patrol_filter;

};

#endif
