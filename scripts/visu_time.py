#!/usr/bin/env python

from __future__ import division

import matplotlib
matplotlib.use('TKAgg')

import matplotlib.pyplot as plt
import matplotlib.animation as animation

import os
import re
import sys
from itertools import cycle

######   Parameters


timeScale = 1


############

paths = {}
lines = {}
tMax = 0
points = []

def getCoord(ptName):
    l = ptName.split("_")
    return (float(l[1])/100, float(l[2])/100)
    
def parsePlan(fileName):
    global paths
    global tMax
    global points

    with open(fileName, "r") as f:
        for line in f:
            if line.startswith(";"):
                continue

            #match an action in PDDL and extract its name            
            m = re.match("^(\d*(?:.\d*)?)\s*:\s*\((.*)\)\s*\[(\d*(?:.\d*)?)\]", line)
            if m:
                tStart, actionName, dur = m.groups()
                if actionName.startswith("move"):
                    _,robot,start,end = actionName.split(" ")
                    start = getCoord(start)
                    end = getCoord(end)
                    
                    if robot not in paths:
                        paths[robot] = []
                      
                    paths[robot].append({"tStart" : float(tStart), "tEnd" : float(tStart)+float(dur), "wpStart":start, "wpEnd":end})
                    tMax = max(tMax, float(tStart)+float(dur))
                    points.append(start)
                    points.append(end)

path = [ {"tStart":0, "tEnd":1, "wpStart" : [0,0], "wpEnd" : [0.5,1]},
         {"tStart":1, "tEnd":5, "wpStart" : [0.5,1], "wpEnd" : [1,0.5]} ]

def update(num):
    #print num

    for robot,path in paths.items():
        p = [sorted(path, key= lambda x:x["tStart"])[0]["wpStart"]] #path to draw

        t = num / timeScale
        for d in sorted(path, key= lambda x:x["tStart"]):
            if d["tEnd"] < t:
                p.append(d["wpEnd"])
            elif d["tStart"] < t and d["tEnd"] > t:
                index = (t - d["tStart"])/(d["tEnd"] - d["tStart"])
                p.append( [d["wpStart"][0] + index*(d["wpEnd"][0] - d["wpStart"][0]), d["wpStart"][1] + index*(d["wpEnd"][1] - d["wpStart"][1])])
            else:
                break
        lines[robot].set_data([a[0] for a in p], [a[1] for a in p])

    return (lines.values())
    

def main(argv):
    planFile = None
    
    if len(argv) < 2:
        print("Usage : visu.py plan.pddl [save.mp4]")
        sys.exit(1)
    else:
        planFile = argv[1]
        if not os.access(planFile, os.R_OK):
            print("Cannot open %s" % planFile)
            sys.exit(1)
            
    saveFile = None
    if len(argv) >= 3:
      saveFile = argv[2]

    fig = plt.figure(1)

    parsePlan(planFile)
        
    global lines

    colors = cycle(["k", "r", "g", "b"])
    for robot in paths.keys():
        lines[robot], = plt.plot([], [], "-", color=next(colors))
    l1, = plt.plot([], [], 'r-')


    plt.xlim(min([p[0] for p in points]), max([p[0] for p in points]))
    plt.ylim(min([p[1] for p in points]), max([p[1] for p in points]))
    plt.gca().set_aspect('equal', adjustable='box')
    ani = animation.FuncAnimation(fig, update, int(tMax*timeScale), interval=100, blit=True, repeat=False)
    
    if saveFile is not None:
        ani.save(saveFile, writer="mencoder")

    plt.show()


if __name__ == '__main__':
    sys.exit( main(sys.argv) )

