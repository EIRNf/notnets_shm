import CommonConf
import CommonViz

import re
from collections import OrderedDict
import numpy as np
import matplotlib.pyplot as pp

EXPERIMENT_NAME = "exampleExp"
X_LABEL         = "Parameter"
Y_LABEL         = "Metric "
            
def main(dirn, fname): 
  (xs, ysPerSolver, ydevsPerSolver) = CommonViz.parseData(dirn, fname)
     
  CommonConf.setupMPPDefaults()
  fmts = CommonConf.getLineFormats()
  mrkrs = CommonConf.getLineMarkers()
  fig = pp.figure()
  ax = fig.add_subplot(111)
  #ax.set_xscale("log")
  #ax.set_yscale("log" )

  index = 0
  for (solver, ys), (solver, ydevs) in zip(ysPerSolver.items(),ydevsPerSolver.items()) : 
    ax.errorbar(xs, ys, yerr=ydevs, label=solver, marker=mrkrs[index], linestyle=fmts[index])
    index = index + 1

  ax.set_xlabel(X_LABEL);
  ax.set_ylabel(Y_LABEL);
  ax.legend(loc='best', fancybox=True)

  pp.savefig(dirn+"/"+fname+".pdf")
  pp.show()

if __name__ == "__main__":
  main("expData", EXPERIMENT_NAME)

