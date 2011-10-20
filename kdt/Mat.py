import math
from Graph import master
from Vec import Vec, DeVec, SpVec
import kdt.pyCombBLAS as pcb

import time

class Mat:
	Column  = pcb.pySpParMat.Column()
	Row = pcb.pySpParMat.Row()

	# NOTE:  for any vertex, out-edges are in the column and in-edges
	#	are in the row
	def __init__(self, sourceV=None, destV=None, valueV=None, nv=None, element=0):
		"""
		FIX:  doc
		creates a new Mat instance.  Can be called in one of the 
		following forms:

	Mat():  creates an empty Mat instance with elements.  Useful as input for genGraph500Edges.

	Mat(sourceV, destV, weightV, n)
	Mat(sourceV, destV, weightV, n, m)
		create a Mat Instance with edges with source represented by 
		each element of sourceV and destination represented by each 
		element of destV with weight represented by each element of 
		weightV.  In the 4-argument form, the resulting Mat will 
		have n out- and in-vertices.  In the 5-argument form, the 
		resulting Mat will have n out-vertices and m in-vertices.

		Input Arguments:
			sourceV:  a ParVec containing integers denoting the 
			    source vertex of each edge.
			destV:  a ParVec containing integers denoting the 
			    destination vertex of each edge.
			weightV:  a ParVec containing double-precision floating-
			    point numbers denoting the weight of each edge.
			n:  an integer scalar denoting the number of out-vertices 
			    (and also in-vertices in the 4-argument case).
			m:  an integer scalar denoting the number of in-vertices.

		Output Argument:  
			ret:  a Mat instance

		Note:  If two or more edges have the same source and destination
		vertices, their weights are summed in the output Mat instance.

		SEE ALSO:  toParVec
	def __init__(self, sourceV=None, destV=None, valueV=None, nv=None, element=0):
		"""
		if sourceV is None:
			if nv is not None: #create a Mat with an underlying pySpParMat* of the right size with no nonnulls
				nullVec = pcb.pyDenseParVec(0,0)
			if isinstance(element, (float, int, long)):
				if nv is None:
					self._m_ = pcb.pySpParMat()
				else:
					self._m_ = pcb.pySpParMat(nv,nv,nullVec,nullVec, nullVec)
			elif isinstance(element, bool):
				if nv is None:
					self._m_ = pcb.pySpParMatBool()
				else:
					self._m_ = pcb.pySpParMatBool(nv,nv,nullVec,nullVec, nullVec)
			elif isinstance(element, pcb.Obj1):
				if nv is None:
					self._m_ = pcb.pySpParMatObj1()
				else:
					self._m_ = pcb.pySpParMatObj1(nv,nv,nullVec,nullVec, nullVec)
			elif isinstance(element, pcb.Obj2):
				if nv is None:
					self._m_ = pcb.pySpParMatObj2()
				else:
					self._m_ = pcb.pySpParMatObj2(nv,nv,nullVec,nullVec, nullVec)
			self._identity_ = element
		elif sourceV is not None and destV is not None:
			i = sourceV
			j = destV
			v = valueV
			if type(v) == tuple and isinstance(element,(float,int,long)):
				raise NotImplementedError, 'tuple valueV only valid for Obj element'
			if len(i) != len(j):
				raise KeyError, 'source and destination vectors must be same length'
			if type(v) == int or type(v) == long or type(v) == float:
				raise NotImplementedError
				v = ParVec.broadcast(len(i),v)
