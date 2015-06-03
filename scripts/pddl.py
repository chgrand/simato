import re

class Domain:
    def __init__(self, name):
        self.name = name
        self.requirements = []
        self.types = []
        self.predicates = []
        self.functions = []
        self.actions = []
    
    def addRequirement(self, req):
        if req.startswith(":"):
            req = req[1:]
    
        if not req in ["typing", "equality", "durative-actions", "strips", "agents-def"]:
            print("pddl.Domain.addRequirement : req is not recognized : %s" % req)
            return
            
        self.requirements.append(":" + req)
    
    def addType(self, type, supertype = "object"):
        self.types.append((type, supertype))
    
    def addPredicate(self, pred):
        self.predicates.append(pred)
        
    def addFunction(self, func):
        self.functions.append(func)
    
    def addAction(self, a):
        if not isinstance(a, Action):
            print("pddl.Domain.addAction : this is not an action : %s" % str(type(a)))
            return
            
        self.actions.append(a)
        
    def toString(self):
        result = """
(define (domain {name})
  (:requirements {reqList})
  (:types {typeList})
  (:predicates {predList})
  (:functions {funcList})

  {actionList}
)
""".format(name = self.name,
           reqList = " ".join(self.requirements),
           typeList = "\n    ".join([t[0] + " - " + t[1] for t in self.types]),
           predList = "\n    ".join(["(" + p + ")" for p in self.predicates]),
           funcList = "\n    ".join(["(" + f + ")" for f in self.functions]),
           actionList = "\n".join([a.toString() for a in self.actions]))
           
        return result

class Action:
    def __init__(self, name, isDurative = False):
        self.name = name
        self.parameters = []
        self.agent = ""
        self.conflicts = []
        self.prec = []
        self.eff = []
        self.sideEff = []
        
        self.duration = 1.0
        self.isDurative = isDurative

        self.methods = []
    
    def addParameter(self, param, type):
        
        if not param.startswith("?"):
            print("pddl.Action.addParameter : expecting parameter name to start with ?, got %s" % str(param))
            return

        self.parameters.append((type, param))
        #if type not in self.parameters:
        #    self.parameters[type] = []
        #self.parameters[type].append(param)
        
    def addAgent(self, agent):
        if self.agent:
            print("pddl.Action.addAgent : there is already an agent defined : %s. Will not use %s for %s." %(self.agent, self.name, agent))
        else:
            self.agent = agent
        
    def addConflict(self, conflict):
        self.conflicts.append(conflict)

    def addPrecs(self, *arg):
        for e in arg:
            if type(e)==str:
                self.addPrec(e)
            else:
                self.addPrec(e[0], e[1])


    def addPrec(self, literal, tag = ""):
        if not self.validateLit(literal, tag):
            return
            
        if tag:
            self.prec.append("(" + tag + " (" + literal + "))")
        else:
            self.prec.append("(" + literal + ")")
    
    def addEffs(self, *arg):
        for e in arg:
            if type(e)==str:
                self.addEff(e)
            else:
                self.addEff(e[0], e[1])
    
    def addEff(self, literal, tag = ""):
        if not self.validateLit(literal, tag):
            return
            
        if tag == "over all":
            print("pddl.Action.addEff : over all is not an allowed tag in an effect : %s" % literal)
            return
            
        if tag:
            self.eff.append("(" + tag + " (" + literal + "))")
        else:
            self.eff.append("(" + literal + ")")
            
    def addSideEff(self, literal, tag = ""):
        if not self.validateLit(literal, tag):
            return
            
        if tag == "over all":
            print("pddl.Action.addSideEff : over all is not an allowed tag in an side effect : %s" % literal)
            return
            
        if tag:
            self.sideEff.append("(" + tag + " (" + literal + "))")
        else:
            self.sideEff.append("(" + literal + ")")
            
    def validateLit(self, literal, tag):
        if tag and not self.isDurative:
            print("pddl.Action.addLit : adding a temporal tag to a non durative action : %s" % tag)
            return False
            
        if self.isDurative and not tag:
            print("pddl.Action.addLit : no tag in a durative action")
            tag = "over all"

        if tag and not tag in ["at start", "at end", "over all"]:
            print("pddl.Action.addLit : invalid tag : %s" % tag)
            return False
 
        if not re.match(r"^(not(\s)*\()?[a-z0-9 \-_?=]+(\))?$", literal):
            print("pddl.Action.addLit : invalid literal ?, not recognized by regex : %s" % literal)

        if literal.count("(") - literal.count(")") != 0:
            print("pddl.Action.addLit : invalid literal, parenthesis mismatch : %s" % literal)
            return False
            
        return True
    
    def setDuration(self, dur):
        if type(dur) == float:
            dur = float(int(dur*1000))/1000
        self.duration = dur
        
    def addMethod(self, method):
        if not isinstance(method, Method):
            print("pddl.Action.addMethod : was expecting a method here %s" % type(method))
            
        self.methods.append(method)

    def toString(self):

        conflictList = ""
        if self.conflicts:
            conflictList = ":conflict-with %s" % " ".join(["(" + c + ")" for c in self.conflicts])
    
        sideEffList = ""
        if self.sideEff:
            sideEffList = ":side-effect (and %s)" % " ".join(self.sideEff)
    
        methodList = ""
        if self.methods:
            methodList = ":methods(\n%s\n  )" % "\n".join([m.toString() for m in self.methods])
    
        result = """
({actionType} {name}
  :parameters ({paramList})
  {agent}
  {conflictList}
  {duration}
  {precType} {precList}
  :effect {effList}
  {sideEffList}
  {methodList}
)
""".format(actionType = (":durative-action" if self.isDurative else ":action"),
           name=self.name,
           paramList="\n       ".join([param + " - " + type for type,param in self.parameters]),
           agent=(":agent (%s)" % self.agent if self.agent else ""),
           conflictList = conflictList,
           duration = (":duration (= ?duration %s)" % self.duration if self.isDurative else ""),
           precType = (":condition" if self.isDurative else ":precondition"),
           precList= "(and " + " ".join(self.prec) + ")" if self.prec else "",
           effList= "(and " + " ".join(self.eff) + ")" if self.eff else "()",
           sideEffList=sideEffList,
           methodList = methodList)
        return result

