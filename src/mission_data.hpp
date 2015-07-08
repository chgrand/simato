//-*-c++-*-

#ifndef MISSION_DATA_HPP
#define MISSION_DATA_HPP

class MissionData
{
public:
  QString filename;
  bool exist;
  MissionModel *model;

  MissionData() : filename(""), exist(false), model(nullptr){};

  ~MissionData()
  {
    if(exist)
      delete model;
  }

};

#endif
