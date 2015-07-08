// MainWindow Class:
// manage de main view and the menu function
// 

#include <iostream>
#include <iomanip>
#include <sstream>
#include "agent_dialog.h"
#include "wp_group_dialog.h"
#include "mission_dialog.h"
#include "mission_model.h"
#include "mission_viewer.h"
#include "main_window.h"

//-----------------------------------------------------------------------------
MainWindow::MainWindow(MissionData *data, QWidget *parent) :
    QMainWindow(parent)
{
  mission_model = new MissionModel();
  mission_exist = false;
  mission_viewer = new MissionViewer(mission_model);
  setCentralWidget(mission_viewer);
  createMenu();
}

//-----------------------------------------------------------------------------
void MainWindow::createMenu()
{
  QPixmap newpix("new.png");
  QPixmap openpix("open.png");
  QPixmap quitpix("quit.png");

  QAction *mission_new = new QAction(newpix, "&New", this);
  QAction *mission_open = new QAction(openpix, "&Open", this);
  QAction *mission_save = new QAction("&Save", this);
  QAction *mission_save_as = new QAction("Save &as", this);
  QAction *mission_reload = new QAction("Reload", this);
  QAction *mission_edit = new QAction("&Edit paramters", this);
  QAction *quit = new QAction(quitpix, "&Quit", this);
  quit->setShortcut(tr("CTRL+Q"));

  QMenu *app_menu;

  // Mission menu
  app_menu = menuBar()->addMenu("&File");
  app_menu->addAction(mission_new);
  app_menu->addAction(mission_open);
  app_menu->addAction(mission_save);
  app_menu->addAction(mission_save_as);
  app_menu->addAction(mission_reload);
  app_menu->addSeparator();
  app_menu->addAction(mission_edit);
  app_menu->addSeparator();
  app_menu->addAction(quit);
  QObject::connect(mission_new, SIGNAL(triggered()), this, SLOT(missionNew()));
  QObject::connect(mission_open, SIGNAL(triggered()), this, SLOT(missionOpen()));
  QObject::connect(mission_save, SIGNAL(triggered()), this, SLOT(missionSave()));
  QObject::connect(mission_save_as, SIGNAL(triggered()), this, SLOT(missionSaveAs()));
  QObject::connect(mission_reload, SIGNAL(triggered()), this, SLOT(missionReload()));
  QObject::connect(mission_edit, SIGNAL(triggered()), this, SLOT(missionEdit()));
  QObject::connect(quit, SIGNAL(triggered()), qApp, SLOT(quit()));

  // Waypoint menu
  // -------------
  app_menu = menuBar()->addMenu("&Waypoints");
  QAction *wp_group_new = new QAction("&New Group", this);
  QAction *wp_group_edit = new QAction("&Edit Group", this);
  QAction *wp_group_del = new QAction("&Delete Group", this);
  app_menu->addAction(wp_group_new);
  app_menu->addAction(wp_group_edit);
  app_menu->addAction(wp_group_del);

  QAction *waypoints_add = new QAction("&Add Waypoints", this);
  QAction *waypoints_move = new QAction("&Move Waypoints", this);
  QAction *waypoints_del = new QAction("&Delete Waypoints", this);
  QMenu *sub_menu = app_menu->addMenu("Edit Waypoints Group");
  {
    sub_menu->addAction(waypoints_add);
    sub_menu->addAction(waypoints_move);
    sub_menu->addAction(waypoints_del);
  }
  app_menu->addSeparator();
  QAction *safety_to_wp_group = new QAction("&Safety zone -> Waypoints Group", this);
  app_menu->addAction(safety_to_wp_group);
  app_menu->addSeparator();
  QAction *patrol_add = new QAction("&Add Patrol", this);
  QAction *patrol_del = new QAction("&Delete Patrol", this);
  app_menu->addAction(patrol_add);
  app_menu->addAction(patrol_del);
  // associated callbacks
  QObject::connect(wp_group_new, SIGNAL(triggered()), this, SLOT(wpGroupNew()));
  wp_group_edit->setEnabled(false);
  wp_group_del->setEnabled(false);
  QObject::connect(waypoints_add, SIGNAL(triggered()), this, SLOT(waypointsAdd()));
  QObject::connect(waypoints_move, SIGNAL(triggered()), this, SLOT(waypointsMove()));
  QObject::connect(waypoints_del, SIGNAL(triggered()), this, SLOT(waypointsDelete()));
  //  waypoints_del->setEnabled(false);
  safety_to_wp_group->setEnabled(false);
  //patrol_add->setEnabled(false);patrolAdd
  QObject::connect(patrol_add, SIGNAL(triggered()), this, SLOT(patrolAdd()));
  patrol_del->setEnabled(false);

  // Agent model menu
  // ----------------
  app_menu = menuBar()->addMenu("&Agent models");
  QAction *agent_model_new = new QAction("New model", this);
  QAction *agent_model_open = new QAction("Open model", this);
  QAction *agent_model_save = new QAction("Save model", this);
  QAction *agent_model_save_as = new QAction("Save model as", this);
  QAction *agent_model_edit = new QAction("Edit model", this);
  app_menu->addAction(agent_model_new);
  //app_menu->addAction(agent_model_open);
  //app_menu->addAction(agent_model_save);
  //app_menu->addAction(agent_model_save_as);
  app_menu->addAction(agent_model_edit);
  // TODO Callback
  agent_model_new->setEnabled(false);
  agent_model_edit->setEnabled(false);

  app_menu = menuBar()->addMenu("&Agents");
  QAction *agent_add = new QAction("&Add agent", this);
  QAction *agent_edit = new QAction("&Edit agent", this);
  QAction *agent_del = new QAction("&Delete agent", this);
  QAction *agent_zone = new QAction("Define &Safety zone", this);
  app_menu->addAction(agent_add);
  app_menu->addAction(agent_edit);
  app_menu->addAction(agent_del);
  app_menu->addAction(agent_zone);

  agent_add->setEnabled(false);
  QObject::connect(agent_edit, SIGNAL(triggered()), this, SLOT(agentEdit()));
  agent_del->setEnabled(false);
  QObject::connect(agent_zone, SIGNAL(triggered()), this, SLOT(agentZone()));

  // Mission menu
  QAction *observation_points_add = new QAction("&Add points", this);
  QAction *observation_points_move = new QAction("&Move points", this);
  QAction *observation_points_del = new QAction("&Delete points", this);
  QAction *communication_add = new QAction("&Add", this);
  QAction *communication_edit = new QAction("&Edit", this);
  QAction *communication_del = new QAction("&Delete", this);
  QAction *generate_data = new QAction("&Generate Hipop data", this);

  app_menu = menuBar()->addMenu("&Mission");
  sub_menu = app_menu->addMenu("Observations");
  {
    sub_menu->addAction(observation_points_add);
    sub_menu->addAction(observation_points_move);
    sub_menu->addAction(observation_points_del);
  }

  sub_menu = app_menu->addMenu("Comunications");
  {
    sub_menu->addAction(communication_add);
    sub_menu->addAction(communication_edit);
    sub_menu->addAction(communication_del);
  }
  app_menu->addAction(generate_data);

  QObject::connect(observation_points_add, SIGNAL(triggered()), this, SLOT(observationPointsAdd()));
  QObject::connect(observation_points_move, SIGNAL(triggered()), this, SLOT(observationPointsMove()));
  QObject::connect(observation_points_del, SIGNAL(triggered()), this, SLOT(observationPointsDelete()));

  communication_add->setEnabled(false);
  communication_edit->setEnabled(false);
  communication_del->setEnabled(false);
  QObject::connect(generate_data, SIGNAL(triggered()), this, SLOT(generateHipopData()));
}