class Method:
    def __init__(self, name):
        self.name = name
        self.actions = []
        self.duration = None
        self.preconditions = []
        self.causalLinks = []
        self.temporalLinks = []
        
    def addAction(self, name, literal):
        self.actions.append( (name, literal) )
        
    def setDuration(self, dur):
        self.duration = dur
        
    def addCausalLink(self, start, end, literal):
        self.causalLinks.append( (start, end, literal) )
        
    def addTemporalLink(self, start, end):
        self.temporalLinks.append( (start, end) )
        
    def toString(self):
        result = """
      :method {name}
      :actions {actionList}
      {duration}
      :precondition
      :causal-links {causalLinks}
      :temporal-links {temporalLinks}
""".format(name = self.name,
                actionList = "\n        ".join(["(" + a[0] + " (" + a[1] + "))" for a in self.actions]),
                duration = ":duration (= ?duration %s)" % self.duration if self.duration else "",
                causalLinks = "\n        ".join(["(" + c[0] + " " + c[1] + " (" + c[2] + "))" for c in self.causalLinks]),
                temporalLinks = "\n        ".join(["(" + c[0] + " " + c[1] + ")" for c in self.temporalLinks]))

        return result

class Helper:
    def __init__(self, domainName):
        self.domainName = domainName
        self.options = []
        self.allowedActions = []
        self.lowPriorityPredicates = []
        self.actions = []
        
    def addOption(self, opt):
        if opt.startswith(":"):
            opt = opt[1:]
            
        self.options.append(":" + opt)
        
    def addLowPriorityPredicate(self, pred):
        self.lowPriorityPredicates.append(pred)
        
    def addAllowedAction(self, actionName):
        self.allowedActions.append(actionName)
        
    def addAction(self, action):
        if not isinstance(action, Action):
            print("pddl.Helper.addAction : the parameter given is not of type pddl.Action : %s" % type(action))
            return
            
        self.actions.append(action)
        
    def toString(self):
        result = """
(define (domain-helper {name})
    (:options {optionList})
    (:allowed-actions {allowedActionList})
    (:low-priority-predicates {lowPriorityList})

    {actionList}
)
""".format(name = self.domainName,
            optionList = " ".join(self.options),
            allowedActionList = " ".join(self.allowedActions),
            lowPriorityList = " ".join(self.lowPriorityPredicates),
            actionList="\n".join([a.toString() for a in self.actions]))

        return result

