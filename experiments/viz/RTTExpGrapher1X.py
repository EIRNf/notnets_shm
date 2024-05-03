import CommonConf
import CommonViz

import re
from collections import OrderedDict
import numpy as np
import matplotlib.pyplot as pp
import matplotlib

EXPERIMENT_NAME = "tcp_sem"
X_LABEL         = "throughput(ops/ms)"
Y_LABEL         = "latency(ns/op) "
            
def main(dirn, fname): 
  (xsPerSolver, ysPerSolver, ydevsPerSolver) = CommonViz.parseVariableData(dirn, fname)
     
  CommonConf.setupMPPDefaults()
  fmts = CommonConf.getLineFormats()
  mrkrs = CommonConf.getLineMarkers()
  colors = CommonConf.getColors()
  fig = pp.figure()
  ax = fig.add_subplot(111)
  # ax.set_xscale("log")
  # ax.set_yscale("log" )

  index = 0
  for (solver,xs), (solver, ys), (solver, ydevs) in zip(xsPerSolver.items(),ysPerSolver.items(),ydevsPerSolver.items()) : 
    ax.errorbar(xs, ys, yerr=ydevs, label=solver, marker=mrkrs[index], linestyle=fmts[index], color=colors[index])
    index = index + 1

  ax.set_xlabel(X_LABEL);
  ax.set_ylabel(Y_LABEL);
  ax.legend(loc='best', fancybox=True)

  matplotlib.layout_engine.TightLayoutEngine().execute(fig)

  pp.savefig(dirn+"/"+fname+".pdf")
  pp.show()

if __name__ == "__main__":
  main("expData", EXPERIMENT_NAME)