//-----------------------------------------------------------------------------
// check if mission alread exist and ask to erase it if necessary
// return true if mission_model_ is free
bool MainWindow::tryEraseExistingMission()
{
  if(mission_exist) {
    QMessageBox msgBox(this);
    msgBox.setText("A mission model already exist:");
    msgBox.setInformativeText("erase current mission model?");
    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    int ret = msgBox.exec();
    if(ret == QMessageBox::Cancel)
      return false;
    mission_exist = false;
    mission_model->reset();
  }
  return true;
}

//-----------------------------------------------------------------------------
void MainWindow::missionNew()
{
  if(!tryEraseExistingMission()) {
    return;
  }

  MissionDialog dialog(this);
  dialog.initData(mission_model);
  if(dialog.exec() == 0) {
    mission_exist = false;
    return;
  }
  dialog.storeData(mission_model);
  mission_viewer->updateMap();
  mission_exist = true;
}

//-----------------------------------------------------------------------------
void MainWindow::missionEdit()
{
  if(!mission_exist) {
    QMessageBox msgBox(this);
    msgBox.setText("You should first create a new mission");
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
    return;
  }

  MissionDialog dialog(this);
  dialog.initData(mission_model);
  if(dialog.exec() != 0) {
    dialog.storeData(mission_model);
    mission_viewer->updateMap();
  }
}

