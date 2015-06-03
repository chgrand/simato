/*
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QStatusBar>
#include <QMainWindow>
#include <QApplication>
#include <QResizeEvent>
*/

#include <QtGui>
#include <iostream>
#include "main_window.h"
#include "mission_model.h"
#include "mission_data.hpp"

  
MissionData mission_data;

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);

  //MapWidget map_view;
  //AppWindow window;

  MainWindow window(&mission_data);
  
  //set the icon
  QString iconPath = QString("$ACTION_HOME/ressources/images/icone_action.png");
  iconPath = iconPath.replace(QString("$ACTION_HOME"), qgetenv("ACTION_HOME"));
  QFileInfo checkIcon(iconPath);
  if (checkIcon.exists() && checkIcon.isFile()) {
    window.setWindowIcon(QIcon(iconPath));
  }
  
  if(argc > 1){
    std::cout << "Loading " << argv[1] << std::endl;
    QString filename = QString(argv[1]);
    if(filename.contains("$ACTION_HOME")){
      filename = filename.replace(QString("$ACTION_HOME"), qgetenv("ACTION_HOME"));
    }
    window.loadFile(filename);
  }

  //  if(!map_view.loadPixmap("rover.jpg"))
  //  return 1;

  window.resize(600,600);
  //window.move(300, 50);
  window.setWindowTitle("ISMAC - MapView");
  window.show();
  return app.exec();
}
