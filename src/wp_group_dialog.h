// -*-C++-*-
#ifndef WP_GROUP_DIALOG_H
#define WP_GROUP_DIALOG_H

#include <QtGui>
#include "mission_model.h"

class WpGroupDialog: public QDialog
{
  Q_OBJECT
 public:
  static const int NEW = 1;
  static const int ADD = 2;
  
 public:
  WpGroupDialog(QWidget *parent=0, int mode=NEW);
  //void initData(MissionModel *mission_model);
  //void storeData(MissionModel *mission_model);
  void addWpGroup(MissionModel *mission_model);

						
  std::string getName() {return name->text().toStdString();}
  //std::string getColor() {};
  //std::string get() {};
		      


  private slots:
    //void selectModel();

 private:
  MissionModel *model;
  QComboBox *name_list;
  QLineEdit *name;
  QComboBox *color_name;
  QComboBox *marker_char;
};


#endif
