#include <iostream>
#include <iomanip>
#include <QPainter>
#include <QImage>
#include <QMouseEvent>
#include "map_viewer.h"
#include "colors.h"

//-----------------------------------------------------------------------------
MapViewer::MapViewer(MissionModel *model, QWidget *parent) :
    QWidget(parent), map_exist(false), opacity_(1.0), mission_model(model), action_state(Stop)
{
  setCursor(Qt::CrossCursor);
  setMouseTracking(true);

  viewport_origin_px = QPoint(0, 0);
  zoom_factor = 1.0;

  marker_width = 2;
  marker_size = 12;
  zone_linewidth = 4;
  patrol_linewidth = 1;

  map_view_on = true;
  grid_view_on = true;
  observations_on = true;
  safety_zones_on = true;

  // Ihm action
  move_action = false;

  action_type = None;
  action_state = Stop;

  selected_point_group = "";
  selected_point_index = "";

  wp_group_filter = "";

}

//-----------------------------------------------------------------------------
MapViewer::~MapViewer()
{
}

//-----------------------------------------------------------------------------
bool MapViewer::updateMapData()
{
  QString filename = mission_model->get_home_dir().filePath(QString::fromStdString(mission_model->map_data.image));

  if(map_exist)
    delete map_image;

  map_image = new QImage(filename);
  if(map_image->isNull()) {
    delete map_image;
    double dx = mission_model->map_data.x_max - mission_model->map_data.x_min;
    double dy = mission_model->map_data.y_max - mission_model->map_data.y_min;

    int W = 100;
    int H = int(100. * dy / dx);

    map_image = new QImage(W, H, QImage::Format_RGB32);
    map_image->fill(QColor(245, 245, 245));
  }
  else {
    std::cout << "Load map file: " << filename.data() << " -- map size (" << map_image->width() << ", "
        << map_image->height() << ")" << std::endl;
  }

  if(map_image->isNull()) {
    std::cerr << "MapViewer init map error..." << filename.data() << std::endl;
    map_exist = false;
    return false;
  }
  map_exist = true;

  // Analyse map size / viewport
  float w_factor = (float) this->width() / map_image->width();
  float h_factor = (float) this->height() / map_image->height();

  if(w_factor < h_factor)
    viewport_factor = w_factor;
  else
    viewport_factor = h_factor;

  //std::cout << "viewport_factor = " << viewport_factor << std::endl;

  setMapSize(mission_model->map_data.x_min, mission_model->map_data.y_min, mission_model->map_data.x_max,
      mission_model->map_data.y_max);

  this->update();
  return true;
}

//-----------------------------------------------------------------------------
void MapViewer::setAlpha(int value)
{
  opacity_ = value / 100.0;
  if(opacity_ < 0.0)
    opacity_ = 0.0;
  if(opacity_ > 1.0)
    opacity_ = 1.0;
  update();
}

//-----------------------------------------------------------------------------
void MapViewer::setMapView(int state)
{
  if(state == Qt::Checked)
    map_view_on = true;
  else
    map_view_on = false;
  this->update();
}

//-----------------------------------------------------------------------------
void MapViewer::setGridView(int state)
{
  if(state == Qt::Checked)
    grid_view_on = true;
  else
    grid_view_on = false;
  this->update();
}

//-----------------------------------------------------------------------------
void MapViewer::setObservationsFilter(int state)
{
  if(state == Qt::Checked)
    observations_on = true;
  else
    observations_on = false;
  this->update();
}

//-----------------------------------------------------------------------------
void MapViewer::setZonesFilter(int state)
{
  if(state == Qt::Checked)
    safety_zones_on = true;
  else
    safety_zones_on = false;
  this->update();
}

//-----------------------------------------------------------------------------
void MapViewer::setWpGroupFilter(const QString & name)
{
  wp_group_filter = name.toStdString();
  this->update();
}

