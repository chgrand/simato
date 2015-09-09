#! /usr/bin/env python3

import argparse
from functools import reduce
import itertools
import json
import logging
import math
import os
from pprint import pprint
import shutil
import sys
import yaml

import pddl
import subprocess

try:
    import gladys
except ImportError:
    print("[error] install gladys [and setup PYTHONPATH]")
    sys.exit(1)



##################     Parameters     ##################

MorseFastmode = False

speedAAV = 2 #m/s
accAAV = 2 #m/s2
motionDelayAAV = 20 #s

initActionLength = 30 #s

################     End Parameters     ################

useAAVPatrol = True
useAGVPatrol = True

class IllFormatedInput(Exception):
    pass

class ErrorDistanceMap(Exception):
    pass

costExploreAction = 1

class ProblemGenerator:
    
    def __init__(self, mission, distanceFile):
    
        self.canLaunchHiPOP = True
        self.mission = mission
            
        ##Convert all data to numeric format
        for agent in self.mission["agents"].keys():
            if "spare" in self.mission["agents"][agent]:
                if self.mission["agents"][agent]["spare"] in ["false", "False"]:
                    self.mission["agents"][agent]["spare"] = False
                elif self.mission["agents"][agent]["spare"] in ["true", "True"]:
                    self.mission["agents"][agent]["spare"] = True
                else:
                    logging.error("Cannot convert %s to boolean" % self.mission["agents"][agent]["spare"])
                    sys.exit(1)
            else:
                self.mission["agents"][agent]["spare"] = False

            for k,v in self.mission["agents"][agent]["position"].items():
                self.mission["agents"][agent]["position"][k] = float(v)
                
        for wpg in self.mission["wp_groups"].keys():
            for wpn in self.mission["wp_groups"][wpg]["waypoints"].keys():
                for k,v in self.mission["wp_groups"][wpg]["waypoints"][wpn].items():
                    self.mission["wp_groups"][wpg]["waypoints"][wpn][k] = float(v)

        for obs in self.mission["mission_goal"]["observation_points"].keys():
            for k,v in self.mission["mission_goal"]["observation_points"][obs].items():
                self.mission["mission_goal"]["observation_points"][obs][k] = float(v)

        ##
    
        homeDir = str(self.mission["home_dir"])
        if not os.path.exists(homeDir):
            logging.error("Home folder defined in the mission file do not exists : %s" % homeDir)
            sys.exit(1)

        self.gladys = {}
        for name,model in self.mission["models"].items():
            
            regionFile = os.path.join(homeDir, str(self.mission["map_data"]["region_file"]))
            if "region_file" in model:
                regionFile = os.path.join(homeDir, str(model["region_file"]))
            if not os.path.exists(regionFile):
                logging.error("Cannot open %s" % regionFile)
                sys.exit(1)
            
            dtmFile = os.path.join(homeDir, str(self.mission["map_data"]["dtm_file"]))
            if not os.path.exists(dtmFile):
                logging.error("Cannot open %s" % dtmFile)
                sys.exit(1)
            
            configFile = os.path.join(homeDir, str(model["config_file"]))
            if not os.path.exists(configFile):
                logging.error("Cannot open %s" % configFile)
                sys.exit(1)

            self.gladys[name] = gladys.gladys(regionFile, dtmFile, configFile)
        
            with open(os.path.join(homeDir, model["config_file"])) as f:
                modelData = json.load(f)

            for k,v in modelData.items():
                model[k] = v

            logging.info("Velocity of %s : %f" % (name,modelData["robot"]["velocity"]))
    
        self.initIndex = {}
        for agent in self.getRobotList():
            initPos = self.mission["agents"][agent]["position"]
            initIndex = None
            for index,pt in self.mission["wp_groups"][self.mission["agents"][agent]["wp_group"]]["waypoints"].items():
                if(pt["x"] == initPos["x"] and pt["y"] == initPos["y"]):
                    initIndex = index
                    break
                    
            if initIndex is None:
                initIndex = "_init_" + agent
                self.mission["wp_groups"][self.mission["agents"][agent]["wp_group"]]["waypoints"][initIndex] = initPos
            
            self.initIndex[agent] = initIndex
        
        self.distanceMap = {}

        self.computeDistanceAGV(distanceFile)
        
        logging.info("Initialisation done")


    def computeDistanceAGV(self, distanceFile):

        recompute = False
        if os.access(distanceFile, os.R_OK):
            with open(distanceFile, "r") as f:
                self.distanceMap = json.load(f)

            #Check if it needs to be recomputed
            for model in self.mission["models"].keys():
                if not self.useGladysForModel(model): continue
                if model not in self.distanceMap :
                    recompute = True
                    break

                points = set()
                for r in self.mission["agents"].values():
                    if r["model"] == model:
                        for pt in self.mission["wp_groups"][r["wp_group"]]["waypoints"].values():
                            points.add((pt["x"], pt["y"]))
                points = list(points)

                for pt1,pt2 in itertools.combinations(points, 2):
                    k1 = "%s_%s_%s_%s" % (pt1[0], pt1[1], pt2[0], pt2[1])
                    k2 = "%s_%s_%s_%s" % (pt2[0], pt2[1], pt1[0], pt1[1])
                    if k1 not in self.distanceMap[model] and k2 not in self.distanceMap[model]:
                        logging.warning("Points have changed. Recomputing the distances")
                        recompute = True
                        break

                if recompute:
                    break

        else:
            logging.warning("Cannot find %s. Using gladys to compute the distances." % distanceFile)
            recompute = True

        if recompute:
            for model in self.mission["models"].keys():
                if not self.useGladysForModel(model):
                    continue
                
                logging.info("Pre-computing distances for %s" % model)

                self.distanceMap[model] = {}
                
                g = self.gladys[model]
                
                points = set()
                for r in self.mission["agents"].values():
                    if r["model"] == model:
                        for pt in self.mission["wp_groups"][r["wp_group"]]["waypoints"].values():
                            points.add((pt["x"], pt["y"]))
                points = list(points)

                for i in range(len(points)):
                    logging.debug("Using gladys for computing distance map of %s %d / %d" % (model,i,len(points)))
                    costs = g.single_source_all_costs(points[i], points[i+1:])
                    for j,cost in zip(range(i+1, len(points)), costs):
                        #print("%.2f,%.2f -> %.2f,%.2f : %.2f" % (points[i][0], points[i][1], points[j][0], points[j][1], cost))
                        self.distanceMap[model]["%s_%s_%s_%s" % (points[i][0], points[i][1], points[j][0], points[j][1])] = cost

                logging.info("Computing distances for %s done" % model)
        
            with open(distanceFile, "w") as f:
                json.dump(self.distanceMap, f)


    def getGladysDistance(self, model, start, end):
        if not self.useGladysForModel(model):
            logging.error("Error : call gladys distance for %s" % model)
            sys.exit(1)

        key = "%s_%s_%s_%s" % (start["x"], start["y"], end["x"], end["y"])
        cost = self.distanceMap[model].get(key, None)
        if cost is not None:
            return cost
        
        key = "%s_%s_%s_%s" % (end["x"], end["y"], start["x"], start["y"])
        cost = self.distanceMap[model].get(key, None)
        if cost is not None:
            return cost
        
        logging.error(sorted(self.distanceMap.keys()))
        raise ErrorDistanceMap("Cannot find key %s" % key)

    def getModelList(self):
        return list(self.mission["models"].keys())

    def useGladysForModel(self, model):
        return False
        return model in ["mana", "minnie", "effibot"]

    def getRobotList(self):
        return list(self.mission["agents"].keys())
    
    def getRobotModel(self, r):
        return self.mission["agents"][r]["model"]
        
    def getRobotName(self, robot):
        return robot
    
    # A wp is a tuple (wp_group, index)
    def getLocName(self, wp):
        pt = self.mission["wp_groups"][wp[0]]["waypoints"][wp[1]]
        if "z" in pt:
            return "%s_%d_%d_%d" % (wp[0], pt["x"]*100, pt["y"]*100, pt["z"]*100)
        else:
            return "%s_%d_%d" % (wp[0], pt["x"]*100, pt["y"]*100)

    def getLocsOfRobot(self, robot):
        groupName = self.mission["agents"][robot]["wp_group"]
        return [(groupName, i) for i in self.mission["wp_groups"][groupName]["waypoints"].keys()]

    def getTupleLoc(self, wp):
        pt = self.mission["wp_groups"][wp[0]]["waypoints"][wp[1]]
        
        if "ressac" in wp[0]:
            return (pt["x"], pt["y"], 40)
        else:
            return (pt["x"], pt["y"], 0)
        
        if "z" in pt:
            return (pt["x"], pt["y"], pt["z"])
        else:
            return (pt["x"], pt["y"])

    def computeDistance(self, robot, pt1, pt2):
        start = self.mission["wp_groups"][pt1[0]]["waypoints"][pt1[1]]
        end   = self.mission["wp_groups"][pt2[0]]["waypoints"][pt2[1]]
        
        if self.useGladysForModel(self.getRobotModel(robot)):
            return self.getGladysDistance(self.mission["agents"][robot]["model"], start, end)
        elif "ressac" in robot:
            dist = math.sqrt(math.pow(start["x"] - end["x"], 2) + math.pow(start["y"] - end["y"], 2))
            if dist >= speedAAV*speedAAV/accAAV:
                duration = dist/speedAAV + speedAAV/accAAV
            else:
                raise PathTooShort
                    
            return duration + motionDelayAAV
        else:
            dist = math.sqrt(math.pow(start["x"] - end["x"], 2) + math.pow(start["y"] - end["y"], 2))
            velocity = self.mission["models"][self.mission["agents"][robot]["model"]]["robot"]["velocity"]
            return dist / velocity

    
    # A wp is just the index of the point
    def getObsLocName(self, wp):
        pt = self.mission["mission_goal"]["observation_points"][wp]
        return "ptobs_%d_%d" % (pt["x"]*100, pt["y"]*100)

    def getTupleObs(self, wp):
        pt = self.mission["mission_goal"]["observation_points"][wp]
        return (pt["x"], pt["y"], 1)
        if "z" in pt:
            return (pt["x"], pt["y"], pt["z"])
        else:
            return (pt["x"], pt["y"])

    def getInitialPos(self, robot):
        return (self.mission["agents"][robot]["wp_group"], self.initIndex[robot])

    def getDomainString(self):
    
        d = pddl.Domain("action")
        d.addRequirement("strips")
        d.addRequirement("typing")
        d.addRequirement("durative-actions")
        d.addRequirement("equality")
        d.addRequirement("agents-def")
        
        d.addType("robot", "object")
        d.addType(" ".join(self.getModelList()), "robot")
        d.addType("loc", "object")
        d.addType("loc-wp loc-obs", "loc")
        
        d.addPredicate("explored ?z - loc-obs")
        d.addPredicate("at-r ?r - robot ?z - loc-wp")
        
        d.addPredicate("adjacent ?from ?to - loc-wp")
        d.addFunction( "distance ?from ?to - loc-wp")
        d.addPredicate("robot-allowed ?r - robot ?wp - loc-wp")
        
        d.addPredicate("visible ?r - robot ?from - loc-wp ?to - loc-obs")
        d.addPredicate("visible-com ?r1 ?r2 - robot ?wp1 ?wp2 - loc-wp")

        d.addPredicate("in-communication-at ?r1 ?r2 - robot ?l1 ?l2 - loc-wp")
        d.addPredicate("in-communication ?r1 ?r2 - robot")
        d.addPredicate("have-been-in-com ?r1 ?r2 - robot")

        ###
        a = pddl.Action("move", True)
        a.addParameter("?r", "robot")
        a.addParameter("?from ?to", "loc-wp")
        a.addAgent("?r")
        a.setDuration("(distance ?from ?to)")
        a.addPrecs(["robot-allowed ?r ?from", "over all"], ["robot-allowed ?r ?to", "over all"])
        a.addPrecs(["adjacent ?from ?to", "over all"], ["not (= ?from ?to)", "over all"])
        a.addPrecs(["at-r ?r ?from", "at start"])
        a.addEffs(["at-r ?r ?to", "at end"], ["not (at-r ?r ?from)", "at start"])
        
        d.addAction(a)
        ###
        
        ###
        a = pddl.Action("observe", True)
        a.addParameter("?r", "agv")
        a.addParameter("?from", "loc-wp")
        a.addParameter("?to", "loc-obs")
        a.addAgent("?r")
        a.setDuration(costExploreAction)
        a.addPrecs(["robot-allowed ?r ?from", "over all"], ["visible ?r ?from ?to", "over all"])
        a.addPrecs(["at-r ?r ?from", "over all"])
        a.addEff("explored ?to", "at end")
        
        d.addAction(a)
        ###
        
        ### r1 try to communicate with r2
        a = pddl.Action("communicate", True)
        a.addParameter("?r1 ?r2", "robot")
        a.addParameter("?l1 ?l2", "loc-wp")
        a.addAgent("?r1")
        a.setDuration(1)
        a.addPrecs(["robot-allowed ?r1 ?l1", "over all"], ["robot-allowed ?r2 ?l2", "over all"], ["not (= ?r1 ?r2)", "over all"],  ["visible-com ?r1 ?r2 ?l1 ?l2", "over all"])
        a.addPrecs(["at-r ?r1 ?l1", "over all"]) # no need to add ["at-r ?r2 ?l2", "over all"] since this is not the responsability of r1
        a.addEffs(["in-communication-at ?r1 ?r2 ?l1 ?l2", "at start"], ["not (in-communication-at ?r1 ?r2 ?l1 ?l2)", "at end"])
        #a.addEffs(["in-communication ?axv2 ?axv1", "at start"], ["not (in-communication ?axv2 ?axv1)", "at end"])
        
        d.addAction(a)
        ###

        ###
        a = pddl.Action("communicate-meta", True)
        a.addParameter("?r1 ?r2", "robot")
        a.addParameter("?l1 ?l2", "loc-wp")
        a.addAgent("?r1") #need to assign it to someone
        a.setDuration(0.5)
        a.addPrecs(["robot-allowed ?r1 ?l1", "over all"], ["robot-allowed ?r2 ?l2", "over all"], ["not (= ?r1 ?r2)", "over all"],  ["visible-com ?r1 ?r2 ?l1 ?l2", "over all"])
        a.addPrecs(["in-communication-at ?r1 ?r2 ?l1 ?l2", "over all"], ["in-communication-at ?r2 ?r1 ?l2 ?l1", "over all"])
        a.addEffs(["in-communication ?r1 ?r2", "at start"], ["in-communication ?r2 ?r1", "at start"])
        a.addEffs(["not (in-communication ?r1 ?r2)", "at end"], ["not (in-communication ?r2 ?r1)", "at end"])
        
        d.addAction(a)
        ###

        ###
        a = pddl.Action("has-communicated", True)
        a.addParameter("?r1 ?r2", "robot")
        a.addAgent("?r1") #need to assign it to someone
        a.setDuration(1)
        a.addPrecs(["not (= ?r1 ?r2)", "over all"])
        a.addPrecs(["in-communication ?r1 ?r2", "at start"], ["in-communication ?r2 ?r1", "at start"])
        a.addEffs(["have-been-in-com ?r1 ?r2", "at end"], ["have-been-in-com ?r2 ?r1", "at end"])
        
        d.addAction(a)
        ###
        
        ###
        #a = pddl.Action("has-communicated-goal", True)
        #a.addParameter("?r1 ?r2", "robot")
        #a.setDuration(1)
        #a.addPrecs(["have-been-in-com ?r1 ?r2", "at start"], ["not (= ?r1 ?r2)", "over all"])
        #a.addEffs(["not (have-been-in-com ?r1 ?r2)", "at end"])
        
        #d.addAction(a)
        ###
        
        ###
        a = pddl.Action("init", True)
        a.addParameter("?r", "robot")
        a.addParameter("?l", "loc-wp")
        a.addAgent("?r")
        a.setDuration(initActionLength)
        a.addPrecs(["at-r ?r ?l", "over all"])
        a.addEffs(["at-r ?r ?l", "at end"])

        d.addAction(a)
        ###
        
        return d.toString()

    def getProblemString(self):
    
        p = pddl.Problem("action-prb", "action")

        ####  Objects ####
        for r in self.getRobotList():
            p.addObject(self.getRobotName(r), self.mission["agents"][r]["model"])
        
        for index,group in self.mission["wp_groups"].items():
            [p.addObject(self.getLocName((index, i)), "loc-wp") for i in group["waypoints"].keys()]

        [p.addObject(self.getObsLocName(l), "loc-obs") for l in self.mission["mission_goal"]["observation_points"].keys()]

        ### Init position ###

        for robot in self.getRobotList():
            p.addInits("at-r {robot} {pt}".format(robot=robot, pt=self.getLocName(self.getInitialPos(robot))))

        ### Points allowed ###

        for robot in self.getRobotList():
            groupName = self.mission["agents"][robot]["wp_group"]
            for index in self.mission["wp_groups"][groupName]["waypoints"].keys():
                p.addInits("robot-allowed {robot} {pt}".format(robot=robot, pt=self.getLocName((groupName, index))))

        ####  Distance for motion ####

        for robot in self.getRobotList():
            for pt1,pt2 in itertools.combinations(sorted(self.getLocsOfRobot(robot)), r=2):
                cost = self.computeDistance(robot, pt1, pt2)
                if cost != 0 and cost != float("inf"):
                    p.addInits("= (distance {start} {end}) {cost}) (adjacent {start} {end}".format(start=self.getLocName(pt1),end=self.getLocName(pt2),cost=cost) )
                    p.addInits("= (distance {start} {end}) {cost}) (adjacent {start} {end}".format(start=self.getLocName(pt2),end=self.getLocName(pt1),cost=cost) )
    
        ####  visibility for observation ####

        count = 0
        for ptObs in self.mission["mission_goal"]["observation_points"].keys():
            isVisible = False
            isVisiblePatrol = False
            visibleFrom = []
            
            for robot in self.getRobotList():
                model = self.getRobotModel(robot)
                
                for ptMove in self.getLocsOfRobot(robot):
                    if self.gladys[model].is_visible(self.getTupleLoc(ptMove), self.getTupleObs(ptObs)):
                        p.addInits("visible %s %s %s" %(robot, self.getLocName(ptMove), self.getObsLocName(ptObs)))
                        count += 1
                        isVisible = True
                        visibleFrom.append(self.getLocName(ptMove))
        
                        for patrolName, patrol in self.mission["wp_groups"][ptMove[0]]["patrols"].items():
                            if ptMove[1] in patrol:
                                isVisiblePatrol = True

            if not isVisible:
                logging.error("Cannot see observation point %s from any point" % self.getObsLocName(ptObs))
                self.canLaunchHiPOP = False
            if not isVisiblePatrol:
                logging.error("Cannot see observation point %s with a patrol" % self.getObsLocName(ptObs))
                logging.error("Visible from : %s" % visibleFrom)
                self.canLaunchHiPOP = False
    
        logging.info("Found %d visibility links" % count)

        ####  visibility for communication ####

        count = 0
        for robot1, robot2 in itertools.product(self.getRobotList(), self.getRobotList()):
            points1 = self.getLocsOfRobot(robot1)
            points2 = self.getLocsOfRobot(robot2)
            canCommunicate = False
            
            for pt1, pt2 in itertools.product(points1, points2):
                if self.canRobotsCommunicate(robot1, robot2, pt1, pt2):
                    p.addInits("visible-com {robot1} {robot2} {pt1} {pt2}".format(robot1=robot1, robot2=robot2, pt1=self.getLocName(pt1), pt2=self.getLocName(pt2)))
                    count += 1
                    canCommunicate = True
    
            if not canCommunicate:
                logging.warning("Robots %s and %s cannot communicate" % (self.getRobotName(robot1), self.getRobotName(robot2)))
                #self.canLaunchHiPOP = False
        logging.info("Found %d com links" % count)


        ####  Goals ####
        
        p.addGoals(*["explored %s" % self.getObsLocName(wp) for wp in self.mission["mission_goal"]["observation_points"].keys()])

        return p.toString()

    def canRobotsCommunicate(self, robot1, robot2, pt1, pt2):
        g1 = self.gladys[self.mission["agents"][robot1]["model"]]
        g2 = self.gladys[self.mission["agents"][robot2]["model"]]
        antennaHeight1 = self.mission["models"][self.mission["agents"][robot1]["model"]]["antenna"]["pose"]["z"]
        antennaHeight2 = self.mission["models"][self.mission["agents"][robot2]["model"]]["antenna"]["pose"]["z"]

        p = self.getTupleLoc(pt1)
        posAntenna1 = (p[0], p[1], p[2] + antennaHeight1)

        p = self.getTupleLoc(pt2)
        posAntenna2 = (p[0], p[1], p[2] + antennaHeight2)

        return g1.can_communicate(self.getTupleLoc(pt1), posAntenna2) and g2.can_communicate(self.getTupleLoc(pt2), posAntenna1)

    def getHelperString(self):

        h = pddl.Helper("action")
        h.addOption("abstractOnly")
        h.addOption("erasePlansWhenAbstractMet")

        h.addAllowedAction("move")
        h.addAllowedAction("communicate")
        h.addAllowedAction("communicate-meta")
        h.addAllowedAction("has-communicated")
        if not useAAVPatrol:
            logging.error("Impossible d'autoriser les observe des AAV mais pas des AGV")
            h.addAllowedAction("observe")
        if not useAGVPatrol:
            logging.error("Impossible d'autoriser les observe des AGV mais pas des AAV")
            h.addAllowedAction("observe")

        for robot in self.getRobotList():
            l = self.getPatrolActions(robot)
        
            for p in l:
                h.addAction(p)

        return h.toString()


    def getPatrolActions(self, robot):
        result = []
        wpGroupName = self.mission["agents"][robot]["wp_group"]
        g = self.gladys[self.mission["agents"][robot]["model"]]
        
        for patrolName,direct in itertools.product(self.mission["wp_groups"][wpGroupName]["patrols"].keys(), [True, False]):
            patrol = [(wpGroupName, i) for i in self.mission["wp_groups"][wpGroupName]["patrols"][patrolName]]
            if not direct:
                patrol.reverse()
            
            if not direct and len(patrol) < 2:
                continue #direct and indirect are the same if there is only one point
            
            wpList = self.mission["wp_groups"][self.mission["agents"][robot]["wp_group"]]["waypoints"]
            start = patrol[0]
            end = patrol[-1]
            
            obs = {} #key:point name. Value : list of obs point
            for pt in patrol:
                listObs = [obsPt for obsPt in self.mission["mission_goal"]["observation_points"].keys() if g.is_visible(self.getTupleLoc(pt), self.getTupleObs(obsPt))]

                if len(listObs) > 0:
                    obs[self.getLocName(pt)] = listObs
        
            #clean the list : keep only one point for each observation
            seenPt = set()
            for k,v in obs.items():
                for pt in list(v): #copy it
                    if pt in seenPt:
                        v.remove(pt)
                    else:
                        seenPt.add(pt)
            
            for k in list(obs.keys()):
                if len(obs[k]) == 0:
                    del obs[k]
        
            a = pddl.Action("patrol_%s_%s__%s__%s" % (robot, patrolName, self.getLocName(start), self.getLocName(end)))

            a.addAgent(robot)
            a.addConflict("at-r %s *" % robot)

            a.addPrec("at-r %s %s" % (robot, self.getLocName(start)))

            if len(obs) == 0:
                logging.warning("Patrol %s of group %s cannot see any observation point. Ignoring it" % (patrolName,wpGroupName))
                
            for obsPoint in set(reduce(lambda x,y : x + y, obs.values(), [])):
                a.addEffs("explored " + self.getObsLocName(obsPoint))
            a.addEff("at-r %s %s" % (robot, self.getLocName(end))) #Pas de side effect : on veut pouvoir l'introduire pour le mouvement et resoudre des buts de maniere opportuniste en presence de deadline

            if direct:
                m = pddl.Method(a.name + "_d_" + patrolName)
            else:
                m = pddl.Method(a.name + "_i_" + patrolName)

            #move-i goes from patrol[i] to patrol[i+1]
            for i,f,t in zip(range(len(patrol)), patrol, patrol[1:]):

                cost = self.computeDistance(robot, f, t)
                if cost == 0 or cost == float("inf"):
                    logging.error("In patrol %s, robot %s cannot go from %s to %s" % (patrolName, robot, self.getLocName(f), self.getLocName(t)))
                    self.canLaunchHiPOP = False

                m.addAction("move-%d" % i, "move %s %s %s" % (robot, self.getLocName(f), self.getLocName(t)))
                    
                if i == 0:
                    m.addCausalLink(":init", "move-0", "at-r %s %s" % (robot, self.getLocName(start)))
                else:
                    m.addCausalLink("move-%d" % (i-1), "move-%d" % i, "at-r %s %s" % (robot, self.getLocName(f)))

            if len(patrol) > 1:
                m.addCausalLink("move-%d" % (len(patrol) - 2), ":goal", "at-r %s %s" % (robot, self.getLocName(end)))
            else:
                #single point, start = end
                m.addCausalLink(":init", ":goal", "at-r %s %s" % (robot, self.getLocName(start)))
                    
            #Add the observe actions
            exploreId = -1
            for i,mvPt in zip(range(len(patrol)), patrol):
                for obsPt in obs.get(self.getLocName(mvPt), []):
                    exploreId += 1
                    m.addAction("explore-%d" % exploreId, "observe %s %s %s" % (robot, self.getLocName(mvPt), self.getObsLocName(obsPt)))
            
                    m.addCausalLink("explore-%d" % exploreId, ":goal", "explored %s" % self.getObsLocName(obsPt))
            
                    if mvPt == patrol[0]:
                        s = ":init"
                    else:
                        s = "move-%d" % (patrol.index(mvPt) - 1)
                    m.addCausalLink(s, "explore-%d" % exploreId, "at-r %s %s" % (robot, self.getLocName(mvPt)))
                
                    if mvPt != patrol[-1]:
                        m.addTemporalLink("explore-%d" % exploreId, "move-%d" % (patrol.index(mvPt)))
            
            a.addMethod(m)

            result.append(a)
        
        return result

    #return a json-formated string
    def getPlanString(self):
        
        result = {}
        
        result["actions"] = {"0" : {"name":"dummy init", "dMax": 0.0, "dMin": 0.0, "endTp": 0,"startTp": 0},
                             "1" : {"name":"dummy end", "dMax": 0.0, "dMin": 0.0, "endTp": 1,"startTp": 1},
                            }
        result["causal-links"] = []
        result["temporal-links"] = []
        result["absolute-time"] = []

        nextTimepoint = 2
        
        #add init action
        for robot in self.getRobotList():
            if self.mission["agents"][robot]["spare"] or robot in []:
                logging.info("Adding an init action for %s" % robot)
                
                actionIndex = str(len(result["actions"]))
                result["actions"][actionIndex] = {
                                  "name":"init %s %s" % (robot, self.getLocName(self.getInitialPos(robot))),
                                  "startTp": nextTimepoint,
                                  "endTp": nextTimepoint + 1,
                                  "dMin": float(initActionLength),
                                  "dMax": float(initActionLength),
                                  "locked": True,
                                  "agent":robot
                                  }
                result["absolute-time"].append([nextTimepoint, 0.1])
                nextTimepoint += 2
        
        if "communication_goals" in self.mission["mission_goal"]:
            if type(self.mission["mission_goal"]["communication_goals"]) == list:
                l = self.mission["mission_goal"]["communication_goals"]
            else:
                l = list(self.mission["mission_goal"]["communication_goals"].values())
            
            for c in l:
                if not "agent1" in c or not "agent2" in c or not "date" in c:
                    raise IllFormatedInput
                
                if (not "wp_1" in c and "wp_2" in c) or (not "wp_2" in c and "wp_1" in c):
                    raise IllFormatedInput
                
                if c["agent1"] > c["agent2"]:
                    c["agent1"], c["agent2"] = c["agent2"], c["agent1"]
                    if "wp_1" in c and "wp_2" in c:
                        c["wp_1"], c["wp_2"] = c["wp_2"], c["wp_1"]
                
                actionIndex = len(result["actions"])

                if not "wp_1" in c:
                    logging.error("Cannot set a communication constraint without specifying the points yet")
                    self.canLaunchHiPOP = False
                    
                    result["actions"][str(actionIndex)] = {
                                                      "name":"has-communicated %s %s" % (c["agent1"], c["agent2"]),
                                                      "startTp": nextTimepoint,
                                                      "endTp": nextTimepoint + 1,
                                                      "dMin": 1.0,
                                                      "dMax": 1.0,
                                                      "locked": True
                                                      }
                    result["absolute-time"].append([nextTimepoint, float(c["date"])])
                    nextTimepoint += 2

                else:
                    pt1 = (self.mission["agents"][c["agent1"]]["wp_group"], c["wp_1"])
                    pt2 = (self.mission["agents"][c["agent2"]]["wp_group"], c["wp_2"])
                    
                    if not self.canRobotsCommunicate(c["agent1"], c["agent2"], pt1, pt2):
                        logging.error("Com impossible between %s and %s at %s and %s" %(c["agent1"], c["agent2"], self.getLocName(pt1), self.getLocName(pt2)))
                        logging.error(self.canRobotsCommunicate(c["agent2"], c["agent1"], pt2, pt1))
                        self.canLaunchHiPOP = False
                    
                    actionName1 = "communicate %s %s %s %s" % (c["agent1"], c["agent2"], self.getLocName(pt1), self.getLocName(pt2))
                    actionName2 = "communicate %s %s %s %s" % (c["agent2"], c["agent1"], self.getLocName(pt2), self.getLocName(pt1))
                    actionName3 = "communicate-meta %s %s %s %s" % (c["agent1"], c["agent2"], self.getLocName(pt1), self.getLocName(pt2))
                    lit1 = "in-communication-at %s %s %s %s" % (c["agent1"], c["agent2"], self.getLocName(pt1), self.getLocName(pt2))
                    lit2 = "in-communication-at %s %s %s %s" % (c["agent2"], c["agent1"], self.getLocName(pt2), self.getLocName(pt1))

                    result["actions"][str(actionIndex)] = {
                                                "name":actionName1,
                                                "startTp": nextTimepoint,
                                                "endTp": nextTimepoint + 1,
                                                "dMin": 1.0,
                                                "dMax": 1.0,
                                                "locked": True
                                                }
                    
                    result["actions"][str(actionIndex+1)] = {
                                                "name":actionName2,
                                                "startTp": nextTimepoint + 2,
                                                "endTp": nextTimepoint + 3,
                                                "dMin": 1.0,
                                                "dMax": 1.0,
                                                "locked": True
                                                }

                    result["actions"][str(actionIndex+2)] = {
                                                "name":actionName3,
                                                "startTp": nextTimepoint + 4,
                                                "endTp": nextTimepoint + 5,
                                                "dMin": 0.5,
                                                "dMax": 0.5,
                                                "locked": True
                                                }

                    #result["absolute-time"].append([nextTimepoint    , float(c["date"])])
                    #result["absolute-time"].append([nextTimepoint + 2, float(c["date"])])
                    result["absolute-time"].append([nextTimepoint + 4, float(c["date"])])
                    #result["absolute-time"].append([nextTimepoint + 5, float(c["date"]) + 0.5])

                    result["causal-links"].append({ "startAction" : str(actionIndex),
                                                    "endAction" : str(actionIndex+2),
                                                    "startTp": nextTimepoint, "endTp" : nextTimepoint + 4,
                                                    "startTs":1,"endTs":3,
                                                    "lit":lit1})

                    result["causal-links"].append({ "startAction" : str(actionIndex+1),
                                                    "endAction" : str(actionIndex+2),
                                                    "startTp": nextTimepoint+2, "endTp" : nextTimepoint + 4,
                                                    "startTs":1,"endTs":3,
                                                    "lit":lit2})

                    nextTimepoint += 6
        
        return json.dumps(result)
    
    def writeHiPOPLaunchFile(self, f, data):
        nonSpareAgents = [agent for agent in self.mission["agents"] if not self.mission["agents"][agent].get("spare", False)]
        spareAgents = [agent for agent in self.mission["agents"] if self.mission["agents"][agent].get("spare", False)]

        logging.info("There is %d robots with %s additional spare robots" % (len(nonSpareAgents), len(spareAgents)))
        
        f.write("""#! /usr/bin/env bash
    DIR=$( cd "$( dirname "${{BASH_SOURCE[0]}}" )" && pwd )
    cd $DIR
    
    hipop --logLevel error -H {helper} -I {planInit} --agents {agentList} -P hadd_time_lifo -A areuse_motion_nocostmotion -F local_openEarliestMostCostFirst_motionLast -O {output}.pddl -o {output}.plan {domain} {prb}
    """.format(domain=data["domainFile"], 
               prb=data["prbFile"], 
               helper=data["helperFile"], 
               planInit=data["planInitFile"], 
               output=data["outputName"],
               agentList = "_".join(nonSpareAgents)))

        os.chmod(f.name, 0o755)

    def writeMorseFile(self, f):
        robots = []
        for r in self.getRobotList():
            if "ressac" in r:
                robots.append("%s = Ressac(%d, %d) #,teleport=True)" % (r, self.mission["agents"][r]["position"]["x"], self.mission["agents"][r]["position"]["y"]))
            else:
                robots.append("%s = AGV(%d, %d) #,teleport=True)" % (r, self.mission["agents"][r]["position"]["x"], self.mission["agents"][r]["position"]["y"]))
            
        speedAGV = 1
        if "mana" in self.mission["models"]:
            speedAGV = self.mission["models"]["mana"]["robot"]["velocity"]
    
        s = os.path.expandvars("$ACTION_HOME")
        morseFilepath = os.path.join(self.mission["home_dir"].replace(s, "$ACTION_HOME"), self.mission["map_data"]["blender_file"])

        f.write("""from morse.builder import *
import os

class Ressac(RMax):
    def __init__(self, x=0, y=0, z=30, teleport=False):
        RMax.__init__(self)
        if not teleport:
            gps = GPS()
            self.append(gps)
            pose = Pose()
            self.append(pose)
            pose.add_interface('ros')
            waypoint = Waypoint()
            waypoint.properties(FreeZ=True, Speed={speedAAV}, ControlType="Position")
            self.append(waypoint)
            gps.add_interface('socket')
            waypoint.add_interface('socket')
        else:
            teleport = Teleport()
            teleport.add_interface('ros')
            self.append(teleport)
        self.translate(x, y, z)

class AGV(ATRV):
    def __init__(self, x=0, y=0, teleport=False):
        ATRV.__init__(self)
        self.properties(GroundRobot=True)
        if not teleport:
            gps = GPS()
            gps.add_interface('socket')
            self.append(gps)
            pose = Pose()
            pose.add_interface('ros')
            self.append(pose)
            waypoint = Waypoint()
            waypoint.properties(Speed={speedAGV})
            waypoint.add_interface('socket')
            self.append(waypoint)
        else:
            teleport = Teleport()
            teleport.add_interface('ros')
            self.append(teleport)
        self.translate(x, y, 10)

{robots}

# Scene
env = Environment(os.path.expandvars("{morseFilepath}"), fastmode={fastmode})
env.set_camera_clip(clip_end=1000)
env.aim_camera([0.4, 0, -0.7854])
env.place_camera([138, -150, 100])
env.set_camera_speed(100)
env.create()
""".format(robots="\n".join(robots), speedAGV=speedAGV, speedAAV= speedAAV, fastmode=MorseFastmode, morseFilepath=morseFilepath))

    #write in the current working directory
    def writeRoslaunchFiles(self, pathToMission, data, missionName):
        pathToMission = pathToMission.replace("$ACTION_HOME", "$(env ACTION_HOME)")

        with open("hidden-params.launch", "w") as f:
            f.write("""
<launch>
    <param name="hidden/plan"        type="str" textfile="{plan}" />
    <param name="hidden/pddl/domain" type="str" textfile="{domain}" />
    <param name="hidden/pddl/prb"    type="str" textfile="{prb}" />
    <param name="hidden/pddl/helper" type="str" textfile="{helper}" />

    <rosparam command="load" param="vnet/config" file="{vnet}" />
</launch>
""".format(plan  =os.path.join(pathToMission, "hipop-files/" + data["planFile"]),
           domain=os.path.join(pathToMission, "hipop-files/" + data["domainFile"]),
           prb   =os.path.join(pathToMission, "hipop-files/" + data["prbFile"]),
           helper=os.path.join(pathToMission, "hipop-files/" + data["helperFile"]),
           vnet  =os.path.join(pathToMission, "vnet_config.yaml")))
        
        
        with open("mission.launch", "w") as f:
            f.write("<launch>\n")

            f.write("""
        <arg name="visu" default="true"/>
        <arg name="simu" default="true"/>
        <arg name="executor" default="morse+ros"/>
        <arg name="bag_repair" default="false"/>
        <arg name="bag_all" default="true" />
        <arg name="vnet" default="true"/>
        <arg name="morse" default="false"/>
        <arg name="auto_start" default="false"/>
        <arg name="ismac" default="false" />

        <node name="$(anon bag_all)" pkg="rosbag" type="record" args="-a -o $(env ACTION_HOME)/logs/{mission_name}" if="$(arg bag_all)" />

        <include file="hidden-params.launch" />

        <include file="$(find ismac)/launch/start.launch" if="$(arg ismac)">
            <arg name="mission_file" value="{missionFile}" />
        </include>

        <node name="$(anon visu)" pkg="metal" type="onlineTimeline.py" ns="visu" args="--missionFile {missionFile}" if="$(arg visu)" />
        
        <node name="$(anon bag)" pkg="rosbag" type="record" args="/hidden/repair -o hidden_repair" if="$(arg bag_repair)" />
        
        <node name="$(anon morse)" pkg="metal" type="morse_run" args="{morsePath}" if="$(arg morse)" />
        <node name="$(anon pose2teleport)" pkg="ismac" type="pose2teleport" args="{roboList}" if="$(arg morse)" />
              
        <node name="$(anon autoStart)" pkg="metal" type="autoStart.py" ns="autoStart" if="$(arg auto_start)" />

""".format(morsePath = os.path.join(pathToMission, "run_morse.py"),
           missionFile = pathToMission+".json" ,
           mission_name = missionName,
           roboList = " ".join(self.getRobotList())))

            
            f.write("""\n\n    <group if="$(arg simu)">\n""")
            
            for r in self.getRobotList():
                f.write("""
        <include file="$(find metal)/launch/{robot}.launch">
            <arg name="executor" value="$(arg executor)" />
            <arg name="vnet" value="$(arg vnet)" />
        </include>
""".format(robot=r))
            f.write("""    </group>\n""")

            f.write("""        <node name="vnet" pkg="vnet" type="vnet_ros.py" if="$(arg vnet)"/>\n""")
            f.write("</launch>\n")

        with open("stats_simu.launch", "w") as f:
            f.write("<launch>\n")

            f.write("""
    <arg name="visu" default="false"/>
    <arg name="executor" default="ros"/>
    <arg name="bag_repair" default="false"/>
    <arg name="vnet" default="false"/>
    <arg name="auto_start" default="false"/>
    <arg name="alea_file"/>

    <param name="/hidden/aleas" type="str" textfile="$(arg alea_file)" />

    <include file="hidden-params.launch" />

    <node name="$(anon visu)" pkg="metal" type="onlineTimeline.py" ns="visu" args="--missionFile {missionFile}" if="$(arg visu)" />

    <node name="$(anon autoStart)" pkg="metal" type="autoStart.py" ns="autoStart" if="$(arg auto_start)" />

    <node name="$(anon bag)" pkg="rosbag" type="record" args="/hidden/repair -o hidden_repair" if="$(arg bag_repair)" />
    <node name="$(anon bag)" pkg="rosbag" type="record" args="/hidden/stats /hidden/stnvisu /hidden/start /hidden/repair /hidden/mastnUpdate /hidden/communicate -O $(optenv ROS_LOG_DIR ~/.ros)/stats.bag" />
    
    <node name="$(anon watcher)" pkg="metal" type="watcher.py" required="true" />
    
    <node name="$(anon aleas_injector)" pkg="metal" type="aleaInjector.py" />
""".format(missionFile = pathToMission+".json"))
            for r in self.getRobotList():
                f.write("""
    <include file="$(find metal)/launch/{robot}.launch">
        <arg name="executor" value="$(arg executor)" />
        <arg name="vnet" value="$(arg vnet)" />
    </include>
""".format(robot=r))
   
            f.write("""        <node name="vnet" pkg="vnet" type="vnet_ros.py" if="$(arg vnet)"/>\n""")
            f.write("</launch>\n")

    def writeVNetFile(self, f):
        vNetConfig = {}
        for topic in ["communicate", "repair", "mastnUpdate"]:
            vNetConfig[topic] = {}
            for r in self.getRobotList():
                vNetConfig[topic][r] = {"in":"/%s/hidden/%s/in" % (r, topic), "out":"/%s/hidden/%s/out" % (r, topic)}

        yaml.dump(vNetConfig, f)

