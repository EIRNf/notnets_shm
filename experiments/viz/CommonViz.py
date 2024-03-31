
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
  
