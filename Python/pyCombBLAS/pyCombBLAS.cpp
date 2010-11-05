#include "pyCombBLAS.h"

////////////////// PREDICATES
int64_t invert64(int64_t v)
{
	return ~v;
}

int64_t abs64(int64_t v)
{
	if (v < 0) return -v;
	return v;
}

int64_t negate64(int64_t v)
{
	return -v;
}

bool nonzero64(int64_t v)
{
	return v != 0;
}

bool zero64(int64_t v)
{
	return v == 0;
}

bool eq64(int64_t test, int64_t val)
{
	return test == val;
}
bool neq64(int64_t test, int64_t val)
{
	return test != val;
}


////////////////// OPERATORS

pySpParVec* EWiseMult(const pySpParVec& a, const pySpParVec& b, bool exclude)
{
	pySpParVec* ret = new pySpParVec(0);
	//ret->v = ::EWiseMult(a.v, b.v, exclude);
	return ret;
}

pySpParVec* EWiseMult(const pySpParVec& a, const pyDenseParVec& b, bool exclude, int64_t zero)
{
	pySpParVec* ret = new pySpParVec(0);
	ret->v = EWiseMult(a.v, b.v, exclude, (int64_t)0);
	return ret;
}


////////////////////////// INITALIZATION/FINALIZE

void init_pyCombBLAS_MPI()
{
	cout << "calling MPI::Init" << endl;
	MPI::Init();
	/*
	int nprocs = MPI::COMM_WORLD.Get_size();
	int myrank = MPI::COMM_WORLD.Get_rank();
	MPI::COMM_WORLD.Barrier();
	
	int sum = 0;
	int one = 1;
	MPI_Reduce(&one, &sum, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD); 

	cout << "I am proc " << myrank << " out of " << nprocs << ". Hear me roar!" << endl;
	if (myrank == 0) {
		cout << "We all reduced our ones to get " << sum;
		if (sum == nprocs)
			cout << ". Success! MPI works." << endl;
		else
			cout << ". SHOULD GET #PROCS! MPI is broken!" << endl;
	}
	*/
}

void finalize()
{
	cout << "calling MPI::Finalize" << endl;
	MPI::Finalize();
}

bool root()
{
	return MPI::COMM_WORLD.Get_rank() == 0;
}