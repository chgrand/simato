//-*-c++-*-
#ifndef MAPVIEWER_H
#define MAPVIEWER_H

#include <QtGui>
#include <vector>
#include "mission_model.h"

/*
typedef struct
{
  float x_min;
  float y_min;
  float x_max;
  float y_max;
  float x_len;
  float y_len;
} MapRegion;
*/

class MapViewer : public QWidget
{
  Q_OBJECT

public:  
  enum action_type_t { 
    Add_Waypoints,
    Move_Waypoints,
    Delete_Waypoints,
    Add_Safety_Zone,
    Patrol_Add,
    Observations_Add,
    Observations_Move,
    Observations_Delete,
    None };
  
private:
  enum action_state_t { Start, Run, Stop };


public:
  MapViewer(MissionModel *model, QWidget *parent = 0);
  ~MapViewer();

  // Map
  //bool loadPixmap(QString filename);
  bool updateMapData();
  void setMapSize(float xm, float ym, float xM, float yM);
  void clearZoom();
  //void setOrigin(double x, double y);
  //void setScaleFactor(double xscale, double yscale);
  //void setZoom(double x1, double y1, double x2, double y2);
  
  // Mission data
  //void setMissionData(MissionModel *mission_model);

  void actionMode(action_type_t type, std::string object);

signals:
  void mouseNewPosition(QPointF pos);
				    
public slots:
  void setAlpha(int value);
  void setMapView(int state);
  void setGridView(int state);
  void setWpGroupFilter(const QString & name);
  void setZonesFilter(int state);
  void setObservationsFilter(int state);

protected:
  void plotMarker(QPainter &painter, QPoint pos, char mark, int size);
  void paintEvent(QPaintEvent *event);
  void resizeEvent ( QResizeEvent * event );
  
  // mouse callback used
  void mousePressEvent(QMouseEvent *event);
  void mouseReleaseEvent(QMouseEvent * event);
  void mouseMoveEvent(QMouseEvent *event);
  void wheelEvent ( QWheelEvent * event );
  
  // utility function
  void findPoint ( QPoint vp_pos);//, std::string &group, std::string &index);
  void findPoint ( QPoint vp_pos, std::string group_name);
  void movePoint ( QPoint new_vp_pos);

  void findObsPoint(QPoint vp_pos);
  void moveObsPoint( QPoint new_vp_pos);
  
  QPoint mapPixelPosFromViewport(const QPoint &pos_vp_px);
  QPointF mapMeterPosFromViewport(const QPoint &pos_vp_px);
  QPoint vpPosFromMapMeter(const QPointF &pos_map_m);

  // Mission data
  std::vector<QPointF> points_list;

  // Map data properties
  bool    map_exist;
  QImage  *map_image;       // Pixmap of the mission map
  QSizeF  map_size_m;      // Map size in meters
  QPointF map_origin_m;    // Coordinate of top-left corner in meters
  qreal opacity_;          // Map drawing opacity value
  //float origin_longitude;
  //float origin_latitude;
  
  // Viewport properties
  QPoint viewport_origin_px;  // Origin of viewport windows in the map (in pixels)
  float map_scale_factor;     // From map_pixel to map_meters
  float viewport_factor;      // From map_meters to viewport_pixel
  float zoom_factor;          // Zoom on the map

  // IHM actions
  bool move_action;
  QPoint move_position;

  // Mission data and actions
  MissionModel *mission_model;


  action_type_t  action_type;
  action_state_t action_state;
  std::string  action_object;
  //int action_counter;

  int marker_width;
  int marker_size;
  int zone_linewidth;
  int patrol_linewidth;

  bool map_view_on;
  bool grid_view_on;
  bool safety_zones_on;
  bool observations_on;

  std::string wp_group_filter;

  //std::vector<QPoint> edited_zone;
  QPointF current_map_point;
  QPoint  current_point;

  std::string selected_point_group;
  std::string selected_point_index;
  std::string edited_patrol;

};

#endif