//-----------------------------------------------------------------------------
void MapViewer::setMapSize(float xm, float ym, float xM, float yM)
{
  map_origin_m.setX(xm);
  map_origin_m.setY(yM);

  map_size_m.setWidth(xM - xm);
  map_size_m.setHeight(yM - ym);

  // Map meter to pixel scale factor
  map_scale_factor = (float) map_image->width() / (float) map_size_m.width();

  //std::cout << "Map_sacle_factor = " << map_scale_factor << std::endl;
}

//-----------------------------------------------------------------------------
void MapViewer::wheelEvent(QWheelEvent * event)
{
  // Mouse pos = zoom center on viewport (in pixel)
  QPoint pos_vp_px = QPoint(event->x(), event->y());

  // Zoom center on map (in pixel)
  QPoint pos_map_px = pos_vp_px / (viewport_factor * zoom_factor) + viewport_origin_px;

  // Zoom increment
  if(event->delta() > 0) {
    zoom_factor += 0.5;
    if(zoom_factor > 10.0)
      zoom_factor = 10.0;
  }
  else {
    zoom_factor -= 0.5;
    if(zoom_factor < 1.0)
      zoom_factor = 1.0;
  }

  // Maintain zoom at center on map
  viewport_origin_px = pos_map_px - pos_vp_px / (viewport_factor * zoom_factor);

  if(viewport_origin_px.x() < 0)
    viewport_origin_px.setX(0);
  if(viewport_origin_px.y() < 0)
    viewport_origin_px.setY(0);

  if(zoom_factor == 1.0)
    viewport_origin_px = QPoint(0, 0);

  this->update();
}

//-----------------------------------------------------------------------------
void MapViewer::actionMode(action_type_t type, std::string object)
{
  action_type = type;
  action_state = Start;
  action_object = object;
}

//-----------------------------------------------------------------------------
void MapViewer::mousePressEvent(QMouseEvent *event)
{
  int buttons = event->buttons();

  // position in viewport
  QPoint pos_vp_px = QPoint(event->x(), event->y());
  QPoint pos_map_px = mapPixelPosFromViewport(pos_vp_px);
  QPointF pos_map_m = mapMeterPosFromViewport(pos_vp_px);

  // Start action with button 1
  if((action_state == Start) && (buttons == 1)) {
    //action_counter = 0;
    switch (action_type) {
    case Add_Waypoints:
      action_state = Run;
      break;

    case Add_Safety_Zone:
      action_state = Run;
      break;

    case Move_Waypoints:
      action_state = Run;
      findPoint(pos_vp_px, action_object);
      movePoint(pos_vp_px);
      this->update();
      break;

    case Delete_Waypoints:
      action_state = Run;
      //mission_model->wp_groups[action_object].patrols.clear();
      //mission_model->wp_groups[action_object].patrol_index = 1;
      break;

    case Patrol_Add:
      action_state = Run;
      edited_patrol = to_string(mission_model->wp_groups[action_object].patrol_index++);
      break;

    case Observations_Add:
      action_state = Run;
      break;

    case Observations_Move:
      action_state = Run;
      findObsPoint(pos_vp_px);
      moveObsPoint(pos_vp_px);
      this->update();
      break;

    case Observations_Delete:
      action_state = Run;
      break;

    case None:
      break;
    }
  }

  // Run action on press event
  if((action_state == Run) && (buttons == 1)) {
    // current point in the map in meters
    mission::point3_t a_point;
    a_point.x = pos_map_m.x();
    a_point.y = pos_map_m.y();
    a_point.z = 0.0;
    int index = 0;

    switch (action_type) {
    case Add_Waypoints:
      index = mission_model->wp_groups[action_object].waypoint_index++;
      mission_model->wp_groups[action_object].waypoints[to_string(index)] = a_point;
      //action_counter++;
      break;

    case Move_Waypoints:
      break;

    case Add_Safety_Zone:
      mission_model->agents[action_object].safety_zone.push_back(a_point);
      break;

    case Delete_Waypoints:
      findPoint(pos_vp_px, action_object);
      mission_model->wp_groups[action_object].waypoints.erase(selected_point_index);

      //now erase all patrol that have this point
      {
        auto& patrols = mission_model->wp_groups[action_object].patrols;
        for(auto it_p = patrols.begin(); it_p != patrols.end();) {
          if(std::find(it_p->second.begin(), it_p->second.end(), selected_point_index) != it_p->second.end()) {
            std::cout << "Erasing " << it_p->first << std::endl;
            it_p = patrols.erase(it_p);
          }
          else {
            ++it_p;
          }
        }
      }
      break;

    case Patrol_Add:
      findPoint(pos_vp_px, action_object);
      mission_model->wp_groups[action_object].patrols[edited_patrol].push_back(selected_point_index);
      break;

    case Observations_Add:
      index = mission_model->observation_points_counter++;
      if(mission_model->observations.count(to_string(index)) >= 1) {
        std::cout << "Error : cannot add a point since " << index << " is an existing point" << std::endl;
        break;
      }
      mission_model->observations[to_string(index)] = a_point;
      break;

    case Observations_Move:
      break;

    case Observations_Delete:
      findObsPoint(pos_vp_px);
      mission_model->observations.erase(selected_point_index);
      break;

    case None:
      break;
    }
    this->update();
    return;
  }

  // Stop action on button 2
  if((action_state != Stop) && (buttons == 2)) {
    action_type = None;
    action_state = Stop;
    selected_point_index = "";
    this->update();
    return;
  }

  // Else move map on button 2
  if(action_state == Stop) {
    if((!move_action) & (buttons == 2)) {
      move_action = true;
      move_position = pos_map_px;
    }
  }
}

