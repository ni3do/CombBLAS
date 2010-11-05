
%module pyCombBLAS

%typemap(in) int64_t = long long;
%typemap(out) int64_t = long long;

// This block gets copied verbatim into the header area of the generated wrapper. DiGraph has to
// be defined here somehow. Prefereably we'd #include "DiGraph.h", but that brings in templates which
// cause duplicate definition linker errors. Unless that gets resolved, we just redefine DiGraph
// omitting the templated protected members.

%{
#define SWIG_FILE_WITH_INIT

#include "pySpParMat.h"
#include "pySpParVec.h"
#include "pyDenseParVec.h"
%}


// This block gets called when the module is loaded. It is wrapped in extern "C".
%init %{
init_pyCombBLAS_MPI();
%}

// It's possible to have the generated python code also include some custom code.
// This may be a good place to add an atexit() to call mpi finalize.
%pragma(python) code="import atexit"
%pragma(python) code="atexit.register(DiGraph.finalize())"


// wrapped classes

class pySpParMat {
public:
	pySpParMat();

public:
	int64_t getnnz();
	int64_t getnrow();
	int64_t getncol();
	
public:	
	void load(const char* filename);
	void GenGraph500Edges(int scale);
	
public:
	pySpParVec* FindIndsOfColsWithSumGreaterThan(int64_t gt);
	//pyDenseParVec* Reduce_ColumnSums();
	
public:
	pySpParVec* SpMV_SelMax(const pySpParVec& v);
};


class pySpParVec {
public:
	pySpParVec();
	//pySpParVec(const pySpParMat& commSource);

public:
	int64_t getnnz() const;

	void add(const pySpParVec& other);
	//void subtract(const pySpParVec& other);
	pySpParVec* copy();

	void SetElement(int64_t index, int64_t numx);	// element-wise assignment
	
public:	
	void invert(); // "~";  almost equal to logical_not
	void abs();
	
	bool anyNonzeros() const;
	bool allNonzeros() const;
	
	int64_t intersectSize(const pySpParVec& other);

	
public:	
	void load(const char* filename);

public:
	static pySpParVec* zeros(int64_t howmany);
	static pySpParVec* range(int64_t howmany, int64_t start);
	
	
};

//pySpParVec* EWiseMult(const pySpParVec& a, const pySpParVec& b, bool exclude);
pySpParVec* EWiseMult(const pySpParVec& a, const pyDenseParVec& b, bool exclude, int64_t zero);

class pyDenseParVec {
public:
	pyDenseParVec();
	pyDenseParVec(int64_t size, int64_t id);
	//pyDenseParVec(const pySpParMat& commSource, int64_t zero);
	
public:
	int length() const;
	
	void add(const pyDenseParVec& other);
	void add(const pySpParVec& other);
	pyDenseParVec& operator+=(const pyDenseParVec & rhs);
	pyDenseParVec& operator-=(const pyDenseParVec & rhs);
	pyDenseParVec& operator=(const pyDenseParVec & rhs);
	pyDenseParVec* copy();
	
public:
	void invert(); // "~";  almost equal to logical_not
	void abs();
	
	//bool anyNonzeros() const;
	//bool allNonzeros() const;

	
public:	
	void load(const char* filename);
	
public:
	pySpParVec* FindInds_GreaterThan(int64_t value);
};



//void init();
void finalize();
