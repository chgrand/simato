#include "mission_dialog.h"

//---------------------------------------------------------------------------------------
MissionDialog::MissionDialog(QWidget *parent) :
    QDialog(parent)
{
  maxtime_edit = new QLineEdit("10.0");
  maxtime_edit->setValidator(new QDoubleValidator());
  maxtime_edit->setMaximumWidth(80);

  home_edit = new QLineEdit("/home/christophe/");
  home_btn = new QPushButton("Select");
  QHBoxLayout *home_layout = new QHBoxLayout;
  home_layout->addWidget(home_edit);
  home_layout->addWidget(home_btn);

  map_image_edit = new QLineEdit();
  map_image_btn = new QPushButton("Select");
  QHBoxLayout *map_image_layout = new QHBoxLayout;
  map_image_layout->addWidget(map_image_edit);
  map_image_layout->addWidget(map_image_btn);

  map_region_edit = new QLineEdit();
  map_region_btn = new QPushButton("Select");
  QHBoxLayout *map_region_layout = new QHBoxLayout;
  map_region_layout->addWidget(map_region_edit);
  map_region_layout->addWidget(map_region_btn);

  map_dtm_edit = new QLineEdit();
  map_dtm_btn = new QPushButton("Select");
  QHBoxLayout *map_dtm_layout = new QHBoxLayout;
  map_dtm_layout->addWidget(map_dtm_edit);
  map_dtm_layout->addWidget(map_dtm_btn);

  map_blender_edit = new QLineEdit();
  map_blender_btn = new QPushButton("Select");
  QHBoxLayout *map_blender_layout = new QHBoxLayout;
  map_blender_layout->addWidget(map_blender_edit);
  map_blender_layout->addWidget(map_blender_btn);

  {
    QDoubleSpinBox *w = new QDoubleSpinBox();
    w->setMaximumWidth(160);
    w->setRange(-1000, 1000);
    map_size_xmin = w;
  }
  {
    QDoubleSpinBox *w = new QDoubleSpinBox();
    w->setMaximumWidth(160);
    w->setRange(-1000, 1000);
    map_size_xmax = w;
  }
  {
    QDoubleSpinBox *w = new QDoubleSpinBox();
    w->setMaximumWidth(160);
    w->setRange(-1000, 1000);
    map_size_ymin = w;
  }
  {
    QDoubleSpinBox *w = new QDoubleSpinBox();
    w->setMaximumWidth(160);
    w->setRange(-1000, 1000);
    map_size_ymax = w;
  }

  QGridLayout *map_size_layout = new QGridLayout;
  map_size_layout->setAlignment(Qt::AlignHCenter);
  map_size_layout->addWidget(new QLabel("X min"), 0, 0, 1, 1);
  map_size_layout->addWidget(new QLabel("X max"), 0, 1, 1, 1);
  map_size_layout->addWidget(new QLabel("Y min"), 0, 2, 1, 1);
  map_size_layout->addWidget(new QLabel("Y max"), 0, 3, 1, 1);
  map_size_layout->addWidget(map_size_xmin, 1, 0, 1, 1);
  map_size_layout->addWidget(map_size_xmax, 1, 1, 1, 1);
  map_size_layout->addWidget(map_size_ymin, 1, 2, 1, 1);
  map_size_layout->addWidget(map_size_ymax, 1, 3, 1, 1);

  // Dialog exit button
  QDialogButtonBox *buttonBox = new QDialogButtonBox;
  buttonBox->addButton(tr("Confirm"), QDialogButtonBox::AcceptRole);
  buttonBox->addButton(tr("Cancel"), QDialogButtonBox::RejectRole);
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

  // Signals callbacks
  connect(home_btn, SIGNAL(clicked()), this, SLOT(selectHomeDir()));
  connect(map_image_btn, SIGNAL(clicked()), this, SLOT(selectFile()));
  connect(map_region_btn, SIGNAL(clicked()), this, SLOT(selectFile()));
  connect(map_dtm_btn, SIGNAL(clicked()), this, SLOT(selectFile()));

  // Form layout
  QFormLayout *layout = new QFormLayout;
  layout->addRow(tr("Max time:"), maxtime_edit);
  layout->addRow(tr("Home path:"), home_layout);
  layout->addRow(tr("Map image:"), map_image_layout);
  layout->addRow(tr("Map region:"), map_region_layout);
  layout->addRow(tr("Map dtm:"), map_dtm_layout);

  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainLayout->addSpacing(18);
  mainLayout->addLayout(layout);
  mainLayout->addSpacing(12);
  mainLayout->addLayout(map_size_layout);
  mainLayout->addStretch(1);
  mainLayout->addWidget(buttonBox);
  setLayout(mainLayout);

  setWindowTitle(tr("Mission setup"));
  setMinimumWidth(480);
  setMinimumHeight(280);
  setModal(true);

}
;

//---------------------------------------------------------------------------------------
void MissionDialog::selectHomeDir()
{
  QString directory = QFileDialog::getExistingDirectory(this);
  if(directory != "") {
    directory += QString::fromStdString("/");
    home_edit->setText(directory);
  }
}

//---------------------------------------------------------------------------------------
void MissionDialog::selectFile()
{
  QPushButton *button = (QPushButton *) sender();
  QString home_path = home_edit->text();
  QString globalFileName = QFileDialog::getOpenFileName(this, "Select file", home_path);
  QString localFileName;

  if(globalFileName != "") {
    if(globalFileName.startsWith(home_path))
      localFileName = globalFileName.remove(home_path);

    if(button == map_image_btn)
      map_image_edit->setText(globalFileName);
    if(button == map_region_btn)
      map_region_edit->setText(globalFileName);
    if(button == map_dtm_btn)
      map_dtm_edit->setText(globalFileName);
  }
}

//---------------------------------------------------------------------------------------
void MissionDialog::initData(MissionModel *mission_model)
{
  maxtime_edit->setText(QString::number(mission_model->max_time));
  home_edit->setText(QString::fromStdString(mission_model->get_raw_home_dir()));
  map_image_edit->setText(QString::fromStdString(mission_model->map_data.image));
  map_region_edit->setText(QString::fromStdString(mission_model->map_data.region_geotiff));
  map_dtm_edit->setText(QString::fromStdString(mission_model->map_data.dtm_geotiff));
  map_blender_edit->setText(QString::fromStdString(mission_model->map_data.blender));

  map_size_xmin->setValue(mission_model->map_data.x_min);
  map_size_xmax->setValue(mission_model->map_data.x_max);
  map_size_ymin->setValue(mission_model->map_data.y_min);
  map_size_ymax->setValue(mission_model->map_data.y_max);
}

//---------------------------------------------------------------------------------------
void MissionDialog::storeData(MissionModel *mission_model)
{
  mission_model->max_time = maxtime_edit->text().toDouble();
  mission_model->set_home_dir(home_edit->text().toStdString());

  mission_model->map_data.image = map_image_edit->text().toStdString();
  mission_model->map_data.dtm_geotiff = map_dtm_edit->text().toStdString();
  mission_model->map_data.region_geotiff = map_region_edit->text().toStdString();
  mission_model->map_data.blender = map_blender_edit->text().toStdString();

  mission_model->map_data.x_min = map_size_xmin->value();
  mission_model->map_data.x_max = map_size_xmax->value();
  mission_model->map_data.y_min = map_size_ymin->value();
  mission_model->map_data.y_max = map_size_ymax->value();

}