//-----------------------------------------------------------------------------
void MapViewer::mouseReleaseEvent(QMouseEvent *event)
{
  int buttons = event->buttons();

  if((move_action) & (buttons != 2)) {
    move_action = false;
    if(viewport_origin_px.x() < 0)
      viewport_origin_px.setX(0);
    if(viewport_origin_px.y() < 0)
      viewport_origin_px.setY(0);

    QSizeF map_size_vp = map_size_m * map_scale_factor * viewport_factor * zoom_factor;
    if(this->width() >= map_size_vp.width())
      viewport_origin_px.setX(0);
    if(this->height() >= map_size_vp.height())
      viewport_origin_px.setY(0);
    this->update();
    return;
  }

  if((action_type == Move_Waypoints || action_type == Observations_Move) && (buttons != 1)) {
    selected_point_group = "";
    selected_point_index = "";
    action_state = Start;
    this->update();
  }

}

//-----------------------------------------------------------------------------
void MapViewer::mouseMoveEvent(QMouseEvent *event)
{
  int buttons = event->buttons();

  QPoint pos_vp_px = QPoint(event->x(), event->y());
  QPoint pos_map_px = mapPixelPosFromViewport(pos_vp_px);
  QPointF pos_map_m = mapMeterPosFromViewport(pos_vp_px);
  emit mouseNewPosition(pos_map_m);

  current_map_point = pos_map_m;
  current_point = pos_vp_px;

  if((move_action) & (buttons == 2)) {
    QPoint delta_pos = pos_map_px - move_position;
    viewport_origin_px -= delta_pos;
    this->update();
    return;
  }

  if(action_state == Run) {
    switch (action_type) {
    case Add_Safety_Zone:
      this->update();
      break;

    case Move_Waypoints:
      movePoint(pos_vp_px);
      this->update();
      break;

    case Observations_Move:
      moveObsPoint(pos_vp_px);
      this->update();
      break;
    }
  }
}

//-----------------------------------------------------------------------------
void MapViewer::resizeEvent(QResizeEvent * event)
{
  if(!map_exist)
    return;
  float w_factor = (float) event->size().width() / map_image->width();
  float h_factor = (float) event->size().height() / map_image->height();

  if(w_factor < h_factor)
    viewport_factor = w_factor;
  else
    viewport_factor = h_factor;

  std::cout << "viewport_factor = " << viewport_factor << std::endl;
}

