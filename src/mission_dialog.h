// -*-C++-*-
#ifndef MISSION_DIALOG_H
#define MISSION_DIALOG_H

#include <QtGui>
#include "mission_model.h"

class MissionDialog: public QDialog
{
  Q_OBJECT
  
public:
  MissionDialog(QWidget *parent=0);
  void initData(MissionModel *mission_model);
  void storeData(MissionModel *mission_model);

private slots:
  void selectHomeDir();
  void selectFile();

private:
  QLineEdit   *maxtime_edit;
  QLineEdit   *home_edit;
  QPushButton *home_btn;
  QLineEdit   *map_image_edit;
  QPushButton *map_image_btn;
  QLineEdit   *map_region_edit;
  QPushButton *map_region_btn;
  QLineEdit   *map_dtm_edit;
  QPushButton *map_dtm_btn;
  QLineEdit   *map_blender_edit;
  QPushButton *map_blender_btn;
  QDoubleSpinBox *map_size_xmin;
  QDoubleSpinBox *map_size_xmax;
  QDoubleSpinBox *map_size_ymin;
  QDoubleSpinBox *map_size_ymax;
  
};
#endif