def setup():
    parser = argparse.ArgumentParser(description='Create a set of plans for ACTION')
    parser.add_argument('missionFile', type=str)
    parser.add_argument('--outputFolder', type=str, default=None)
    parser.add_argument('-g', '--forceGladys' , action='store_true')
    parser.add_argument('--noAAVPatrols', action='store_true')
    parser.add_argument('--noAGVPatrols', action='store_true')
    parser.add_argument('--force', action='store_true')
    parser.add_argument('--logLevel',   type=str, default="info")
    args = parser.parse_args()

    global useAAVPatrol
    useAAVPatrol = not args.noAAVPatrols
    global useAGVPatrol
    useAGVPatrol = not args.noAGVPatrols

    #Configure the logger
    numeric_level = getattr(logging, args.logLevel.upper(), None)
    if not isinstance(numeric_level, int):
        raise ValueError('Invalid log level: %s' % args.logLevel)
    logging.basicConfig(level=numeric_level, format='%(levelname)s(%(filename)s:%(lineno)d):%(message)s')

    args.missionFile = os.path.abspath(args.missionFile)
    pathToMission = os.path.splitext(args.missionFile)[0]
    s = os.path.expandvars("$ACTION_HOME")
    pathToMission = pathToMission.replace(s, "$ACTION_HOME")

    args.missionFile = os.path.expandvars(args.missionFile)
    logging.info("Using mission file : %s" % args.missionFile)

    with open(args.missionFile) as f:
        mission = json.load(f)
        
    mission["home_dir"] = os.path.expandvars(mission["home_dir"])

    if args.outputFolder is None:
        outputFolder = os.path.abspath(os.path.splitext(args.missionFile)[0])
    else:
        outputFolder = args.outputFolder

    logging.info("Output folder : %s" % outputFolder) 

    distanceFileName = "distancemap.json"

    if os.access(outputFolder, os.R_OK):
        if not args.force:
            print("Output folder already exists. Do you want to erase it ? Press enter to continue")
            if sys.version_info.major <= 2:
                raw_input()
            else:
                input()

        for f in os.listdir(outputFolder):
            if f == distanceFileName and not args.forceGladys: continue #if the distances are pre-computed, do not remove them
            s = os.path.join(outputFolder, f)
            if os.path.isdir(s):
                shutil.rmtree(s)
            else:
                os.remove(s)
    else:
        os.mkdir(outputFolder)

    distanceFile = os.path.join(outputFolder, distanceFileName)

    if outputFolder.endswith("/"):
        missionName = os.path.basename(os.path.split(outputFolder)[0])
    else:
        missionName = os.path.basename(outputFolder)

    p = ProblemGenerator(mission, distanceFile)
    
    os.chdir(outputFolder)
    
    return p,missionName,pathToMission,args.missionFile