//-----------------------------------------------------------------------------
void MapViewer::findPoint(QPoint vp_pos)
{
  double sq_dist_min = 10e6;
  QPointF map_pos = mapMeterPosFromViewport(vp_pos);

  for(auto wp_group : mission_model->wp_groups) {
    for(auto point : wp_group.second.waypoints) {
      double dx = point.second.x - map_pos.x();
      double dy = point.second.y - map_pos.y();
      if((dx * dx + dy * dy) < sq_dist_min) {
        sq_dist_min = dx * dx + dy * dy;
        selected_point_index = point.first;
        selected_point_group = wp_group.first;
      }
    }
  }
}

//-----------------------------------------------------------------------------
void MapViewer::findPoint(QPoint vp_pos, std::string group_name)
{
  double sq_dist_min = 10e6;
  QPointF map_pos = mapMeterPosFromViewport(vp_pos);

  for(auto point : mission_model->wp_groups[group_name].waypoints) {
    double dx = point.second.x - map_pos.x();
    double dy = point.second.y - map_pos.y();
    if((dx * dx + dy * dy) < sq_dist_min) {
      sq_dist_min = dx * dx + dy * dy;
      selected_point_index = point.first;
      selected_point_group = group_name;
    }
  }
}

//-----------------------------------------------------------------------------
void MapViewer::findObsPoint(QPoint vp_pos)
{
  double sq_dist_min = 10e6;
  QPointF map_pos = mapMeterPosFromViewport(vp_pos);

  for(auto point : mission_model->observations) {
    double dx = point.second.x - map_pos.x();
    double dy = point.second.y - map_pos.y();
    if((dx * dx + dy * dy) < sq_dist_min) {
      sq_dist_min = dx * dx + dy * dy;
      selected_point_index = point.first;
    }
  }
}

//-----------------------------------------------------------------------------
void MapViewer::movePoint(QPoint vp_pos)      //, std::string &group, std::string &index)
{
  QPointF map_pos = mapMeterPosFromViewport(vp_pos);
  mission_model->wp_groups[selected_point_group].waypoints[selected_point_index].x = map_pos.x();
  mission_model->wp_groups[selected_point_group].waypoints[selected_point_index].y = map_pos.y();
}

//-----------------------------------------------------------------------------
void MapViewer::moveObsPoint(QPoint vp_pos)
{
  QPointF map_pos = mapMeterPosFromViewport(vp_pos);
  mission_model->observations[selected_point_index].x = map_pos.x();
  mission_model->observations[selected_point_index].y = map_pos.y();
}

