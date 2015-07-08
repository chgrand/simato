/*
 #include <QWidget>
 #include <QLabel>
 #include <QVBoxLayout>
 #include <QStatusBar>
 #include <QMainWindow>
 #include <QApplication>
 #include <QResizeEvent>
 */

#include <iostream>
#include <iomanip>
#include <sstream>
//#include "map_viewer.h"
#include "mission_viewer.h"

//-----------------------------------------------------------------------------
void MissionViewer::updateMap()
{
  /*
   QString filename = QString::fromStdString(mission_model->home_dir
   +mission_model->map_data.image);
   map_viewer->loadPixmap(filename);

   map_viewer->setMapSize(mission_model->map_data.x_min,
   mission_model->map_data.y_min,
   mission_model->map_data.x_max,
   mission_model->map_data.y_max);
   map_viewer->update();
   */
  map_viewer->updateMapData();
}

//-----------------------------------------------------------------------------
MissionViewer::MissionViewer(MissionModel *model, QWidget *parent) :
    QWidget(parent), mission_model(model)
{
  map_viewer = new MapViewer(model, this);
  map_viewer->setMinimumSize(200, 200);

  // Tools menu
  QGroupBox *viewGroup = new QGroupBox(this);
  viewGroup->setFixedWidth(180);
  viewGroup->setTitle(tr("View"));
  map_view_check = new QCheckBox("View Map", viewGroup);
  map_view_check->setChecked(true);

  grid_view_check = new QCheckBox("View Grid", viewGroup);
  grid_view_check->setChecked(true);

  QLabel *alpha_label = new QLabel("Map transparency:", viewGroup);
  alpha_spin = new QSpinBox(viewGroup);
  alpha_spin->setRange(0, 100);
  alpha_spin->setSingleStep(5);
  alpha_spin->setValue(50);

  QLabel *grid_label = new QLabel("Grid step:", viewGroup);
  grid_spin = new QSpinBox(viewGroup);
  grid_spin->setRange(5, 100);
  grid_spin->setSingleStep(5);
  grid_spin->setValue(20);

  // Filters
  QGroupBox *filterGroup = new QGroupBox(this);
  filterGroup->setFixedWidth(180);
  filterGroup->setTitle(tr("Filters"));
  QCheckBox *safety_zone = new QCheckBox("Safety Zone", filterGroup);
  safety_zone->setChecked(true);
  observations_filter = new QCheckBox("Observation goals", filterGroup);
  observations_filter->setChecked(true);
  QLabel *agent_label = new QLabel("Agent:");
  agent_filter = new QComboBox(filterGroup);
  QLabel *wp_group_label = new QLabel("Waypoints Group:");
  wp_group_filter = new QComboBox(filterGroup);
  QLabel *patrol_label = new QLabel("Patrol:");
  patrol_filter = new QComboBox(filterGroup);

  // Tools menu layout configuration
  QVBoxLayout *viewGroupLayout = new QVBoxLayout(viewGroup);
  viewGroupLayout->addWidget(map_view_check);
  viewGroupLayout->addWidget(grid_view_check);
  viewGroupLayout->addSpacing(12);
  viewGroupLayout->addWidget(alpha_label);
  viewGroupLayout->addWidget(alpha_spin);
  viewGroupLayout->addWidget(grid_label);
  viewGroupLayout->addWidget(grid_spin);

  QVBoxLayout *filterGroupLayout = new QVBoxLayout(filterGroup);
  filterGroupLayout->addWidget(safety_zone);
  filterGroupLayout->addWidget(observations_filter);
  filterGroupLayout->addWidget(agent_label);
  filterGroupLayout->addWidget(agent_filter);
  filterGroupLayout->addWidget(wp_group_label);
  filterGroupLayout->addWidget(wp_group_filter);
  filterGroupLayout->addWidget(patrol_label);
  filterGroupLayout->addWidget(patrol_filter);

  // Print cursor position
  pos_msg = new QLabel("Local position: ", this);

  QVBoxLayout *toolLayout = new QVBoxLayout();
  //toolLayout->addWidget(actionGroup);
  toolLayout->addWidget(viewGroup);
  toolLayout->addSpacing(12);
  toolLayout->addWidget(filterGroup);
  toolLayout->addStretch(1);

  QHBoxLayout *viewLayout = new QHBoxLayout();
  viewLayout->addWidget(map_viewer);
  viewLayout->addLayout(toolLayout);

  QVBoxLayout *baseLayout = new QVBoxLayout(this);
  baseLayout->addLayout(viewLayout);
  baseLayout->addWidget(pos_msg);

  QObject::connect(map_viewer, SIGNAL(mouseNewPosition(QPointF)), this, SLOT(onMouseOverMap(QPointF)));

  QObject::connect(map_view_check, SIGNAL(stateChanged(int)), map_viewer, SLOT(setMapView(int)));

  QObject::connect(grid_view_check, SIGNAL(stateChanged(int)), map_viewer, SLOT(setGridView(int)));

  QObject::connect(alpha_spin, SIGNAL(valueChanged(int)), map_viewer, SLOT(setAlpha(int)));

  QObject::connect(wp_group_filter, SIGNAL(currentIndexChanged(const QString &)), map_viewer,
      SLOT(setWpGroupFilter(const QString &)));

  QObject::connect(safety_zone, SIGNAL(stateChanged(int)), map_viewer, SLOT(setZonesFilter(int)));

  QObject::connect(observations_filter, SIGNAL(stateChanged(int)), map_viewer, SLOT(setObservationsFilter(int)));

  //std::cout << "group filter count=" << wp_group_filter->count() << std::endl;

  alpha_spin->setValue(50);
  map_viewer->setAlpha(50);
}
;

//-----------------------------------------------------------------------------
void MissionViewer::onMouseOverMap(QPointF pos)
{
  std::stringstream message;

  message << "Local position: " << std::fixed << std::setprecision(3) << pos.x() << ", " << pos.y()
      << "  |  GPS coord: ";

  pos_msg->setText(QString(message.str().c_str()));
}
;

//-----------------------------------------------------------------------------
void MissionViewer::updateControl()
{

  std::cout << "group filter count=" << wp_group_filter->count() << std::endl;
  /*while(wp_group_filter->count()>0)
   wp_group_filter->removeItem(0);
   */
  wp_group_filter->clear();
  std::cout << "group filter count=" << wp_group_filter->count() << std::endl;

  wp_group_filter->addItem("All");
  for(auto wp_group : mission_model->wp_groups) {
    wp_group_filter->addItem(QString::fromStdString(wp_group.first));
  }
}

//-----------------------------------------------------------------------------
void MissionViewer::setFilter(std::string name)
{
  int index = wp_group_filter->findText(QString::fromStdString(name));
  std::cout << "Set filter=" << index << std::endl;
  wp_group_filter->setCurrentIndex(index);

}
