#!/usr/bin/env python3

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

import itertools
import os
import re
import sys

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
    
def drawPlanGeo(filename, outputFile = None):
    plt.clf()
    fig = plt.figure(1)
    plt.axis('equal')
    
    paths = {}
    
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

    colors = itertools.cycle(["r", "b", "g", "m"])

    for robot,p in paths.items():
        c = next(colors)
        for start, end in zip(p, p[1:]):
            plt.arrow(start[0], start[1], end[0] - start[0], end[1] - start[1], color=c, head_width=2, head_length=4, length_includes_head=True)
        
        #name = {"mana" : "AGV1", "minnie" : "AGV2", "ressac1" : "AAV1", "ressac2" : "AAV2"}
        
        #draw all paths in transparent to fix the borders and put the legend
        plt.plot([pt[0] for pt in p], [pt[1] for pt in p], c=c, marker=" ", label=robot)
    
    inits = [p[0] for p in paths.values()]
    plt.plot([pt[0] for pt in inits], [pt[1] for pt in inits], color="black", marker="s", label="init pos", alpha=0.7, linestyle="")
    
    #plt.legend(loc="lower right")
    plt.legend(loc="best")
       
    if outputFile is not None:
        plt.savefig(outputFile, bbox_inches='tight')
    return fig

def drawPlanTimeline(filename, outputFile = None):
    plt.clf()
    fig = plt.figure(1)
    data = getActionsFromPlan(filename)
    
    actions = {}
    actionsType = set([d["type"] for d in data if "type" in d])
    for robot in set([d["robot"] for d in data if "robot" in d]):
        actions[robot] = {}
        for a in actionsType:
            actions[robot][a] = []

    for d in data:
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

    planFile = None
    
    if len(argv) < 2:
        print("Usage : actionvisu.py plan.pddl [plan.png]")
        sys.exit(1)
    else:
        planFile = argv[1]
        if len(argv) > 2:
            outputFile = argv[2]
        else:
            outputFile = None
        if not os.access(planFile, os.R_OK):
            print("Cannot open %s" % planFile)
            sys.exit(1)

    #fig = drawPlanGeo(planFile, outputFile)
    drawPlanTimeline(planFile, outputFile)
    plt.show()


if __name__ == '__main__':
    sys.exit( main(sys.argv) )