//-----------------------------------------------------------------------------
void MapViewer::paintEvent(QPaintEvent *)
{
  QPainter painter(this);

  painter.initFrom(this);
  painter.setRenderHint(QPainter::Antialiasing, true);

  // Background
  // ----------
  painter.setPen(QPen());
  painter.setBrush(QBrush(Qt::white, Qt::SolidPattern));
  painter.drawRect(0, 0, this->width(), this->height());
  painter.setBrush(Qt::NoBrush);

  // Map image
  // ---------
  if(map_view_on)
    if(map_exist) {
      painter.save();
      painter.setOpacity(opacity_);
      painter.scale(viewport_factor * zoom_factor, viewport_factor * zoom_factor);
      painter.translate(-viewport_origin_px);
      painter.drawImage(0, 0, *map_image);
      painter.restore();
    }

  // Plot map grid
  // -------------
  if(grid_view_on) {
    float x_step_m = 20;
    float y_step_m = 20;
    float xo = map_origin_m.x();
    float yo = map_origin_m.y();
    float x0 = int(xo / x_step_m) * x_step_m;
    float y0 = int(yo / y_step_m) * y_step_m;
    float xs = map_size_m.width();
    float ys = map_size_m.height();

    painter.save();
    painter.setOpacity(opacity_);
    painter.setPen(QPen(Qt::black, 1));
    for(float x = x0; x < (xo + xs); x += x_step_m) {
      QPoint p1_vp = vpPosFromMapMeter(QPointF(x, yo));
      QPoint p2_vp = vpPosFromMapMeter(QPointF(x, yo - ys));
      painter.drawLine(p1_vp, p2_vp);
    }
    for(float y = y0; y > (yo - ys); y -= y_step_m) {
      QPoint p1_vp = vpPosFromMapMeter(QPointF(xo, y));
      QPoint p2_vp = vpPosFromMapMeter(QPointF(xo + xs, y));
      painter.drawLine(p1_vp, p2_vp);
    }

    // map origin
    painter.setPen(QPen(Qt::red, 1));
    {
      QPoint p1_vp = vpPosFromMapMeter(QPointF(-x_step_m, 0));
      QPoint p2_vp = vpPosFromMapMeter(QPointF(+x_step_m, 0));
      painter.drawLine(p1_vp, p2_vp);
    }
    {
      QPoint p1_vp = vpPosFromMapMeter(QPointF(0, -y_step_m));
      QPoint p2_vp = vpPosFromMapMeter(QPointF(0, +y_step_m));
      painter.drawLine(p1_vp, p2_vp);
    }
    painter.restore();
  }

  //std::vector<std::string> color_cycle{"cyan","yellow","gray","darkRed",
  //    "darkBlue","darkYellow"};
  std::vector<std::string> color_cycle { "blue", "magenta", "darkGray", "darkRed", "darkBlue" };

  int color_index = 0;

  // Plot waypoints groups
  // ---------------------
  //std::cout << "filter =" << wp_group_filter << std::endl;

  // for each wp_group
  for(auto wp_group : mission_model->wp_groups)
    // if not filtered
    if((wp_group_filter == "All") || (wp_group_filter == wp_group.first)) {
      std::string name = wp_group.first;
      std::string the_color = wp_group.second.color;
      char the_marker = wp_group.second.marker;

      // Waypoints 
      painter.setPen(QPen(QColor(ColorMap::CMap[the_color]), marker_width));
      for(auto point : wp_group.second.waypoints) {
        QPointF map_point = QPointF(point.second.x, point.second.y);
        QPoint vp_point = vpPosFromMapMeter(map_point);

        // if waypoint is moved by plot it in red
        if((wp_group.first == selected_point_group) && (point.first == selected_point_index)) {
          painter.setPen(QPen(Qt::red, marker_width));
          plotMarker(painter, vp_point, the_marker, marker_size);
          painter.setPen(QPen(QColor(ColorMap::CMap[the_color]), marker_width));
        }
        else
          plotMarker(painter, vp_point, the_marker, marker_size);
        painter.drawText(vp_point, QString::fromStdString(" " + point.first));
      }
      // Patrols
      // for each patrol of wp_group
      for(auto patrol : wp_group.second.patrols) {
        painter.setPen(QPen(QColor(ColorMap::CMap[color_cycle[color_index++]]), patrol_linewidth));

        if(color_index >= color_cycle.size())
          color_index = 0;
        QVector<QPoint> _line;

        for(auto index : patrol.second) {
          mission::point3_t point = wp_group.second.waypoints[index];
          QPointF map_point = QPointF(point.x, point.y);
          QPoint vp_point = vpPosFromMapMeter(map_point);
          _line.push_back(vp_point);
        }
        painter.drawPolyline(QPolygon(_line));
      }
    }

  // Plot observation points
  // -----------------------
  if(observations_on) {
    painter.setPen(QPen(Qt::red, marker_width));
    for(auto point : mission_model->observations) {
      QPointF map_point = QPointF(point.second.x, point.second.y);
      QPoint vp_point = vpPosFromMapMeter(map_point);
      plotMarker(painter, vp_point, '+', marker_size);
    }
  }

  // Plot Agent properties (initial_position + safety_zone)
  // ------------------------------------------------------
  for(auto agent : mission_model->agents) {
    std::string agent_name = agent.first;

    // agent initial position
    QPointF map_point = QPointF(agent.second.initial_position.x, agent.second.initial_position.y);

    QPoint vp_point = vpPosFromMapMeter(map_point);
    painter.setPen(QPen(Qt::darkCyan, marker_width));
    plotMarker(painter, vp_point, 's', marker_size);

    // plot safety zones (excepted the current edited one)
    if(safety_zones_on) {
      if(!((action_state == Run) && (action_type == Add_Safety_Zone) && (agent_name == action_object))) {
        std::string the_color = agent.second.color;
        painter.setPen(QPen(QColor(ColorMap::CMap[the_color]), zone_linewidth));
        QVector<QPoint> _polygon;
        for(auto point : agent.second.safety_zone) {
          QPointF map_point = QPointF(point.x, point.y);
          QPoint vp_point = vpPosFromMapMeter(map_point);
          _polygon.push_back(vp_point);
        }
        painter.drawPolygon(QPolygon(_polygon));
      }
    }
  }

  // Plot the currently edited safety zone
  // -------------------------------------
  if(action_state == Run) {
    switch (action_type) {
    case Add_Safety_Zone:
      mission::agent_t * an_agent_ptr = &(mission_model->agents[action_object]);
      QPainterPath path;
      bool first = true;
      for(auto point : an_agent_ptr->safety_zone) {
        QPointF map_point = QPointF(point.x, point.y);
        QPoint vp_point = vpPosFromMapMeter(map_point);
        if(first)
          path.moveTo(QPointF(vp_point));
        else
          path.lineTo(QPointF(vp_point));
        first = false;
      }
      path.lineTo(QPointF(current_point));
      painter.setPen(QPen(Qt::gray, zone_linewidth));
      painter.drawPath(path);
      break;
    }
  }
}

