//-*-c++-*-
#ifndef MISSION_MODEL_H
#define MISSION_MODEL_H

#include <string>
#include <vector>
#include <map>

#include <QDir>

using namespace std;

namespace mission
{
  typedef struct
  {
    double x, y, z;
  } point3_t;

  typedef struct
  {
    double x, y, z, t;
  } point4_t;

  typedef struct
  {
    string image;
    string dtm_geotiff;
    string region_geotiff;
    string blender;
    double x_min, x_max, y_min, y_max;
  } map_data_t;

  typedef struct
  {
    point3_t position;
    //orientation ?
    double range;
    double fov;
  } instrument_t;

  typedef struct
  {
    string color;
  } target_t;

  typedef struct
  {
    string type;
    double velocity;
    instrument_t sensor;
    instrument_t antenna;
    //string morse_filename; TODO
  } agent_model_t;

  typedef struct
  {
    string color;
    char marker;
    double energy;
    point3_t initial_position;
    point3_t current_position; // local variable
    std::string model_name;
    std::string wp_group_name;
    std::vector<point3_t> safety_zone;
    std::vector<int> autorized_comm;
    bool spare;
  } agent_t;

  typedef struct
  {
    string config_file;
    string region_file;
  } model_t;

  typedef struct
  {
    string agent_1;
    string agent_2;
    double date;
    std::string waypoint_1;
    std::string waypoint_2;
  } comm_t;

  typedef struct
  {
    char marker;
    std::string color;
    std::map<std::string, point3_t> waypoints;
    std::map<std::string, vector<std::string> > patrols;
    int waypoint_index; // local variable not saved in json file
    int patrol_index;    // local variable not saved in json file
  } wp_group_t;

  // == TODO ==
  // actions of generated plan
  typedef struct
  {
    string action;
    string agent[2];
    point3_t position[2];
    double t_start;
    double t_end;
  } elementary_action_t;
}

class MissionModel
{
public:
  // mission data
  double max_time;
  mission::map_data_t map_data;
  std::map<std::string, mission::model_t> models;
  std::map<std::string, mission::target_t> targets;
  std::map<std::string, mission::agent_t> agents;
  std::map<std::string, mission::wp_group_t> wp_groups;
  std::map<std::string, mission::point3_t> observations;
  int observation_points_counter;
  std::map<string, mission::comm_t> comm_goals;

  // generated plan    
  //vector<elementary_action_t> generated_plan;

  MissionModel();
  ~MissionModel();
  bool read_json(string filename);  // template mission.json
  bool write_json(string filename);
  void reset();

  QDir get_home_dir() const; //return a home dir with environement variable expanded
  std::string get_raw_home_dir() const
  {
    return home_dir;
  } //return a home dir with environement variable expanded
  void set_home_dir(const std::string& s)
  {
    home_dir = s;
  }
private:
  std::string home_dir;
};

#endif
