#include <QtGui>
#include <iostream>
#include "main_window.h"
#include "mission_model.h"
#include "mission_data.hpp"

  
MissionData mission_data;

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);

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


  window.move(QApplication::desktop()->screen()->rect().center() - window.rect().center());
  window.resize(800,500);
  window.setWindowTitle("Simato");
  window.show();
  return app.exec();
}
