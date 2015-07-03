#!/usr/bin/env python

import matplotlib
matplotlib.use('Qt4Agg')
import matplotlib.pyplot as plt
import matplotlib.animation as animation

import itertools
import json
import os
import re
import sys

import argparse

def getCoord(ptName):
    l = ptName.split("_")
    return (float(l[1])/100, float(l[2])/100)

"""
Return a list of actions. Each action is a dictionary with a "type" field indication
the type of action. It contains at least a field "tStart", "dur, and "name".
The other actions depends on the type : "robot", "start", "end", ...
"""
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
                d = {"tStart":float(tStart), "dur":float(dur), "name":actionName}
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
            start = d["start1"]
            end = d["start2"]
            if start[0] != start[1] and end[0] - start[0] != end[1] - start[1]:
                plt.arrow(start[0], start[1], end[0] - start[0], end[1] - start[1], color="cyan", length_includes_head=True)
        
        elif d["type"] == "observe":
            start = d["start"]
            end = d["end"]
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
            plt.imshow(img, extent=[float(size["x_min"]), float(size["x_max"]), float(size["y_min"]), float(size["y_max"])], alpha=0.5)
    except ImportError:
        print("Cannot import scipy. No background")

    plt.legend(loc="upper center", bbox_to_anchor=(0.5, 1.15), fancybox=True, shadow=True, ncol=4)

    if mission is not None:
        size = mission["map_data"]["map_size"]
        ax = plt.gca()
        ax.set_ylim((float(size["y_min"]), float(size["y_max"]) + 20))
    
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
    ax.set_yticks(range(len(captions) + 1))
    ax.set_yticklabels(captions)
    plt.xlabel('Time')
    
    if outputFile is not None:
        plt.savefig(outputFile, bbox_inches='tight')

def drawPlanTime(filename, outputFile = None, missionFile=None):
    if missionFile is not None:
        with open(missionFile) as f:
            mission = json.load(f)
    else:
        mission = None

    plt.clf()
    fig = plt.figure(1)
    data = getActionsFromPlan(filename)

    tMax = max([float(d["tStart"]) + float(d["dur"]) for d in data])

    timeScale = 1

    lines = {}
    paths = {}
    points = []
    coms = []
    for d in data:
        if d["type"] == "move":
            robot = d["robot"]
            start = d["start"]
            end = d["end"]

            points.append(start); points.append(end)
            if robot not in paths:
                paths[robot] = []

            paths[robot].append({"tStart" : d["tStart"], "tEnd" : d["tStart"] + d["dur"], "wpStart":start, "wpEnd":end})
        elif d["type"] == "communicate":
            start = d["start1"]
            end = d["start2"]

            coms.append({"tStart" : d["tStart"], "tEnd" : d["tStart"] + d["dur"], "wpStart":start, "wpEnd":end})

    #TODO : use mission colors
    colors = itertools.cycle(["k", "r", "g", "b"])
    for robot in sorted(paths.keys()):
        if mission is not None and robot in mission["agents"]:
            c = mission["agents"][robot]["color"]
        else:
            c = next(colors)

        lines[robot], = plt.plot([], [], "-", color=c, label=robot)

    lineComs = []
    for i in range(len(coms)):
        lineCom, = plt.plot([], [], 'r-', linewidth=3)
        lineComs.append(lineCom)

    def update(num):
        #print num

        t = num / timeScale

        for robot,path in paths.items():
            p = [sorted(path, key= lambda x:x["tStart"])[0]["wpStart"]] #path to draw

            for d in sorted(path, key= lambda x:x["tStart"]):
                if d["tEnd"] < t:
                    p.append(d["wpEnd"])
                elif d["tStart"] < t and d["tEnd"] > t:
                    index = (t - d["tStart"])/(d["tEnd"] - d["tStart"])
                    p.append( [d["wpStart"][0] + index*(d["wpEnd"][0] - d["wpStart"][0]), d["wpStart"][1] + index*(d["wpEnd"][1] - d["wpStart"][1])])
                else:
                    break
            lines[robot].set_data([a[0] for a in p], [a[1] for a in p])

        data = []
        for i,c in enumerate(coms):
            if False or (c["tStart"] - 5 <= t and t <= c["tEnd"] + 5): #add some leeway to see it
                lineComs[i].set_data([c["wpStart"][0], c["wpEnd"][0]], [c["wpStart"][1], c["wpEnd"][1]])
            else:
                lineComs[i].set_data([], [])

        return (list(lines.values()) + lineComs)

    plt.xlim(min([p[0] for p in points]), max([p[0] for p in points]))
    plt.ylim(min([p[1] for p in points]), max([p[1] for p in points]))
    plt.gca().set_aspect('equal', adjustable='box')
    ani = animation.FuncAnimation(fig, update, int(tMax*timeScale), interval=100, blit=True, repeat=False)

    try:
        from scipy.misc import imread
        if mission is not None:
            img = imread(os.path.expandvars(os.path.join(mission["home_dir"], mission["map_data"]["image_file"])))
            size = mission["map_data"]["map_size"]
            plt.imshow(img, extent=[float(size["x_min"]), float(size["x_max"]), float(size["y_min"]), float(size["y_max"])], alpha=0.5)
    except ImportError:
        print("Cannot import scipy. No background")

    plt.legend(loc="upper center", bbox_to_anchor=(0.5, 1.15), fancybox=True, shadow=True, ncol=3)

    if outputFile is not None:
        ani.save(outputFile, writer="mencoder")

def main(argv):

    parser = argparse.ArgumentParser(description='Create a visualisation for PDDL plans')
    parser.add_argument('--missionFile', type=str, default=None)
    parser.add_argument('pddlFile', type=str)
    parser.add_argument('--outputFile', type=str, default=None)
    parser.add_argument('--type', choices=["geo", "timeline", "timelineMove", "time"], default="geo")
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
    elif args.type == "time":
        drawPlanTime(planFile, outputFile, missionFile=args.missionFile)

    plt.show()


if __name__ == '__main__':
    sys.exit( main(sys.argv) )