//-----------------------------------------------------------------------------
void MapViewer::plotMarker(QPainter &painter, QPoint pos, char mark, int size)
{
  int half_size = size / 2;
  int x = pos.x();
  int y = pos.y();

  switch (mark) {
  case 'x':
  case 'X':
    painter.drawLine(x - half_size, y - half_size, x + half_size, y + half_size);
    painter.drawLine(x - half_size, y + half_size, x + half_size, y - half_size);
    break;

  case '+':
    painter.drawLine(x, y - half_size, x, y + half_size);
    painter.drawLine(x - half_size, y, x + half_size, y);
    break;

  case 's':
    painter.drawRect(x - half_size, y - half_size, size, size);
    break;
  }
}

//-----------------------------------------------------------------------------
QPoint MapViewer::mapPixelPosFromViewport(const QPoint &pos_vp_px)
{
  // position on map in pixel
  return pos_vp_px / (viewport_factor * zoom_factor) + viewport_origin_px;
}

//-----------------------------------------------------------------------------
QPointF MapViewer::mapMeterPosFromViewport(const QPoint &pos_vp_px)
{
  // position on map in pixel
  QPoint pos_map_px = pos_vp_px / (viewport_factor * zoom_factor) + viewport_origin_px;

  // position on map meter
  return QPointF(+(float) pos_map_px.x() / map_scale_factor + map_origin_m.x(),
      -(float) pos_map_px.y() / map_scale_factor + map_origin_m.y());
}

//-----------------------------------------------------------------------------
QPoint MapViewer::vpPosFromMapMeter(const QPointF &pos_map_m)
{
  QPoint pos_px(+(int) ((pos_map_m.x() - map_origin_m.x()) * map_scale_factor),
      -(int) ((pos_map_m.y() - map_origin_m.y()) * map_scale_factor));
  return (pos_px - viewport_origin_px) * viewport_factor * zoom_factor;
}