#			if i.max() > nv-1:
#				raise KeyError, 'at least one first index greater than #vertices'
#			if j.max() > nv-1:
#				raise KeyError, 'at least one second index greater than #vertices'
			if isinstance(element, (float, int, long)):
				self._identity_ = 0
				self._m_ = pcb.pySpParMat(nv,nv,i._v_,j._v_,v._v_)
			elif isinstance(element, pcb.Obj1):
				self._identity_ = element
				self._identity_.weight = 0
				self._identity_.category = 0
				self._m_ = pcb.pySpParMatObj1(nv,nv,i._v_,j._v_,v._v_)
			elif isinstance(element, pcb.Obj2):
				self._identity_ = element
				self._identity_.weight = 0
				self._identity_.category = 0
				self._m_ = pcb.pySpParMatObj2(nv,nv,i._v_,j._v_,v._v_)
		elif len(args) == 5:
			raise NotImplementedError
			[i,j,v,nv1,nv2] = args
			if len(i) != len(j):
				raise KeyError, 'source and destination vectors must be same length'
			if type(v) == int or type(v) == long or type(v) == float:
				v = ParVec.broadcast(len(i),v)
			if i.max() > nv1-1:
				raise KeyError, 'at least one first index greater than #vertices'
			if j.max() > nv2-1:
				raise KeyError, 'at least one second index greater than #vertices'
			self._spm = pcb.pySpParMat(nv1,nv2,i._dpv,j._dpv,v._dpv)
		else:
			raise NotImplementedError, "only 1, 4, and 5 argument cases supported"

	# NEEDED: update to new fields
	# NEEDED: tests
	def __add__(self, other):
		"""
		adds corresponding edges of two Mat instances together,
		resulting in edges in the result only where an edge exists in at
		least one of the input Mat instances.
		"""
		if type(other) == int or type(other) == long or type(other) == float:
			raise NotImplementedError
		if self.nvert() != other.nvert():
			raise IndexError, 'Graphs must have equal numbers of vertices'
		elif isinstance(other, Mat):
			ret = self.copy()
			ret._spm += other._spm
			#ret._apply(pcb.plus(), other);  # only adds if both mats have nonnull elems!!
		return ret

	# NEEDED: update to new fields
	# NEEDED: tests
	def __div__(self, other):
		"""
		divides corresponding edges of two Mat instances together,
		resulting in edges in the result only where edges exist in both
		input Mat instances.
		"""
		if type(other) == int or type(other) == long or type(other) == float:
			ret = self.copy()
			ret._apply(pcb.bind2nd(pcb.divides(),other))
		elif self.nvert() != other.nvert():
			raise IndexError, 'Graphs must have equal numbers of vertices'
		elif isinstance(other,Mat):
			ret = self.copy()
			ret._apply(pcb.divides(), other)
		else:
			raise NotImplementedError
		return ret

	# NEEDED: update to new fields
	# NEEDED: tests
	def __getitem__(self, key):
		"""
		FIX:  fix documentation

		implements indexing on the right-hand side of an assignment.
		Usually accessed through the "[]" syntax.

		Input Arguments:
			self:  a Mat instance
			key:  one of the following forms:
			    - a non-tuple denoting the key for both dimensions
			    - a tuple of length 2, with the first element denoting
			        the key for the first dimension and the second 
			        element denoting for the second dimension.
			    Each key denotes the out-/in-vertices to be addressed,
			    and may be one of the following:
				- an integer scalar
				- the ":" slice denoting all vertices, represented
				  as slice(None,None,None)
				- a ParVec object containing a contiguous range
				  of monotonically increasing integers 
		
		Output Argument:
			ret:  a Mat instance, containing the indicated vertices
			    and their incident edges from the input Mat.

		SEE ALSO:  subgraph
		"""
		#ToDo:  accept slices for key0/key1 besides ParVecs
		if type(key)==tuple:
			if len(key)==1:
				[key0] = key; key1 = -1
			elif len(key)==2:
				[key0, key1] = key
			else:
				raise KeyError, 'Too many indices'
		else:
			key0 = key;  key1 = key
		if type(key0) == int or type(key0) == long or type(key0) == float:
			tmp = ParVec(1)
			tmp[0] = key0
			key0 = tmp
		if type(key1) == int or type(key0) == long or type(key0) == float:
			tmp = ParVec(1)
			tmp[0] = key1
			key1 = tmp
		#if type(key0)==slice and key0==slice(None,None,None):
		#	key0mn = 0; 
		#	key0tmp = self.nvert()
		#	if type(key0tmp) == tuple:
		#		key0mx = key0tmp[0] - 1
		#	else:
		#		key0mx = key0tmp - 1
		#if type(key1)==slice and key1==slice(None,None,None):
		#	key1mn = 0 
		#	key1tmp = self.nvert()
		#	if type(key1tmp) == tuple:
		#		key1mx = key1tmp[1] - 1
		#	else:
		#		key1mx = key1tmp - 1
		
		ret = Mat()
		ret._spm = self._spm.SubsRef(key0._dpv, key1._dpv)
		return ret

	# NEEDED: update to new fields
	# NEEDED: tests
	def __iadd__(self, other):
		if type(other) == int or type(other) == long or type(other) == float:
			raise NotImplementedError
		if self.nvert() != other.nvert():
			raise IndexError, 'Graphs must have equal numbers of vertices'
		elif isinstance(other, Mat):
			#self._apply(pcb.plus(), other)
			self._spm += other._spm
		return self

	# NEEDED: update to new fields
	# NEEDED: tests
	def __imul__(self, other):
		if type(other) == int or type(other) == long or type(other) == float:
			self._apply(pcb.bind2nd(pcb.multiplies(),other))
		elif isinstance(other,Mat):
			self._apply(pcb.multiplies(), other)
		else:
			raise NotImplementedError
		return self

	# NEEDED: tests
	def __mul__(self, other):
		"""
		multiplies corresponding edges of two Mat instances together,
		resulting in edges in the result only where edges exist in both
		input Mat instances.

		"""
		if type(other) == int or type(other) == long or type(other) == float:
			ret = self.copy()
			ret.apply(pcb.bind2nd(pcb.multiplies(),other))
		elif self.nvert() != other.nvert():
			raise IndexError, 'Graphs must have equal numbers of vertices'
		elif isinstance(other,Mat):
			ret = self.copy()
			ret.apply(pcb.multiplies(), other)
		else:
			raise NotImplementedError
		return ret

	# NEEDED: tests
	def __neg__(self):
		ret = self.copy()
		ret.apply(pcb.negate())
		return ret

	# NEEDED: tests
	#ToDo:  put in method to modify _REPR_MAX
	_REPR_MAX = 100
	def __repr__(self):
		if self.nvert() == 0:
			return 'Null Mat object'
		if self.nvert()==1:
			[i, j, v] = self.toVec()
			if len(v) > 0:
				return "%d %f" % (v[0], v[0])
			else:
				return "%d %f" % (0, 0.0)
		else:
			[i, j, v] = self.toVec()
			if len(i) < self._REPR_MAX:
				return "" + i + j + v
		return ' '

	# NEEDED: tests
	#in-place, so no return value
	def apply(self, op, other=None, notB=False):
		"""
		applies the given operator to every edge in the Mat

		Input Argument:
			self:  a Mat instance, modified in place.
			op:  a Python or pyCombBLAS function

		Output Argument:  
			None.

		"""
		if other is None:
			if not isinstance(op, pcb.UnaryFunction):
				self._m_.Apply(pcb.unary(op))
			else:
				self._m_.Apply(op)
			return
		else:
			if not isinstance(op, pcb.BinaryFunction):
				self._m_ = pcb.EWiseApply(self._m_, other._m_, pcb.binary(op), notB)
			else:
				self._m_ = pcb.EWiseApply(self._m_, other._m_, op, notB)
			return

	# NEEDED: tests
	def eWiseApply(self, other, op, allowANulls, allowBNulls, noWrap=False):
		"""
		ToDo:  write doc
		"""
		if hasattr(self, '_eFilter_') or hasattr(other, '_eFilter_'):
			class tmpB:
				if hasattr(self,'_eFilter_') and len(self._eFilter_) > 0:
					selfEFLen = len(self._eFilter_)
					eFilter1 = self._eFilter_
				else:
					selfEFLen = 0
				if hasattr(other,'_eFilter_') and len(other._eFilter_) > 0:
					otherEFLen = len(other._eFilter_)
					eFilter2 = other._eFilter_
				else:
					otherEFLen = 0
				@staticmethod
				def fn(x, y):
					for i in range(tmpB.selfEFLen):
						if not tmpB.eFilter1[i](x):
							x = type(self._identity_)()
							break
					for i in range(tmpB.otherEFLen):
						if not tmpB.eFilter2[i](y):
							y = type(other._identity_)()
							break
					return op(x, y)
			superOp = tmpB().fn
		else:
			superOp = op
		if noWrap:
			if isinstance(other, (float, int, long)):
				m = pcb.EWiseApply(self._m_, other   ,  superOp)
			else:
				m = pcb.EWiseApply(self._m_, other._m_, superOp)
		else:
			if isinstance(other, (float, int, long)):
				m = pcb.EWiseApply(self._m_, other   ,  pcb.binaryObj(superOp))
			else:
				m = pcb.EWiseApply(self._m_, other._m_, pcb.binaryObj(superOp))
		ret = self._toMat(m)
		return ret

	@staticmethod
	def _hasFilter(self):
		try:
			ret = (hasattr(self,'_eFilter_') and len(self._eFilter_)>0) # ToDo: or (hasattr(self,'vAttrib') and self.vAttrib._hasFilter(self.vAttrib)) 
		except AttributeError:
			ret = False
		return ret

	@staticmethod
	def isObj(self):
		return not isinstance(self._identity_, (float, int, long, bool))
		#try:
		#	ret = hasattr(self,'_elementIsObject') and self._elementIsObject
		#except AttributeError:
		#	ret = False
		#return ret

	#FIX:  put in a common place
	op_add = pcb.plus()
	op_sub = pcb.minus()
	op_mul = pcb.multiplies()
	op_div = pcb.divides()
	op_mod = pcb.modulus()
	op_fmod = pcb.fmod()
	op_pow = pcb.pow()
	op_max  = pcb.max()
	op_min = pcb.min()
	op_bitAnd = pcb.bitwise_and()
	op_bitOr = pcb.bitwise_or()
	op_bitXor = pcb.bitwise_xor()
	op_and = pcb.logical_and()
	op_or = pcb.logical_or()
	op_xor = pcb.logical_xor()
	op_eq = pcb.equal_to()
	op_ne = pcb.not_equal_to()
	op_gt = pcb.greater()
	op_lt = pcb.less()
	op_ge = pcb.greater_equal()
	op_le = pcb.less_equal()

	# NEEDED: update to new fields
	# NEEDED: tests
	def reduce(self, dir, op, pred=None, noWrap=False):
		"""
		ToDo:  write doc
		NOTE:  need to doc clearly that the 2nd arg to the reduction
		fn is the sum;  the first is the current addend and the second
		is the running sum
		"""
		if dir != Mat.Row and dir != Mat.Column:
			raise KeyError, 'unknown direction'
		if hasattr(self, '_eFilter_') and len(self._eFilter_) > 0:
			class tmpB:
				_eFilter_ = self._eFilter_
				@staticmethod
				def fn(x, y):
					for i in range(len(tmpB._eFilter_)):
						if not tmpB._eFilter_[i](x):
							#x = type(self._identity_)()
							return y # no contribution; return existing 'sum'
							#break
					return op(x, y)
			tmpInstance = tmpB()
			superOp = pcb.binaryObj(tmpInstance.fn)
			#self._v_.Apply(pcb.unaryObj(tmpInstance.fn))
		else:
			superOp = op
		if pred is not None:
			if not isinstance(pred, pcb.UnaryFunction):
				if isinstance(self._identity_, (float, int, long)):
					realPred = pcb.binary(pred)
				else:
					realPred = pcb.binaryObj(pred)
			else:
				realPred = pred
		if pred is None:
			tmp = self._m_.Reduce(dir, superOp)
		else:
			tmp = self._m_.Reduce(dir, superOp, realPred)
		ret = Vec._toVec(Vec(element=self._identity_),tmp)
		return ret

	# possibly in-place;  if so, no return value
	def SpMV(self, other, semiRing=None, noWrap=False, inPlace=False):
		"""
		FIX:  add doc
		inPlace -> no return value
		"""
		#FIX:  is noWrap arg needed?
		#ToDo:  is code for if/else cases actually different?
		if isinstance(self._identity_, (float, int, long, bool)) and isinstance(other._identity_, (float, int, long)):
			if isinstance(self._identity_, bool):
				#HACK OF HACKS!
				self._m_.SpMV_SelMax_inplace(other._v_)
				return
			if semiRing is None:
				tSR = pcb.TimesPlusSemiring()
			else:  
				tSR = semiRing
			if not inPlace:
				ret = Vec()
				ret._v_ = self._m_.SpMV(other._v_, tSR)
				return ret
			else:
				self._m_.SpMV_inplace(other._v_, tSR)
				return
		else:
			if semiRing is None:
				tSR = pcb.TimesPlusSemiring()
			else:
				tSR = semiRing
			if not inPlace:
				ret = Vec()
				ret._v_ = self._m_.SpMV(other._v_, tSR)
				return ret
			else:
				self._m_.SpMV_inplace(other._v_, tSR)
				return
	spMV = SpMV

	# NEEDED: update to new fields
	# NEEDED: tests
	def SpGEMM(self, other):
		"""
		"multiplies" two Mat instances together as though each was
		represented by a sparse matrix, with rows representing out-edges
		and columns representing in-edges.
		"""
		selfnv = self.nvert()
		if type(selfnv) == tuple:
			[selfnv1, selfnv2] = selfnv
		else:
			selfnv1 = selfnv; selfnv2 = selfnv
		othernv = other.nvert()
		if type(othernv) == tuple:
			[othernv1, othernv2] = othernv
		else:
			othernv1 = othernv; othernv2 = othernv
		if selfnv2 != othernv1:
			raise ValueError, '#in-vertices of first graph not equal to #out-vertices of the second graph '
		ret = Mat()
		ret._spm = self._spm.SpGEMM(other._spm)
		return ret
	spGEMM = SpGEMM

	# in-place, so no return value
	def addEFilter(self, filter):
		"""
		adds a vertex filter to the Mat instance.  

		A vertex filter is a Python function that is applied elementally
		to each vertex in the Mat, with a Boolean True return value
		causing the vertex to be considered and a False return value
		causing it not to be considered.

		Vertex filters are additive, in that each vertex must pass all
		filters to be considered.  All vertex filters are executed before
		a vertex is considered in a computation.
#FIX:  how is an argument passed to the function?

		Input Arguments:
			self:  a Mat instance
			filter:  a Python function

		SEE ALSO:
			delEFilter  
		"""
		if hasattr(self, '_eFilter_'):
			self._eFilter_.append(filter)
		else:
			self._eFilter_ = [filter]
		return
		
	@staticmethod
	def load(fname):
		"""
		loads the contents of the file named fname (in the Coordinate Format 
		of the Matrix Market Exchange Format) into a Mat instance.

		Input Argument:
			fname:  a filename from which the matrix data will be loaded.
		Output Argument:
			ret:  a Mat instance containing the graph represented
			    by the file's contents.

		NOTE:  The Matrix Market format numbers vertex numbers from 1 to
		N.  Python and KDT number vertex numbers from 0 to N-1.  The load
		method makes this conversion while reading the data and creating
		the graph.

		SEE ALSO:  save, UFget
		"""
		# Verify file exists.
		file = open(fname, 'r')
		file.close()
		
		#FIX:  crashes if any out-of-bound indices in file; easy to
		#      fall into with file being 1-based and Py being 0-based
		ret = Mat()
		ret._m_ = pcb.pySpParMat()
		ret._m_.load(fname)
		return ret

	def save(self, fname):
		"""
		saves the contents of the passed DiGraph instance to a file named
		fname in the Coordinate Format of the Matrix Market Exchange Format.

		Input Arguments:
			self:  a DiGraph instance
			fname:  a filename to which the DiGraph data will be saved.

		NOTE:  The Matrix Market format numbers vertex numbers from 1 to
		N.  Python and KDT number vertex numbers from 0 to N-1.  The save
		method makes this conversion while writing the data.

		SEE ALSO:  load, UFget
		"""
		self._m_.save(fname)
		return