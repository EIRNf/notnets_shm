
import re
from collections import OrderedDict

def parseData(dirn, fname):
  xs = []
  ysPerSolver = OrderedDict()
  ydevsPerSolver = OrderedDict()
  with open(dirn+"/"+fname+".dat") as fin:
    lines = fin.readlines()
    for line in lines:
      if line.startswith("#"):
        continue
      (solver, x, y, ydev, nline) = re.split("[\t]", line)
      x = float(x)
      try:
        y = float(y)
        ydev = float(ydev)  
      except ValueError:
        y = 0.0
        ydev = 0.0  
      if solver in ysPerSolver:
        ysPerSolver[solver].append(y)
        ydevsPerSolver[solver].append(ydev)
      else:
        ysPerSolver[solver] = [y]
        ydevsPerSolver[solver] = [ydev]
      if len(xs) == 0 or x > xs[-1]:
        xs.append(x)
    return (xs, ysPerSolver, ydevsPerSolver)
  

def parseVariableData(dirn, fname):
  xsPerSolver  = OrderedDict()
  ysPerSolver = OrderedDict()
  ydevsPerSolver = OrderedDict()
  with open(dirn+"/"+fname+".dat") as fin:
    lines = fin.readlines()
    for line in lines:
      if line.startswith("#"):
        continue
      (solver, x, y, ydev, nline) = re.split("[\t]", line)
      try:
        x = float(x)
        y = float(y)
        ydev = float(ydev)  
      except ValueError:
        x = 0.0
        y = 0.0
        ydev = 0.0  
      if solver in xsPerSolver:
        xsPerSolver[solver].append(x)
        ysPerSolver[solver].append(y)
        ydevsPerSolver[solver].append(ydev)
      else:
        xsPerSolver[solver] = [x]
        ysPerSolver[solver] = [y]
        ydevsPerSolver[solver] = [ydev]
    return (xsPerSolver, ysPerSolver, ydevsPerSolver)
  
def parseVariableDataTwoXs(dirn, fname):
  x1PerSolver  = OrderedDict()
  x2PerSolver  = OrderedDict()
  ysPerSolver = OrderedDict()
  ydevsPerSolver = OrderedDict()
  with open(dirn+"/"+fname+".dat") as fin:
    lines = fin.readlines()
    for line in lines:
      if line.startswith("#"):
        continue
      (solver, x1, x2, y, ydev, nline) = re.split("[\t]", line)
      try:
        x1 = float(x1)
        x2 = float(x2)
        y = float(y)
        ydev = float(ydev)  
      except ValueError:
        x1 = 0.0
        x2 = 0.0
        y = 0.0
        ydev = 0.0  
      if solver in x1PerSolver:
        x1PerSolver[solver].append(x1)
        x2PerSolver[solver].append(x2)
        ysPerSolver[solver].append(y)
        ydevsPerSolver[solver].append(ydev)
      else:
        x1PerSolver[solver] = [x1]
        x2PerSolver[solver] = [x2]
        ysPerSolver[solver] = [y]
        ydevsPerSolver[solver] = [ydev]
    return (x1PerSolver, x2PerSolver,ysPerSolver, ydevsPerSolver)
  
#Parse tije
def parseVariableDataTwoXs_Timestamp(dirn, fname):
  x1PerSolver  = OrderedDict()
  x2PerSolver  = OrderedDict()
  ysPerSolver = OrderedDict()
  with open(dirn+"/"+fname+".csv") as fin:
    lines = fin.readlines()
    for line in lines:
      if line.startswith("#"):
        continue
      (solver,  x1,num_clients, y, x2) = line.split()   #re.split("[\t]", line)
      try:
        x1 = float(x1)
        x2 = float(x2)
        y = float(y)
        # ydev = float(ydev)  
      except ValueError:
        x1 = 0.0
        x2 = 0.0
        y = 0.0
        # ydev = 0.0  
      if solver in x1PerSolver:
        x1PerSolver[solver].append(x1)
        x2PerSolver[solver].append(x2)
        ysPerSolver[solver].append(y)
        # ydevsPerSolver[solver].append(ydev)
      else:
        x1PerSolver[solver] = [x1]
        x2PerSolver[solver] = [x2]
        ysPerSolver[solver] = [y]
        # ydevsPerSolver[solver] = [ydev]
    return (x1PerSolver, x2PerSolver,ysPerSolver)
  
  
  #Parse tije
def parseVariableDataTwoXs_MoreLatencies(dirn, fname):
  x1PerSolver  = OrderedDict()
  x2PerSolver  = OrderedDict()
  ysPerSolver = OrderedDict()
  y1PerSolver = OrderedDict()
  y2PerSolver = OrderedDict()
  y3PerSolver = OrderedDict()


  with open(dirn+"/"+fname+".csv") as fin:
    lines = fin.readlines()
    for line in lines:
      if line.startswith("#"):
        continue
      (solver,  x1,num_clients, y,x2,y1,y2,y3) = line.split()   #re.split("[\t]", line)
      try:
        x1 = float(x1)
        x2 = float(x2)
        y = float(y)
        y1 = float(y1)
        y2 = float(y2)
        y3 = float(y3)
        # ydev = float(ydev)  
      except ValueError:
        x1 = 0.0
        x2 = 0.0
        y = 0.0
        y1 = 0.0
        y2 = 0.0
        y3 = 0.0
        # ydev = 0.0  
      if solver in x1PerSolver:
        x1PerSolver[solver].append(x1)
        x2PerSolver[solver].append(x2)
        ysPerSolver[solver].append(y)
        y1PerSolver[solver].append(y1)
        y2PerSolver[solver].append(y2)
        y3PerSolver[solver].append(y3)
        # ydevsPerSolver[solver].append(ydev)
      else:
        x1PerSolver[solver] = [x1]
        x2PerSolver[solver] = [x2]
        ysPerSolver[solver] = [y]
        y1PerSolver[solver] = [y1]
        y2PerSolver[solver] = [y2]
        y3PerSolver[solver] = [y3]
        # ydevsPerSolver[solver] = [ydev]
    return (x1PerSolver, x2PerSolver,ysPerSolver, y1PerSolver,y2PerSolver,y3PerSolver)
  