class Problem:
    def __init__(self, name, domain):
        self.name = name
        self.domainName = domain
        self.objects = {}
        self.init = []
        self.goal = []
        
    def addObjects(self, type, *arg):
        for o in arg:
            self.addObject(o, type)
        
    def addObject(self, obj, type):
        if type not in self.objects:
            self.objects[type] = []
        self.objects[type].append(obj)
        
    def addInits(self, *arg):
        for i in arg:
            self.addInit(i)
        
    def addInit(self, literal):
        self.init.append(literal)
        
    def addInitFunc(self, literal, value):
        self.init.append("= (" + literal + ") " + str(value)) 
        
    def addGoals(self, *arg):
        for g in arg:
            self.addGoal(g)
        
    def addGoal(self, literal):
        self.goal.append(literal)
    
    def toString(self):
    
        objectList = "\n       ".join([" ".join(self.objects[k]) + " - " + str(k) for k in self.objects.keys()])
    
        result = """
(define (problem {name})
    (:domain {domainName})
    (:objects {objectList})
    (:init {initList})
    (:goal (and {goalList}))
)
""".format(name = self.name, 
        domainName = self.domainName, 
        objectList=objectList,
        initList="\n      ".join(["(" + l + ")" for l in self.init]),
        goalList="\n      ".join(["(" + l + ")" for l in self.goal]))

        return result

def main():
    a = Action("move")
    a.addParameter("toto", "tata")
    a.addParameter("alla", "olla")
    a.addParameter("?r", "robot")
    a.addPrec("tata ?a", "over all")
    a.addPrec("toto ?r")
    a.addPrec("toto ?r(")
    a.addPrec("toto* ?r")

    print(a.toString())

    b = Action("move2", True)
    b.addPrec("tata ?a", "over all")
    b.addPrec("tata ?a", "at start")
    b.addPrec("tata ?a")
    b.addEff("not tata ?a", "at end")

    print(b.toString())
    
    d = Domain("d")
    d.addRequirement(":typing")
    d.addRequirement("tptp")
    d.addPredicate("toto ?r - wp")
    d.addPredicate("toto ?r - wp")
    d.addPredicate("toto ?r - wp")
    d.addFunction("distance toto tata 28")
    d.addType("wp")
    d.addType("loc", "wp")
    d.addAction(a)
    d.addAction(b)
    print(d.toString())
    
    p = Problem("toto", "d")
    p.addObject("pt1", "wp")
    p.addObject("pt2", "wp")
    p.addObject("r1", "robot")
    
    p.addInit("at r1 pt1")
    p.addInitFunc("distance pt1 pt2", 23.4)
    p.addGoal("explored pt2")
    print(p.toString())
    
    h = Helper("action")
    h.addOption("erase")
    h.addAllowedAction("move")
    
    m = Method("toto")
    m.addAction("m", "move from to")
    m.addCausalLink(":init", "m", "at from")
    m.addTemporalLink("m", ":goal")
    b.addMethod(m)
    
    h.addAction(a)
    h.addAction(b)
    
    print(h.toString())

if __name__=="__main__":
    main()