def main():
    
    p,missionName,pathToMission,missionFile = setup()

    ### Writing HiPOP files ###
    hipopFolder = "hipop-files"
    
    if not os.path.exists(hipopFolder):
        os.mkdir(hipopFolder)
    
    logging.info("Writting HiPOP files to %s" % os.path.join(os.getcwd(), hipopFolder))

    data = {}

    data["domainFile"] = missionName + "-domain.pddl"
    with open(os.path.join(hipopFolder, data["domainFile"]), "w") as f:
        f.write(p.getDomainString())

    data["prbFile"] = missionName + "-prb.pddl"
    with open(os.path.join(hipopFolder, data["prbFile"]), "w") as f:
        f.write(p.getProblemString())
    
    data["helperFile"] = missionName + "-prb-helper.pddl"
    with open(os.path.join(hipopFolder, data["helperFile"]), "w") as f:
        f.write(p.getHelperString())
    
    data["planInitFile"] = missionName + "-prb-init.plan"
    with open(os.path.join(hipopFolder, data["planInitFile"]), "w") as f:
        f.write(p.getPlanString())

    data["outputName"] = missionName
    data["planFile"] = missionName + ".plan"

    with open(os.path.join(hipopFolder, "launch-example.sh"), "w") as f:
        p.writeHiPOPLaunchFile(f, data)

    if p.canLaunchHiPOP:
        logging.info("Launching HiPOP")
        r = subprocess.call(os.path.join(hipopFolder, "launch-example.sh"))
        if r != 0:
            logging.error("HiPOP returned %s" % r)
        else:
            logging.info("Process finished correctly")
            
            try:
                import actionvisu
                
                actionvisu.drawPlanGeo(os.path.join(hipopFolder, data["outputName"] + ".pddl"), "plan-geo.png", missionFile=missionFile)
                actionvisu.drawPlanTimeline(os.path.join(hipopFolder, data["outputName"] + ".pddl"), "plan-timeline.png")
                actionvisu.drawPlanTimeline(os.path.join(hipopFolder, data["outputName"] + ".pddl"), "plan-timeline-move.png", onlyMove = True)
                
            except ImportError:
                logging.error("Cannot draw plans. Import actionvisu failed")

    with open("run_morse.py", "w") as f:
        p.writeMorseFile(f)

    with open("vnet_config.yaml", "w") as f:
        p.writeVNetFile(f)

    p.writeRoslaunchFiles(pathToMission, data, missionName)

    logging.info("Done")
    
    return 0

if __name__ == '__main__':
    sys.exit( main() )
