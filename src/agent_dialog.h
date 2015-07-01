// -*-C++-*-
#ifndef AGENT_DIALOG_H
#define AGENT_DIALOG_H


#include <QtWidgets>
#include "mission_model.h"

class AgentDialog: public QDialog
{
  Q_OBJECT
  
public:
  AgentDialog(QWidget *parent=0);
  void initData(MissionModel *mission_model);
  void storeData(MissionModel *mission_model);

private slots:
  //void selectModel();

 private:
  MissionModel *model;
  QComboBox *agent_name;
  QLineEdit *model_edit;
  QPushButton *model_btn;
  QComboBox *color_name;
  QComboBox *marker;
  QLineEdit *initial_energy;
  
  QDoubleSpinBox *initial_pos_x;
  QDoubleSpinBox *initial_pos_y;
  QDoubleSpinBox *initial_pos_z;
  
  QComboBox *waypoints_group;
};


#endif
