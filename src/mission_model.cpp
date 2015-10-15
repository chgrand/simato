#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <cassert>
#include <exception>

// json reader
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>

#include "mission_model.h"

using namespace mission;

//-----------------------------------------------------------------------------
MissionModel::MissionModel()
{
  reset();
}

//-----------------------------------------------------------------------------
MissionModel::~MissionModel()
{
  reset();
}

//-----------------------------------------------------------------------------
void MissionModel::reset()
{
  max_time = 0.0;
  home_dir = "";
  map_data.image = "";
  map_data.dtm_geotiff = "";
  map_data.region_geotiff = "";
  map_data.blender = "";
  map_data.x_min = 0.0;
  map_data.x_max = 0.0;
  map_data.y_min = 0.0;
  map_data.y_max = 0.0;

  models.clear();
  agents.clear();
  wp_groups.clear();
  observations.clear();
  observation_points_counter = 1;
  comm_goals.clear();
}

//-----------------------------------------------------------------------------
bool MissionModel::read_json(std::string filename)
{
  std::cout << "=========== Reading json file ===============\n";
  try {
    boost::property_tree::ptree main_tree;
    boost::property_tree::read_json(filename, main_tree);

    // general data
    // ------------
    max_time = main_tree.get<double>("max_time", 0);
    home_dir = main_tree.get<std::string>("home_dir", "");

    map_data.image = main_tree.get<std::string>("map_data.image_file");
    map_data.dtm_geotiff = main_tree.get<std::string>("map_data.dtm_file");
    map_data.region_geotiff = main_tree.get<std::string>("map_data.region_file");
    map_data.blender = main_tree.get<std::string>("map_data.blender_file");
    map_data.x_min = main_tree.get<double>("map_data.map_size.x_min");
    map_data.x_max = main_tree.get<double>("map_data.map_size.x_max");
    map_data.y_min = main_tree.get<double>("map_data.map_size.y_min");
    map_data.y_max = main_tree.get<double>("map_data.map_size.y_max");

    // TODO mission_zone

    // read targets data
    // ----------------
    boost::optional<boost::property_tree::ptree&> targets_child = main_tree.get_child_optional("targets");

    if(targets_child) {
      for(auto target : targets_child.get()) {
        mission::target_t a_target;
        a_target.color = target.second.get<std::string>("color", "red");
        targets[target.first] = a_target;
      }
    }

    // read agents data
    // ----------------
    boost::optional<boost::property_tree::ptree&> agents_child = main_tree.get_child_optional("agents");

    if(agents_child) {
      for(auto agent : agents_child.get()) {
        mission::agent_t an_agent;
        an_agent.color = agent.second.get<std::string>("color", "green");
        an_agent.marker = agent.second.get<char>("marker", 'X');
        an_agent.initial_position.x = agent.second.get<double>("position.x", 0.0);
        an_agent.initial_position.y = agent.second.get<double>("position.y", 0.0);
        an_agent.initial_position.z = agent.second.get<double>("position.z", 0.0);
        //TODO position
        an_agent.model_name = agent.second.get<std::string>("model", "");
        an_agent.energy = agent.second.get<double>("energy", 0.0);
        an_agent.wp_group_name = agent.second.get<std::string>("wp_group", "");
        //TODO authorized_comm

        an_agent.safety_zone.clear();
        boost::optional<boost::property_tree::ptree&> safety_zone_child = agent.second.get_child_optional(
            "safety_zone");
        if(safety_zone_child) {
          for(auto point : safety_zone_child.get()) {
            mission::point3_t a_point;
            assert(point.first.empty()); // array elements have no names
            a_point.x = point.second.get<double>("x", 0);
            a_point.y = point.second.get<double>("y", 0);
            a_point.z = point.second.get<double>("z", 0);
            an_agent.safety_zone.push_back(a_point);
          }
        }
        else {
          std::cout << "[WARNING] No safety zone for agent" << agent.first << std::endl;
        }
        an_agent.spare = agent.second.get<bool>("spare", false);

        agents[agent.first] = an_agent;
      }
    }
    else
      std::cout << "[Warning] : no agent in file" << std::endl;

    // Read model info
    // --------------
    boost::optional<boost::property_tree::ptree&> models_entry = main_tree.get_child_optional("models");

    if(models_entry) {
      for(auto model : models_entry.get()) {
        mission::model_t a_model;
        a_model.config_file = model.second.get<std::string>("config_file", "");
        a_model.region_file = model.second.get<std::string>("region_file", "");
        models[model.first] = a_model;
      }
    }
    else
      std::cout << "Warning: no models in mission files" << std::endl;

    // read wp_groups data
    // ----------------
    wp_groups.clear(); // clear data map
    boost::optional<boost::property_tree::ptree&> wp_groups_child = main_tree.get_child_optional("wp_groups");

    if(wp_groups_child) {
      for(auto wp_group : wp_groups_child.get()) {
        mission::wp_group_t a_group;
        a_group.marker = wp_group.second.get<char>("marker", 'X');
        a_group.color = wp_group.second.get<string>("color", "green");

        // read waypoints
        boost::optional<boost::property_tree::ptree&> waypoints_child = wp_group.second.get_child_optional("waypoints");
        //a_group.waypoints.clear(); // clear data map	
        if(waypoints_child) {
          a_group.waypoint_index = 1;
          for(auto point : waypoints_child.get()) {
            a_group.waypoints[point.first].x = point.second.get<double>("x", 0.);
            a_group.waypoints[point.first].y = point.second.get<double>("y", 0.);
            a_group.waypoints[point.first].z = point.second.get<double>("z", 0.);
            a_group.waypoint_index = std::max(a_group.waypoint_index, 1 + std::stoi(point.first));
          }
        }

        // read patrols
        boost::optional<boost::property_tree::ptree&> patrols_child = wp_group.second.get_child_optional("patrols");
        //a_group.patrols.clear();
        if(patrols_child) {
          a_group.patrol_index = 1;
          for(auto patrol : patrols_child.get()) {
            a_group.patrols[patrol.first].clear();
            for(auto index : patrol.second) {
              assert(index.first.empty()); // array elements have no names
              a_group.patrols[patrol.first].push_back(index.second.data());
              a_group.patrol_index = std::max(a_group.patrol_index, 1 + std::stoi(patrol.first));
            }
          }
        }
        wp_groups[wp_group.first] = a_group;
      }
    }
    else
      std::cout << "[Warning] : no waypoints groups in file" << std::endl;

    // read mission goal/observation_points
    // ------------------------------------
    observations.clear(); // clear data map
    boost::optional<boost::property_tree::ptree&> observations_entry = main_tree.get_child_optional(
        "mission_goal.observation_points");

    if(observations_entry) {
      observation_points_counter = 1;
      for(auto point : observations_entry.get()) {
        observations[point.first].x = point.second.get<double>("x", 0.);
        observations[point.first].y = point.second.get<double>("y", 0.);
        observations[point.first].z = point.second.get<double>("z", 0.);
        int index = std::stoi(point.first);
        if(observation_points_counter < index)
          observation_points_counter = index;
      }
      observation_points_counter++;
    }
    else
      std::cout << "[Warning] : no observation points in file" << std::endl;

    // mission goal/communication_goals
    // --------------------------------
    boost::optional<boost::property_tree::ptree&> comms_entry = main_tree.get_child_optional(
        "mission_goal.communication_goals");

    if(comms_entry) {
      for(auto comm : comms_entry.get()) {
        mission::comm_t a_comm;
        a_comm.agent_1 = comm.second.get<std::string>("agent1", "");
        a_comm.agent_2 = comm.second.get<std::string>("agent2", "");
        a_comm.date = comm.second.get<double>("date", 0.0);
        a_comm.waypoint_1 = comm.second.get<std::string>("wp_1", "");
        a_comm.waypoint_2 = comm.second.get<std::string>("wp_2", "");
        comm_goals[comm.first] = a_comm;
      }
    }
    else
      std::cout << "Warning: no communication goal in mission file" << std::endl;

    return true;
  } catch (std::exception const& e) {
    std::cerr << "Error while reading json file: ";
    std::cerr << e.what() << std::endl;
  }
  return false;
}