//-----------------------------------------------------------------------------
void MainWindow::missionOpen()
{
  if(!tryEraseExistingMission())
    return;

  QString fileName = QFileDialog::getOpenFileName(this);
  if(fileName != "") {
    loadFile(fileName);
  }
  else {
    QMessageBox msgBox(this);
    msgBox.setText("Unable to read mission file");
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
  }
}

//-----------------------------------------------------------------------------
void MainWindow::missionReload()
{
  loadFile(filename);
}

void MainWindow::loadFile(const QString& fileName)
{

  if(mission_model->read_json(fileName.toStdString())) {
    mission_exist = true;
    filename = fileName;

    // test map size
    double dx = mission_model->map_data.x_max - mission_model->map_data.x_min;
    double dy = mission_model->map_data.y_max - mission_model->map_data.y_min;
    if((dx < 1) || (dy < 1)) {
      QMessageBox msgBox(this);
      msgBox.setText("Warning : map size not consistant\n--> try to fixe it with extremum value");
      msgBox.setStandardButtons(QMessageBox::Ok);
      msgBox.exec();

      mission_model->map_data.x_max = 0.0;
      mission_model->map_data.x_min = 0.0;
      mission_model->map_data.y_max = 0.0;
      mission_model->map_data.y_min = 0.0;
      for(auto wp_group : mission_model->wp_groups)
        for(auto point_entry : wp_group.second.waypoints) {
          mission::point3_t point = point_entry.second;
          if(point.x > mission_model->map_data.x_max)
            mission_model->map_data.x_max = point.x;
          if(point.x < mission_model->map_data.x_min)
            mission_model->map_data.x_min = point.x;
          if(point.y > mission_model->map_data.y_max)
            mission_model->map_data.y_max = point.y;
          if(point.y < mission_model->map_data.y_min)
            mission_model->map_data.y_min = point.y;
          std::cout << "(" << point.x << ", " << point.y << ")   ";
          std::cout << "[" << mission_model->map_data.x_min << ", " << mission_model->map_data.x_max << ", "
              << mission_model->map_data.y_min << ", " << mission_model->map_data.y_max << "]" << std::endl;

        }
    }

    // map image file existe ?
    QString test_filename = mission_model->get_home_dir().filePath(
        QString::fromStdString(mission_model->map_data.image));
    std::cout << "Testing " << test_filename.data() << std::endl;
    if(!QFileInfo(test_filename).exists()) {
      QMessageBox msgBox(this);
      msgBox.setText("Warning : enable to read map files\nTry setting a different home directory...");
      msgBox.setStandardButtons(QMessageBox::Ok);
      msgBox.exec();
    }

    missionEdit();
    mission_viewer->updateControl();
    mission_viewer->updateMap();
    return;
  }
}

