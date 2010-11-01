import numpy as np
import scipy as sc
import scipy.sparse as sp
import SpParVec as SPV
import SpParMat as SPM

class Graph():
	#ToDo: privatize .spmat name (to .__spmat)
	#ToDo: implement bool, not, _sprand

	print "in Graph"

	def __init__(self, edgev, size):
		# include edges in both directions
		# old self.spmat = sp.csr_matrix((edgev[1],edgev[0]), shape=(size)) + sp.csr_matrix(((edgev[1][1],edgev[1][0]), edgev[0]), shape=(size));

		# NEW:  not clear that any of the SpParMat constructors actually do the following, so
		#   may be a new function
		self.spmat = SPM.SpParMat(edgev, size=size);

	def __len__(self):
		return self.spmat.nverts();

	# NOTE:  no shape() for a graph;  use nverts/nedges instead	
	#def shape(self):
	#	return (self.spmat.nverts(), self.spmat.nverts());

	#FIX:  should only return 1 of the 2 directed edges for simple graphs
	def toEdgeV(self):		
		[ij, v] = self._toArrays(self.spmat);
		return EdgeV(ij, v);

	@staticmethod
	def _toArrays(spmat):		# similar to toEdgeV, except returns arrays
		[i, j] = spmat.nonzero();		# NEW:  don't see a way to do this with current SpParMat
		v = spmat[i,j];				# not NEW:  looks like already supported by general indexing
		return ((i,j), v)

	def nedges(self):
		return self.spmat.getnnz();

	def nverts(self):
		return self.spmat.getnrow();

	def degree(self):
		tmp = SPM.reduce(self._spones(self.spmat), 0, +);	# FIX: syntax
		return tmp;

	@staticmethod
	def _spones(spmat):		
		[nr, nc] = spmat.shape();
		[ij, ign] = Graph._toEdgeV(spmat);
		return Graph.Graph(ij, SPV.ones(len(ign)));

	@staticmethod
	def _sub2ind(size, row, col):		# ToDo:  extend to >2D
		(nr, nc) = size;
		ndx = row + col*nr;
		return ndx

	@staticmethod
	def _SpMV_times_max(X,Y):		# ToDo:  extend to 2-/3-arg versions 
		[nrX, ncX] = X.shape()
		[nrY, ncY] = Y.shape()
		if ncX != nrY:
			print "Inner dimensions of X and Y do not match"
 			return
		if ncY > 1:
			print "Y must be a column vector"
			return
		#ToDo:  make this work with a truly sparse Z
		#old Z = sp.csr_matrix(-np.Inf * np.ones((nrX, ),np.dtype(X))).T;
		#old for i in range(nrX):
 		#old	for k in range(ncX):
 		#old		Z[i,0] = max( (Z[i])[0,0], X[i,k]*Y[k,] );
 		#old		#was:  Z[i,0] = min( Z[i], X[i,k]*Y[k,] );
		#old	foo = Z[i];	#debug

		Z = SPM.SpMV_SelMax(X, Y);	# assuming function creates its own output array

		return Z
		

class EdgeV():
	# NOTE:  implemented in terms of CBLAS' sparse vectors
	# NOTE:  vertex numbers go from 1 to N unlike Python's 0 to N-1 


	# For now, vertex endpoints are each a SpParVec, as is values
	# ToDo:  create a way for a client edge-vector to become a distributed edge-vector
	def __init__(self, verts, values):
		error = False;
		if type(values).__name__ <> 'SpParVec':
			error = True;
		for i in range(len(verts)):		# 'len' overloaded for SPVs
			if type(verts[i]).__name__ <> 'SpParVec':
				error = True;
		if error:
			raise ValueError('inputs must be SpParVecs')
		if len(verts[0]) <> len(values):
			raise ValueError('length of vertex and values vectors must be the same')
		self.__verts = verts;
		self.__values = values;


	#FIX: inconsistency between len and getitem;  len returns #verts;  getitem[0] returns verts
	def __len__(self):
		return len(self.__verts[0]);

	def __getitem__(self, i):
		if i == 0:
			return self.__verts;
		if i == 1:
			return self.__values;
		raise ValueError('index out of range');
		return;

	def vertsShape(self):
		return np.shape(self.__verts);

	def valuesShape(self):
		return np.shape(self.__values);

	def verts(self):
		return self.__verts;

	def values(self):
		return self.__values;
		

#	No VertexV class for now

#class VertexV():
#	print "in VertexV"
#
#	def __init__(self, ndces):