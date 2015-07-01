#include <iostream>
#include "colors.h"
#include "wp_group_dialog.h"



//-----------------------------------------------------------------------------
WpGroupDialog::WpGroupDialog(QWidget *parent, int mode):
  QDialog(parent)
{
  name = new QLineEdit();

  color_name = new QComboBox();
  color_name->setInsertPolicy(QComboBox::InsertAtBottom);
  for(auto & color_item: ColorMap::OrderedList)
    color_name->addItem(QString::fromStdString(color_item));

  marker_char = new QComboBox();
  marker_char->addItem("X");
  marker_char->addItem("+");
  marker_char->addItem("O");
  marker_char->addItem("*");

  QFormLayout *layout = new QFormLayout;
  if(mode==WpGroupDialog::NEW)
    layout->addRow(tr("Name:"), name);
  layout->addRow(tr("Color:"), color_name);
  layout->addRow(tr("Marker:"), marker_char);

  // Dialog exit button
  QDialogButtonBox *buttonBox = new QDialogButtonBox;
  buttonBox->addButton(tr("Confirm"), QDialogButtonBox::AcceptRole);
  buttonBox->addButton(tr("Cancel"), QDialogButtonBox::RejectRole);
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainLayout->addSpacing(12);
  mainLayout->addLayout(layout);
  mainLayout->addStretch(1);
  mainLayout->addWidget(buttonBox);
  setLayout(mainLayout);

  if(mode==WpGroupDialog::NEW)
    setWindowTitle(tr("New waypoints group"));
  //setMinimumWidth(480);
  //setMinimumHeight(280);
  setModal(true);
}


/*
//-----------------------------------------------------------------------------
void WpGroupDialog::initData(MissionModel *model)
{ 
  for(auto& color_item : ColorMap::CMap)
    color->addItem(QString::fromStdString(color_item.first));
}
*/

//-----------------------------------------------------------------------------
void WpGroupDialog::addWpGroup(MissionModel *model)
{
  std::string wp_name = name->text().toStdString();
  if(wp_name=="") return;

  model->wp_groups[wp_name].color = color_name->currentText().toStdString();
  model->wp_groups[wp_name].marker = marker_char->currentText().at(0).toLatin1();
  model->wp_groups[wp_name].waypoints.clear();
}