//-----------------------------------------------------------------------------
void MainWindow::missionSave()
{
  if(!mission_exist) {
    QMessageBox msgBox(this);
    msgBox.setText("No mission created");
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
    return;
  }

  QString a_filename = filename;

  if(a_filename == "")
    a_filename = QFileDialog::getSaveFileName(this);

  if(a_filename == "")
    std::cout << "Function canceled\n";
  else {
    //std::cout << "Save : " << a_filename.toStdString() << std::endl;
    if(!mission_model->write_json(a_filename.toStdString())) {
      //std::cout << "Error while reading mission file\n";
      //delete mission_data->model;
      QMessageBox msgBox(this);
      msgBox.setText("Unable to save mission file");
      msgBox.setStandardButtons(QMessageBox::Ok);
      msgBox.exec();
    }
    else
      filename = a_filename;
  }
}

//-----------------------------------------------------------------------------
void MainWindow::missionSaveAs()
{
  if(!mission_exist) {
    QMessageBox msgBox(this);
    msgBox.setText("No mission created");
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
    return;
  }

  QString a_filename = QFileDialog::getSaveFileName(this);
  if(a_filename == "")
    std::cout << "Function canceled\n";
  else {
    std::cout << "Save as : " << a_filename.toStdString() << std::endl;
    if(!mission_model->write_json(a_filename.toStdString())) {
      //std::cout << "Error while reading mission file\n";
      //delete mission_data->model;

      QMessageBox msgBox(this);
      msgBox.setText("Unable to write mission file");
      msgBox.setStandardButtons(QMessageBox::Ok);
      msgBox.exec();
    }
    else
      filename = a_filename;
  }
}

//-----------------------------------------------------------------------------
void MainWindow::wpGroupNew()
{
  WpGroupDialog dialog(this, WpGroupDialog::NEW);

  if(dialog.exec()) {
    std::string name = dialog.getName();      //.toStdString();

    /*
     QMessageBox msg(this);
     msg.setText("Select waypoints on map\n (Left click to confirm)\n"
     );//+dialog.getName());
     msg.setStandardButtons(QMessageBox::Ok);
     msg.exec();
     */

    dialog.addWpGroup(mission_model);
    mission_model->wp_groups[name].waypoint_index = 1;
    mission_model->wp_groups[name].patrol_index = 1;
    mission_viewer->updateControl();
    mission_viewer->getMapViewer()->actionMode(MapViewer::Add_Waypoints, name);
  }
}

//-----------------------------------------------------------------------------
void MainWindow::waypointsAdd()
{
  QInputDialog inputBox(this);
  QStringList items;
  for(auto wp_group : mission_model->wp_groups)
    items.append(QString::fromStdString(wp_group.first));
  inputBox.setComboBoxItems(items);

  if(inputBox.exec() != 0) {
    std::string name = inputBox.textValue().toStdString();
    std::cout << "Add waypoints to " << name << std::endl;
    mission_viewer->getMapViewer()->actionMode(MapViewer::Add_Waypoints, name);
  }
  else
    std::cout << "Outch !!\n";
}

//-----------------------------------------------------------------------------
void MainWindow::waypointsMove()
{
  QInputDialog inputBox(this);
  QStringList items;
  for(auto wp_group : mission_model->wp_groups)
    items.append(QString::fromStdString(wp_group.first));
  inputBox.setComboBoxItems(items);

  if(inputBox.exec() != 0) {
    std::string name = inputBox.textValue().toStdString();
    std::cout << "Move waypoints of " << name << std::endl;
    mission_viewer->getMapViewer()->actionMode(MapViewer::Move_Waypoints, name);
  }
  else
    std::cout << "Outch !!\n";
}

//-----------------------------------------------------------------------------
void MainWindow::waypointsDelete()
{
  QInputDialog inputBox(this);      //,"Delete Waypoints");
  QStringList items;
  for(auto wp_group : mission_model->wp_groups)
    items.append(QString::fromStdString(wp_group.first));
  inputBox.setComboBoxItems(items);

  if(inputBox.exec() != 0) {
    std::string name = inputBox.textValue().toStdString();
    std::cout << "Delete waypoints of " << name << std::endl;
    mission_viewer->setFilter(name);
    mission_viewer->getMapViewer()->actionMode(MapViewer::Delete_Waypoints, name);
  }
  else
    std::cout << "Outch !!\n";
}