//-----------------------------------------------------------------------------
bool MissionModel::write_json(std::string filename)
{
  try {
    boost::property_tree::ptree p_map_data;
    p_map_data.put("image_file", map_data.image);
    p_map_data.put("region_file", map_data.region_geotiff);
    p_map_data.put("dtm_file", map_data.dtm_geotiff);
    p_map_data.put("blender_file", map_data.blender);
    p_map_data.put("map_size.x_min", map_data.x_min);
    p_map_data.put("map_size.x_max", map_data.x_max);
    p_map_data.put("map_size.y_min", map_data.y_min);
    p_map_data.put("map_size.y_max", map_data.y_max);

    boost::property_tree::ptree p_mission;

    p_mission.put("max_time", max_time);
    p_mission.put("home_dir", home_dir);
    p_mission.push_back(std::make_pair("map_data", p_map_data));

    // write target properties

    boost::property_tree::ptree pt_targets;
    for(auto target : this->targets) {
      boost::property_tree::ptree pt_target;
      std::string target_name = target.first;
      pt_target.put("color", target.second.color);
      pt_targets.push_back(std::make_pair(target_name, pt_target));
    }
    p_mission.push_back(std::make_pair("targets", pt_targets));
    // write agents properties
    // ------------------------------
    boost::property_tree::ptree pt_agents;
    for(auto agent : this->agents) {
      boost::property_tree::ptree pt_agent;
      std::string agent_name = agent.first;

      pt_agent.put("color", agent.second.color);
      pt_agent.put("marker", agent.second.marker);
      pt_agent.put("model", agent.second.model_name);
      pt_agent.put("wp_group", agent.second.wp_group_name);
      pt_agent.put("energy", agent.second.energy);
      {
        boost::property_tree::ptree pt_position;
        pt_position.put("x", agent.second.initial_position.x);
        pt_position.put("y", agent.second.initial_position.y);
        pt_position.put("z", agent.second.initial_position.z);
        pt_agent.push_back(std::make_pair("position", pt_position));
      }

      {
        boost::property_tree::ptree pt_zone;
        for(auto point : agent.second.safety_zone) {
          boost::property_tree::ptree pt_point;
          pt_point.put("x", point.x);
          pt_point.put("y", point.y);
          pt_point.put("z", point.z);
          pt_zone.push_back(std::make_pair("", pt_point));
        }
        pt_agent.push_back(std::make_pair("safety_zone", pt_zone));
      }

      pt_agent.put("spare", agent.second.spare);

      // TODO : authorized_comm

      pt_agents.push_back(std::make_pair(agent_name, pt_agent));
    }
    p_mission.push_back(std::make_pair("agents", pt_agents));

    // Models
    boost::property_tree::ptree models_dict;
    for(auto model : models) {
      boost::property_tree::ptree model_entry;
      model_entry.put("config_file", model.second.config_file);
      if(model.second.region_file != "")
        model_entry.put("region_file", model.second.region_file);
      models_dict.push_back(std::make_pair(model.first, model_entry));
    }
    p_mission.push_back(std::make_pair("models", models_dict));

    // write wp_groups set
    // ------------------------------
    boost::property_tree::ptree p_wp_groups;
    for(auto wp_group : wp_groups) {
      boost::property_tree::ptree p_group;

      std::string wp_group_name = wp_group.first;
      p_group.put("marker", wp_group.second.marker);
      p_group.put("color", wp_group.second.color);

      // write waypoints set of each wp_group
      {
        boost::property_tree::ptree pt_waypoints;
        for(auto point : wp_group.second.waypoints) {
          boost::property_tree::ptree pt_point;
          pt_point.put("x", point.second.x);
          pt_point.put("y", point.second.y);
          pt_point.put("z", point.second.z);
          pt_waypoints.push_back(std::make_pair(point.first, pt_point));
        }
        p_group.push_back(std::make_pair("waypoints", pt_waypoints));
      }

      // write patrols set  of each wp_group
      {
        boost::property_tree::ptree pt_patrols;
        for(auto patrol : wp_group.second.patrols) {
          boost::property_tree::ptree pt_patrol;
          for(auto index : patrol.second) {
            boost::property_tree::ptree pt_index;
            pt_index.put("", index);
            pt_patrol.push_back(std::make_pair("", pt_index));
          }
          pt_patrols.push_back(std::make_pair(patrol.first, pt_patrol));
        }
        p_group.push_back(std::make_pair("patrols", pt_patrols));
      }

      p_wp_groups.push_back(std::make_pair(wp_group_name, p_group));
    }
    p_mission.push_back(std::make_pair("wp_groups", p_wp_groups));

    // Mission Goal
    boost::property_tree::ptree pt_mission_goals;
    {
      // Observation points
      boost::property_tree::ptree pt_observations;
      for(auto point : observations) {
        boost::property_tree::ptree pt_point;
        pt_point.put("x", point.second.x);
        pt_point.put("y", point.second.y);
        pt_point.put("z", point.second.z);
        pt_observations.push_back(std::make_pair(point.first, pt_point));
      }
      pt_mission_goals.push_back(std::make_pair("observation_points", pt_observations));

      // Communication goals
      boost::property_tree::ptree comms_dict;
      for(auto comm : comm_goals) {
        boost::property_tree::ptree comm_entry;
        comm_entry.put("agent1", comm.second.agent_1);
        comm_entry.put("agent2", comm.second.agent_2);
        comm_entry.put("date", comm.second.date);
        if(comm.second.waypoint_1 != "")
          comm_entry.put("wp_1", comm.second.waypoint_1);

        if(comm.second.waypoint_2 != "")
          comm_entry.put("wp_2", comm.second.waypoint_2);

        comms_dict.push_back(std::make_pair(comm.first, comm_entry));
      }
      pt_mission_goals.push_back(std::make_pair("communication_goals", comms_dict));

    }
    p_mission.push_back(std::make_pair("mission_goal", pt_mission_goals));

    boost::property_tree::write_json(filename, p_mission);
    return true;
  } catch (std::exception const& e) {
    std::cerr << "Error while writing json file: ";
    std::cerr << e.what() << std::endl;
  }
  return false;
}

QDir MissionModel::get_home_dir() const
{
  size_t pos = home_dir.find("$ACTION_HOME");
  if(pos != std::string::npos) {
    std::string copy(home_dir);
    copy.replace(pos, std::string("$ACTION_HOME").size(), qgetenv("ACTION_HOME"));
    return QDir(QString::fromStdString(copy));
  }
  return QDir(QString::fromStdString(home_dir));
}
