import kdt
import time
import getopt
import sys
from stats import splitthousands

scale = 50
sample = 0.05
batchSize = -1
file = ""
BCdebug=0

useTorus = True

def usage():
	print "BetwCent.py [-sSCALE] [-xSAMPLE] [-fFILE]"
	print "SCALE refers to the size of the generated Torus graph G. G will have SCALE^2 vertices."
	print "SAMPLE refers to the fraction of vertices to use as SSSP starts. 1.0 = exact BC."
	print "FILE is a MatrixMarket .mtx file with graph to use. Graph should be directed and symmetric"
	print "Default is: python BetwCent.py -s%d -x%f"%(scale, sample)

try:
	opts, args = getopt.getopt(sys.argv[1:], "hs:f:x:b:d", ["help", "scale=", "file=", "sample=", "batchsize", "debug"])
except getopt.GetoptError, err:
	# print help information and exit:
	print str(err) # will print something like "option -a not recognized"
	usage()
	sys.exit(2)
output = None
verbose = False
for o, a in opts:
	if o in ("-h", "--help"):
		usage()
		sys.exit()
	elif o in ("-s", "--scale"):
		scale = int(a)
	elif o in ("-x", "--sample"):
		sample = float(a)
	elif o in ("-b", "--batch"):
		batchSize = int(a)
	elif o in ("-f", "--file"):
		file = a
		useTorus = False
	elif o in ("-d", "--debug"):
		BCdebug = 1
	else:
		assert False, "unhandled option"
		
		
# setup the graph
if len(file) == 0:
	if kdt.master():
		print "Generating a Torus graph with 2^%d vertices..."%(scale)

	G1 = kdt.DiGraph.twoDTorus(scale)
	nverts = G1.nvert()
	if kdt.master():
		print "Graph has",nverts,"vertices."
	#G1.toBool()
else:
	if kdt.master():
		print 'Loading %s'%(file)
	G1 = kdt.DiGraph.load(file)
	#G1.toBool()

# Call BC	
before = time.time();
bc, nStartVerts = G1.centrality('approxBC', sample=sample, BCdebug=BCdebug, batchSize=batchSize, retNVerts=True)
time = time.time() - before;

# Check
if useTorus and ((bc - bc[0]) > 1e-15).any():
	if kdt.master():
		print "not all vertices have same BC value"

# Report
nedges = G1._spm.getnee()*nStartVerts
TEPS = float(nedges)/time
min = bc.min()
max = bc.max()
mean = bc.mean()
std = bc.std()
if kdt.master():
	print "bc[0] = %f, min=%f, max=%f, mean=%f, std=%f" % (bc[0], min, max, mean, std)
	print "   used %d starting vertices" % nStartVerts
	print "   took %4.3f seconds" % time
	print "   TEPS = %s (assumes the graph was connected)" % splitthousands(TEPS)