//-----------------------------------------------------------------------------
void MainWindow::patrolAdd()
{
  QInputDialog inputBox(this);
  QStringList items;
  for(auto wp_group : mission_model->wp_groups)
    items.append(QString::fromStdString(wp_group.first));
  inputBox.setComboBoxItems(items);

  if(inputBox.exec() != 0) {
    std::string name = inputBox.textValue().toStdString();
    std::cout << "Add patrol to " << name << std::endl;
    //mission_viewer->setFilter(name);
    mission_viewer->getMapViewer()->actionMode(MapViewer::Patrol_Add, name);
  }
  else
    std::cout << "Outch !!\n";
}

//-----------------------------------------------------------------------------
void MainWindow::agentEdit()
{
  if(!mission_exist) {
    QMessageBox msgBox(this);
    msgBox.setText("You should first create a new mission");
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
    return;
  }

  AgentDialog dialog(this);
  dialog.initData(mission_model);
  if(dialog.exec() != 0) {
    //dialog.storeData(mission_model);
  }
}

//-----------------------------------------------------------------------------
void MainWindow::agentZone()
// define safety zone
{
  QInputDialog inputBox(this);
  QStringList items;
  for(auto agent : mission_model->agents)
    items.append(QString::fromStdString(agent.first));
  inputBox.setComboBoxItems(items);

  if(inputBox.exec() != 0) {
    std::string agent_name = inputBox.textValue().toStdString();
    std::cout << "Edit safety zone of agent " << agent_name << std::endl;
    mission_model->agents[agent_name].safety_zone.clear();
    mission_viewer->getMapViewer()->update();
    mission_viewer->getMapViewer()->actionMode(MapViewer::Add_Safety_Zone, agent_name);
  }
  else
    std::cout << "Outch !!\n";
}

//-----------------------------------------------------------------------------
void MainWindow::observationPointsAdd()
{
  mission_viewer->getMapViewer()->actionMode(MapViewer::Observations_Add, "");
}

//-----------------------------------------------------------------------------
void MainWindow::observationPointsMove()
{
  mission_viewer->getMapViewer()->actionMode(MapViewer::Observations_Move, "");
}

//-----------------------------------------------------------------------------
void MainWindow::observationPointsDelete()
{
  mission_viewer->getMapViewer()->actionMode(MapViewer::Observations_Delete, "");
}

//-----------------------------------------------------------------------------
void MainWindow::generateHipopData()
{
  QString outputFolder = filename.split(".")[0];

  QMessageBox msgBox(this);
  msgBox.setText("The output will be written in " + outputFolder + "\n\n The previous content will be deleted");
  msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
  msgBox.setDefaultButton(QMessageBox::Cancel);
  int ret = msgBox.exec();
  if(ret == QMessageBox::Cancel)
    return;

  std::cout << "Start script" << std::endl;

  hipop_launcher = new QProcess(this);
  QStringList arguments;
  arguments.push_back("--force");
  arguments.push_back("--outputFolder");
  arguments.push_back(filename.split(".")[0]);
  arguments.push_back(filename);
  hipop_launcher->start("actionGenerator", arguments);
  hipop_launcher->setProcessChannelMode(QProcess::MergedChannels);
  connect(hipop_launcher, SIGNAL(readyReadStandardOutput()), this, SLOT(processHipopOutput()));
  connect(hipop_launcher, SIGNAL(readyReadStandardError()), this, SLOT(processHipopOutput()));

  hipop_launcher->waitForFinished();
  int errorCode = hipop_launcher->exitCode();
  if(errorCode == 0) {
    std::cout << "Script exited normally" << std::endl;
  }
  else {
    std::cout << "Script exited with error code : " << errorCode << std::endl;
  }
}

//-----------------------------------------------------------------------------
void MainWindow::processHipopOutput()
{
  QByteArray output = hipop_launcher->readAllStandardOutput();
  QByteArray error = hipop_launcher->readAllStandardError();
  std::cout << output.data();
  std::cout << error.data();
}

