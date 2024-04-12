import random
import CommonConf
import CommonViz

import re
from collections import OrderedDict
import numpy as np
import matplotlib.pyplot as pp
import matplotlib

EXPERIMENT_NAME = "poll_boost"
X1_LABEL         = "num_clients"
X2_LABEL         = "throughput(ops/us)"
Y_LABEL         = "latency(us/op) "
            
def main(dirn, fname): 
  (x1PerSolver, x2PerSolver ,ysPerSolver) = CommonViz.parseVariableDataTwoXs_Timestamp(dirn, fname)
     
  CommonConf.setupMPPDefaults()
  fmts = CommonConf.getLineFormats()
  mrkrs = CommonConf.getLineMarkers()
  colors = CommonConf.getColors()
  fig = pp.figure(figsize=(12, 5))
  ax = fig.add_subplot(111)

  # x1 = x1PerSolver["SEM_QUEUE"]

  # ax.set_xscale("log")
  # ax.set_yscale("log" )


  index = 0
  for (solver,xs), (solver, ys), (solver, x1) in zip(x2PerSolver.items(),ysPerSolver.items(),x1PerSolver.items()) : 
    ax.errorbar(xs, ys, label=solver, marker=mrkrs[index], linestyle=fmts[index], color=colors[index])
    for i, ix1 in enumerate(x1):
      # pos = int(i-1)
      pos1 = random.randint(5,75)
      pos2 = random.randint(-75,75)
      pp.annotate(ix1,(xs[i], ys[i]), xytext=(pos1, pos2), textcoords='offset points', arrowprops=dict(arrowstyle="->"))
    index = index + 1

  # index = 0
  # for (solver, x1) in zip(x1PerSolver.items()):
  #   pp.annotate(x1, )
  #   index = 0

  # x1 = x1PerSolver["TCP"]
  # for i, label in enumerate(x1)
  #   pp.annotate(x1)

  # ax2 = ax.twiny()
  # ax2.set_xlabel(X1_LABEL)
  # ax2.plot(x1,ys)# Data for the top x-axis
  # ax2.set_xticks(x1) 
  # ax2.set_xticks(np.linspace(x1[0], x1[-1], 21))
  # ax2.set_xlim([x1[0], x1[-1]])


  ax.set_xlabel(X2_LABEL);
  ax.set_ylabel(Y_LABEL);
  ax.legend(loc='best', fancybox=True)

  matplotlib.layout_engine.TightLayoutEngine().execute(fig)

  pp.savefig(dirn+"/"+fname+".pdf")
  pp.show()

if __name__ == "__main__":
  main("expData", EXPERIMENT_NAME)

