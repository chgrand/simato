// -*-C++-*-

#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QtGui>
#include "mission_model.h"
#include "mission_viewer.h"
#include "mission_data.hpp"

class MainWindow: public QMainWindow
{
  Q_OBJECT
  
public:
  MainWindow(MissionData *data, QWidget *parent=0);

  void loadFile(const QString&);

private slots:
  void missionNew();
  void missionEdit();
  void missionOpen();
  void missionSave();
  void missionSaveAs();
  void missionReload();

  void wpGroupNew();
  void waypointsAdd();
  void waypointsMove();
  void waypointsDelete();
 
  void patrolAdd();

  //void agentAdd();
  void agentEdit();
  //void agentDelete();
  void agentZone();
  
  void observationPointsAdd();
  void observationPointsMove();
  void observationPointsDelete();

  void generateHipopData();
  void processHipopOutput();

protected:
  void createMenu();
  bool tryEraseExistingMission();

 private:
  //MissionData *mission_data;
  bool mission_exist;
  QString filename;
  MissionModel *mission_model;
  MissionViewer *mission_viewer;
  QProcess *hipop_launcher;
};

#endif
