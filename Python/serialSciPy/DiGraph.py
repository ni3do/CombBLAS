import numpy as np
import scipy as sc
import scipy.sparse as sp
import Graph as gr

class DiGraph(gr.Graph):

	print "in DiGraph"

	def __init__(self, edgev, size):
		#  Keeping EdgeV independent of the number of vertices touched by an edge 
		#  creates some complications with creating sparse matrices, as 
		#  scipy.sparse.csr_matrix()expects each of i/j/v to have shape (N,1) or 
		#  (1,N), requiring each of i/j/v to be flatten()ed before calling csr_matrix.
		#print "in DiGraph/__init__"
		fv = edgev[1].flatten();
		fi = edgev[0][0].flatten();
		fj = edgev[0][1].flatten();
		self.spmat = sp.csr_matrix((fv, (fi,fj)), shape=size);
		#self.spmat = sp.csr_matrix((edgev[1].flatten(),(edgev[0][0].flatten()),edgev[0][1].flatten()), shape=(size));

	def degree(self):
		return self.indegree() + self.outdegree();

	def indegree(self):
		tmp = self._spones(self.spmat);
		return np.asarray(tmp.sum(0))

	def outdegree(self):
		tmp = self._spones(self.spmat);
		return np.asarray(tmp.sum(1).reshape(1,np.shape(tmp)[0]));

		

class DiEdgeV(gr.EdgeV):
	print "in DiEdgeV"

		

#	No VertexV class for now

#class VertexV():
#	print "in VertexV"
#
#	def __init__(self, ndces):


def torusEdges(n):
	N = n*n;
	nvec = sc.tile(sc.arange(n),(n,1)).T.flatten();	# [0,0,0,...., n-1,n-1,n-1]
	nvecil = sc.tile(sc.arange(n),n)			# [0,1,...,n-1,0,1,...,n-2,n-1]
	north = gr.Graph._sub2ind((n,n),sc.mod(nvecil-1,n),nvec);	
	south = gr.Graph._sub2ind((n,n),sc.mod(nvecil+1,n),nvec);
	west = gr.Graph._sub2ind((n,n),nvecil, sc.mod(nvec-1,n));
	east = gr.Graph._sub2ind((n,n),nvecil, sc.mod(nvec+1,n));
	Nvec = sc.arange(N);
	rowcol = sc.append((Nvec, north), (Nvec, west), axis=1);
	rowcol = sc.append(rowcol,        (Nvec, south), axis=1);
	rowcol = sc.append(rowcol,        (Nvec, east), axis=1);
	rowcol = rowcol.T;
	rowcol = (rowcol[:,0], rowcol[:,1]);
	return gr.EdgeV(rowcol, sc.tile(1,(N*4,)))

def Graph500Edges(n):
	print "NOTE:  Graph500Edges producing torusEdges currently"
	N = n*n;
	nvec = sc.tile(sc.arange(n),(n,1)).T.flatten();	# [0,0,0,...., n-1,n-1,n-1]
	nvecil = sc.tile(sc.arange(n),n)			# [0,1,...,n-1,0,1,...,n-2,n-1]
	north = gr.Graph._sub2ind((n,n),sc.mod(nvecil-1,n),nvec);
	south = gr.Graph._sub2ind((n,n),sc.mod(nvecil+1,n),nvec);
	west = gr.Graph._sub2ind((n,n),nvecil, sc.mod(nvec-1,n));
	east = gr.Graph._sub2ind((n,n),nvecil, sc.mod(nvec+1,n));
	Nvec = sc.arange(N);
	rowcol = sc.append((Nvec, north), (Nvec, west), axis=1)
	rowcol = sc.append(rowcol,        (Nvec, south), axis=1)
	rowcol = sc.append(rowcol,        (Nvec, east), axis=1)
	rowcol = rowcol.T
	rowcol = (rowcol[:,0], rowcol[:,1]);
 	return gr.EdgeV(rowcol, sc.tile(1,(N*4,)))


#	creates a breadth-first search tree of a Graph from a starting set of vertices
#	returns a 1D array with the parent vertex of each vertex in the tree; unreached vertices have parent == -Inf
#        and a 1D array with the level at which each vertex was first discovered (-2 if not in the tree)

def bfsTree(G, starts):
	parents = -2*sc.ones(G.spmat.shape[0]).astype(int);
	levels = np.copy(parents);
	newverts = np.copy(starts);
	parents[newverts] = -1;
	levels[newverts] = 0;
	fringe = np.array([newverts]);
	level = 1;
	while len(fringe) > 0:
		colvec = sc.zeros((G.nverts(),));
		# +1 to deal with 0 being a valid vertex ID
		colvec[fringe] = fringe+1; 
		cand = gr.Graph._SpMV_times_max(G.spmat, colvec)
		newverts = np.array(((cand.toarray().flatten() <> 0) & (parents == -2)).nonzero()).flatten();
		if len(newverts) > 0:
			parents[newverts] = cand[newverts].todense().astype(int) - 1;
			levels[newverts] = level;
		level += 1;
		fringe = newverts;
	parents = parents.astype(int);
	return (parents, levels);



#ToDo:  move bc() here from KDT.py