#!/usr/bin/env python3

import matplotlib
matplotlib.use('Qt5Agg')
import matplotlib.pyplot as plt


import itertools
import json
import os
import re
import sys

import argparse

def getCoord(ptName):
    l = ptName.split("_")
    return (float(l[1])/100, float(l[2])/100)

def getActionsFromPlan(filename):
    result = []
    
    with open(filename, "r") as f:
        for line in f:
            if line.startswith(";"):
                continue

            #match an action in PDDL and extract its name            
            m = re.match("^(\d*(?:.\d*)?)\s*:\s*\((.*)\)\s*\[(\d*(?:.\d*)?)\]", line)
            if m:
                tStart, actionName, dur = m.groups()
                d = {"tStart":tStart, "dur":dur, "name":actionName}
                if actionName.startswith("move"):
                    _,robot,start,end = actionName.split(" ")
                    start = getCoord(start)
                    end = getCoord(end)
                    
                    d["type"] = "move"
                    d["robot"] = robot
                    d["start"] = start
                    d["end"] = end
                elif actionName.startswith("communicate"):
                    actionType,robot1,robot2,start1,start2 = actionName.split(" ")
                    start1 = getCoord(start1)
                    start2 = getCoord(start2)
                    
                    d["type"] = actionType
                    d["robot1"] = robot1
                    d["robot2"] = robot2
                    d["start1"] = start1
                    d["start2"] = start2
                    d["end"] = end

                elif actionName.startswith("observe"):
                    _,robot,start,end = actionName.split(" ")
                    start = getCoord(start)
                    end = getCoord(end)
                    d["type"] = "observe"
                    d["robot"] = robot
                    d["start"] = start
                    d["end"] = end
                    
                elif actionName.startswith("init"):
                    _,robot,start = actionName.split(" ")
                    start = getCoord(start)
                    d["type"] = "init"
                    d["robot"] = robot
                    d["start"] = start
                   
                else:
                    print("Unknown action : %s" % actionName)
                    d["type"] = "unknown"
                    
                result.append(d)
                
    return result
    
def drawPlanGeo(filename, outputFile = None, missionFile=None):
    plt.clf()
    fig = plt.figure(1)
    plt.axis('equal')
    
    paths = {}
    
    if missionFile is not None:
        with open(missionFile) as f:
            mission = json.load(f)
    else:
        mission = None

    for d in getActionsFromPlan(filename):
        if d["type"] == "move":
            robot = d["robot"]
            start = d["start"]
            end = d["end"]
            
            if robot not in paths:
                paths[robot] = [start]
                
            if paths[robot][-1] != start:
                print("Error : last move of %s was to %s. Next move is %s" %(robot, paths[robot][-1], actionName))
                sys.exit(1)

            paths[robot].append(end)
            
        elif d["type"] == "communicate-meta":
            plt.arrow(start[0], start[1], end[0] - start[0], end[1] - start[1], color="cyan", length_includes_head=True)
        
        elif d["type"] == "observe":
            plt.arrow(start[0], start[1], end[0] - start[0], end[1] - start[1], color="yellow", length_includes_head=True)
            

    plt.plot([], [], color="cyan", label="communication")
    plt.plot([], [], color="yellow", label="observation")

    inits = [p[0] for p in paths.values()]
    plt.plot([pt[0] for pt in inits], [pt[1] for pt in inits], color="black", marker="s", label="init pos", alpha=0.7, linestyle="")

    colors = itertools.cycle(["r", "b", "g", "m"])

    for robot,p in sorted(paths.items()):
        #get color
        if mission is not None and robot in mission["agents"]:
            c = mission["agents"][robot]["color"]
        else:
            c = next(colors)
        for start, end in zip(p, p[1:]):
            plt.arrow(start[0], start[1], end[0] - start[0], end[1] - start[1], color=c, head_width=2, head_length=4, length_includes_head=True)
        
        #name = {"mana" : "AGV1", "minnie" : "AGV2", "ressac1" : "AAV1", "ressac2" : "AAV2"}
        
        #draw all paths in transparent to fix the borders and put the legend
        plt.plot([pt[0] for pt in p], [pt[1] for pt in p], c=c, marker=" ", label=robot)

    try:
        from scipy.misc import imread
        if mission is not None:
            img = imread(os.path.expandvars(os.path.join(mission["home_dir"], mission["map_data"]["image_file"])))
            size = mission["map_data"]["map_size"]
            plt.imshow(img, extent=[float(size["x_min"]), float(size["x_max"]), float(size["y_min"]), float(size["y_max"])])
    except ImportError:
        print("Cannot import scipy. No background")

    plt.legend(loc="upper center", bbox_to_anchor=(0.5, 1.1), fancybox=True, shadow=True, ncol=4)

    if outputFile is not None:
        plt.savefig(outputFile, bbox_inches='tight')
    return fig

def drawPlanTimeline(filename, outputFile = None, onlyMove = False):
    plt.clf()
    fig = plt.figure(1)
    data = getActionsFromPlan(filename)
    
    actions = {}
    actionsType = set([d["type"] for d in data if "type" in d])
    if onlyMove:
        actionsType = set([t for t in actionsType if "move" in t])

    for robot in set([d["robot"] for d in data if "robot" in d]):
        actions[robot] = {}
        for a in actionsType:
            actions[robot][a] = []

    for d in data:
        if onlyMove and "move" not in d["type"]: continue

        if "robot1" in d:
            actions[d["robot1"]][d["type"]] += [(float(d["tStart"]), float(d["dur"]))]
            actions[d["robot2"]][d["type"]] += [(float(d["tStart"]), float(d["dur"]))]
        else:
            actions[d["robot"]][d["type"]] += [(float(d["tStart"]), float(d["dur"]))]
            
            
    def timelines(y, xstart, xstop, color='b'):
        """Plot timelines at y from xstart to xstop with given color."""   
        plt.hlines(y, xstart, xstop, color, lw=4)
    
    colors = itertools.cycle(["k", "r", "g", "b"])
    captions = ["{a}-{ac}".format(a=agent, ac=action) for agent, b in actions.items() for action in b.keys()]
    y = 0
    for agent, v in actions.items():
    #    y += 1
        for action, ins in v.items():
            for i in ins:
                timelines(y, i[0], i[0]+i[1], next(colors))
            y += 1

    #Setup the plot
    ax = plt.gca()
    ax.set_yticks(range(len(captions)))
    ax.set_yticklabels(captions)
    plt.xlabel('Time')
    
    if outputFile is not None:
        plt.savefig(outputFile, bbox_inches='tight')

def main(argv):

    parser = argparse.ArgumentParser(description='Create a visualisation for PDDL plans')
    parser.add_argument('--missionFile', type=str, default=None)
    parser.add_argument('pddlFile', type=str)
    parser.add_argument('--outputFile', type=str, default=None)
    parser.add_argument('--type', choices=["geo", "timeline", "timelineMove"], default="geo")
    args = parser.parse_args()


    planFile = args.pddlFile
    if not planFile.endswith(".pddl"):
        print("Warning : are you sure you provided a pddl file ? : %s" % planFile)
    outputFile = args.outputFile
    
    if not os.access(planFile, os.R_OK):
        print("Cannot open %s" % planFile)
        sys.exit(1)

    if args.type == "geo":
        drawPlanGeo(planFile, outputFile, missionFile=args.missionFile)
    elif args.type == "timeline":
        drawPlanTimeline(planFile, outputFile)
    elif args.type == "timelineMove":
        drawPlanTimeline(planFile, outputFile, onlyMove = True)

    plt.show()


if __name__ == '__main__':
    sys.exit( main(sys.argv) )
