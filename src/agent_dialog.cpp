#include <iostream>
#include <string>
#include <vector>
#include <map>
//#include <boost/foreach.hpp>
#include "colors.h"
#include "agent_dialog.h"

//-----------------------------------------------------------------------------
AgentDialog::AgentDialog(QWidget *parent):
  QDialog(parent)
{
  agent_name = new QComboBox();
  agent_name->setEditable(true);
  agent_name->setInsertPolicy(QComboBox::InsertAlphabetically);
  
  model_edit = new QLineEdit();
  //model_edit->setMaximumWidth(80);
  model_btn  = new QPushButton("Select");
  QHBoxLayout *model_layout=new QHBoxLayout;
  model_layout->addWidget(model_edit);
  model_layout->addWidget(model_btn);
  
  color_name = new QComboBox();
  color_name->setInsertPolicy(QComboBox::InsertAtBottom);
  for(auto & color_item: ColorMap::OrderedList)
    color_name->addItem(QString::fromStdString(color_item));

  marker = new QComboBox();
  marker->addItem("X");
  marker->addItem("+");
  marker->addItem("O");
  marker->addItem("*");


   // Dialog exit button
  QDialogButtonBox *buttonBox = new QDialogButtonBox;
  buttonBox->addButton(tr("Confirm"), QDialogButtonBox::AcceptRole);
  buttonBox->addButton(tr("Cancel"), QDialogButtonBox::RejectRole);

  // Signals callbacks
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));


  
  QFormLayout *layout = new QFormLayout;
  layout->addRow(tr("Name:"), agent_name);
  layout->addRow(tr("Model:"), model_layout);
  layout->addRow(tr("Color:"), color_name);
  layout->addRow(tr("Marker:"), marker);
  //model
  //color
  //marker
  //position
  //energy
  //wp_group
  //auuthorized_com??

  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainLayout->addSpacing(18);
  mainLayout->addLayout(layout);
  mainLayout->addStretch(1);
  mainLayout->addWidget(buttonBox);
  setLayout(mainLayout);

  setWindowTitle(tr("Agents configuration"));
  setMinimumWidth(480);
  //setMinimumHeight(280);
  setModal(true);
}

//-----------------------------------------------------------------------------
void AgentDialog::initData(MissionModel *mission_model)
{
  model = mission_model;
  //  BOOST_FOREACH() {
  // }

  for(auto& agent_item : model->agents) {
    std::cout << agent_item.first << std::endl; 
    agent_name->addItem(QString::fromStdString(agent_item.first));
  }

  /*
  std::map<std::string,mission::agent_t>::const_iterator 
    mit (model->agents.begin()), 
    mend(model->agents.end()); 
  for(;mit!=mend;++mit) {
    std::cout << mit->first << std::endl; 
    agent_name->addItem(QString::fromStdString(mit->first));
  } 
  */ 
}

//-----------------------------------------------------------------------------
void AgentDialog::storeData(MissionModel *mission_model)
{
  

}
