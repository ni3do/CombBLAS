
#ifdef THREADED
#ifndef _OPENMP
#define _OPENMP
#endif

#include <omp.h>
int cblas_splits = 1;
#endif

#include "../CombBLAS.h"
#include <mpi.h>
#include <sys/time.h>
#include <iostream>
#include <functional>
#include <algorithm>
#include <vector>
#include <string>
#include <sstream>


#include "BPMaximalMatching.h"
#include "BPMaximumMatching.h"

using namespace std;

// algorithmic options
bool prune, mvInvertMate, randMM, moreSplit;
int init;
bool randMaximal;
bool fewexp;


typedef SpParMat < int64_t, bool, SpDCCols<int64_t,bool> > Par_DCSC_Bool;
typedef SpParMat < int64_t, int64_t, SpDCCols<int64_t, int64_t> > Par_DCSC_int64_t;
typedef SpParMat < int64_t, double, SpDCCols<int64_t, double> > Par_DCSC_Double;
typedef SpParMat < int64_t, bool, SpCCols<int64_t,bool> > Par_CSC_Bool;




template <typename PARMAT>
void Symmetricize(PARMAT & A)
{
    // boolean addition is practically a "logical or"
    // therefore this doesn't destruct any links
    PARMAT AT = A;
    AT.Transpose();
    A += AT;
}



/*
 Remove isolated vertices and purmute
 */
void removeIsolated(Par_DCSC_Bool & A)
{
    
    int nprocs, myrank;
    MPI_Comm_size(MPI_COMM_WORLD,&nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD,&myrank);
    
    
    FullyDistVec<int64_t, int64_t> * ColSums = new FullyDistVec<int64_t, int64_t>(A.getcommgrid());
    FullyDistVec<int64_t, int64_t> * RowSums = new FullyDistVec<int64_t, int64_t>(A.getcommgrid());
    FullyDistVec<int64_t, int64_t> nonisoRowV;	// id's of non-isolated (connected) Row vertices
    FullyDistVec<int64_t, int64_t> nonisoColV;	// id's of non-isolated (connected) Col vertices
    FullyDistVec<int64_t, int64_t> nonisov;	// id's of non-isolated (connected) vertices
    
    A.Reduce(*ColSums, Column, plus<int64_t>(), static_cast<int64_t>(0));
    A.Reduce(*RowSums, Row, plus<int64_t>(), static_cast<int64_t>(0));
    
    // this steps for general graph
    /*
     ColSums->EWiseApply(*RowSums, plus<int64_t>()); not needed for bipartite graph
     nonisov = ColSums->FindInds(bind2nd(greater<int64_t>(), 0));
     nonisov.RandPerm();	// so that A(v,v) is load-balanced (both memory and time wise)
     A.operator()(nonisov, nonisov, true);	// in-place permute to save memory
     */
    
    // this steps for bipartite graph
    nonisoColV = ColSums->FindInds(bind2nd(greater<int64_t>(), 0));
    nonisoRowV = RowSums->FindInds(bind2nd(greater<int64_t>(), 0));
    delete ColSums;
    delete RowSums;
    
    
    {
        nonisoColV.RandPerm();
        nonisoRowV.RandPerm();
    }
    
    
    int64_t nrows1=A.getnrow(), ncols1=A.getncol(), nnz1 = A.getnnz();
    double avgDeg1 = (double) nnz1/(nrows1+ncols1);
    
    
    A.operator()(nonisoRowV, nonisoColV, true);
    
    int64_t nrows2=A.getnrow(), ncols2=A.getncol(), nnz2 = A.getnnz();
    double avgDeg2 = (double) nnz2/(nrows2+ncols2);
    
    
    if(myrank == 0)
    {
        cout << "ncol nrows  nedges deg \n";
        cout << nrows1 << " " << ncols1 << " " << nnz1 << " " << avgDeg1 << " \n";
        cout << nrows2 << " " << ncols2 << " " << nnz2 << " " << avgDeg2 << " \n";
    }
    
    MPI_Barrier(MPI_COMM_WORLD);
    
    
}


void ShowUsage()
{
    int myrank;
    MPI_Comm_rank(MPI_COMM_WORLD,&myrank);
    if(myrank == 0)
    {
        cout << "\n-------------- usage --------------\n";
        cout << "Usage (random matrix): ./bpmm <er|g500|ssca> <Scale> <EDGEFACTOR> <init><diropt><prune><graft>\n";
        cout << "Usage (input matrix): ./bpmm <input> <matrix> <init><diropt><prune><graft>\n\n";
        
        cout << " \n-------------- meaning of arguments ----------\n";
        cout << "** er: Erdos-Renyi, g500: Graph500 benchmark, ssca: SSCA benchmark\n";
        cout << "** scale: matrix dimention is 2^scale\n";
        cout << "** edgefactor: average degree of vertices\n";
        cout << "** (optional) init : maximal matching algorithm used to initialize\n ";
        cout << "      none: noinit, greedy: greedy init , ks: Karp-Sipser, dmd: dynamic mindegree\n";
        cout << "       default: none\n";
        cout << "** (optional) randMaximal: random parent selection in greedy/Karp-Sipser\n" ;
        //cout << "** (optional) diropt: employ direction-optimized BFS\n" ;
        cout << "** (optional) prune: discard trees as soon as an augmenting path is found\n" ;
        //cout << "** (optional) graft: employ tree grafting\n" ;
        cout << "** (optional) mvInvertMate: Invert based on SpMV as opposted to All2All.\n" ;
        cout << "** (optional) moreSplit: more splitting of Matrix.\n" ;
        cout << "(order of optional arguments does not matter)\n";
        
        cout << " \n-------------- examples ----------\n";
        cout << "Example: mpirun -np 4 ./bpmm g500 18 16" << endl;
        cout << "Example: mpirun -np 4 ./bpmm g500 18 16 ks diropt graft" << endl;
        cout << "Example: mpirun -np 4 ./bpmm input cage12.mtx ks diropt graft\n" << endl;
    }
}

template <class IT, class NT>
vector<tuple<IT,IT,NT>> ExchangeData(vector<vector<tuple<IT,IT,NT>>> & tempTuples, MPI_Comm World)
{
    
    /* Create/allocate variables for vector assignment */
    MPI_Datatype MPI_tuple;
    MPI_Type_contiguous(sizeof(tuple<IT,IT,NT>), MPI_CHAR, &MPI_tuple);
    MPI_Type_commit(&MPI_tuple);
    
    int nprocs;
    MPI_Comm_size(World, &nprocs);
    
    int * sendcnt = new int[nprocs];
    int * recvcnt = new int[nprocs];
    int * sdispls = new int[nprocs]();
    int * rdispls = new int[nprocs]();
    
    // Set the newly found vector entries
    IT totsend = 0;
    for(IT i=0; i<nprocs; ++i)
    {
        sendcnt[i] = tempTuples[i].size();
        totsend += tempTuples[i].size();
    }
    
    MPI_Alltoall(sendcnt, 1, MPI_INT, recvcnt, 1, MPI_INT, World);
    
    partial_sum(sendcnt, sendcnt+nprocs-1, sdispls+1);
    partial_sum(recvcnt, recvcnt+nprocs-1, rdispls+1);
    IT totrecv = accumulate(recvcnt,recvcnt+nprocs, static_cast<IT>(0));
    
    vector< tuple<IT,IT,NT> > sendTuples(totsend);
    for(int i=0; i<nprocs; ++i)
    {
        copy(tempTuples[i].begin(), tempTuples[i].end(), sendTuples.data()+sdispls[i]);
        vector< tuple<IT,IT,NT> >().swap(tempTuples[i]);	// clear memory
    }
    vector< tuple<IT,IT,NT> > recvTuples(totrecv);
    MPI_Alltoallv(sendTuples.data(), sendcnt, sdispls, MPI_tuple, recvTuples.data(), recvcnt, rdispls, MPI_tuple, World);
    DeleteAll(sendcnt, recvcnt, sdispls, rdispls); // free all memory
    MPI_Type_free(&MPI_tuple);
    return recvTuples;
     
}



template <class IT, class NT>
vector<tuple<IT,IT,NT>> ExchangeData1(vector<vector<tuple<IT,IT,IT,NT>>> & tempTuples, MPI_Comm World)
{
    
    /* Create/allocate variables for vector assignment */
    MPI_Datatype MPI_tuple;
    MPI_Type_contiguous(sizeof(tuple<IT,IT,IT,NT>), MPI_CHAR, &MPI_tuple);
    MPI_Type_commit(&MPI_tuple);
    
    int nprocs;
    MPI_Comm_size(World, &nprocs);
    
    int * sendcnt = new int[nprocs];
    int * recvcnt = new int[nprocs];
    int * sdispls = new int[nprocs]();
    int * rdispls = new int[nprocs]();
    
    // Set the newly found vector entries
    IT totsend = 0;
    for(IT i=0; i<nprocs; ++i)
    {
        sendcnt[i] = tempTuples[i].size();
        totsend += tempTuples[i].size();
    }
    
    MPI_Alltoall(sendcnt, 1, MPI_INT, recvcnt, 1, MPI_INT, World);
    
    partial_sum(sendcnt, sendcnt+nprocs-1, sdispls+1);
    partial_sum(recvcnt, recvcnt+nprocs-1, rdispls+1);
    IT totrecv = accumulate(recvcnt,recvcnt+nprocs, static_cast<IT>(0));
    
    vector< tuple<IT,IT,IT,NT> > sendTuples(totsend);
    for(int i=0; i<nprocs; ++i)
    {
        copy(tempTuples[i].begin(), tempTuples[i].end(), sendTuples.data()+sdispls[i]);
        vector< tuple<IT,IT,IT,NT> >().swap(tempTuples[i]);	// clear memory
    }
    vector< tuple<IT,IT,IT,NT> > recvTuples(totrecv);
    MPI_Alltoallv(sendTuples.data(), sendcnt, sdispls, MPI_tuple, recvTuples.data(), recvcnt, rdispls, MPI_tuple, World);
    DeleteAll(sendcnt, recvcnt, sdispls, rdispls); // free all memory
    MPI_Type_free(&MPI_tuple);
    return recvTuples;
    
}


template <class IT, class NT,class DER>
int OwnerProcs(SpParMat < IT, NT, DER > & A, IT grow, IT gcol)
{
     auto commGrid = A.getcommgrid();
    int procrows = commGrid->GetGridRows();
    int proccols = commGrid->GetGridCols();
    IT m_perproc = A.getnrow() / procrows;
    IT n_perproc = A.getncol() / proccols;
    

    int pr, pc;
    if(m_perproc != 0)
        pr = std::min(static_cast<int>(grow / m_perproc), procrows-1);
    else	// all owned by the last processor row
        pr = procrows -1;

    if(n_perproc != 0)
        pc = std::min(static_cast<int>(gcol / n_perproc), proccols-1);
    else
        pc = proccols-1;
    
    return commGrid->GetRank(pr, pc);
}


template <class IT, class NT, class DER>
void TwoThirdApprox(SpParMat < IT, NT, DER > & A, FullyDistVec<IT, IT>& mateRow2Col, FullyDistVec<IT, IT>& mateCol2Row)
{

    // some information about CommGrid and matrix layout
    auto commGrid = A.getcommgrid();
    MPI_Comm World = commGrid->GetWorld();
    MPI_Comm ColWorld = commGrid->GetColWorld();
    MPI_Comm RowWorld = commGrid->GetRowWorld();
    IT m_perproc = A.getnrow() / commGrid->GetGridRows();
    IT n_perproc = A.getncol() / commGrid->GetGridCols();
    IT moffset = commGrid->GetRankInProcCol() * m_perproc;
    IT noffset = commGrid->GetRankInProcRow() * n_perproc;
    int nprocs;
    MPI_Comm_size(World, &nprocs);


 
    // -----------------------------------------------------------
    // replicate mate vectors for mateCol2Row
    // Communication cost: same as the first communication of SpMV
    // -----------------------------------------------------------
    int xsize = (int)  mateCol2Row.LocArrSize();
    int trxsize = 0;
    int diagneigh = commGrid->GetComplementRank();
    MPI_Status status;
    MPI_Sendrecv(&xsize, 1, MPI_INT, diagneigh, TRX, &trxsize, 1, MPI_INT, diagneigh, TRX, World, &status);
    vector<IT> trxnums(trxsize);
    MPI_Sendrecv(mateCol2Row.GetLocArr(), xsize, MPIType<IT>(), diagneigh, TRX, trxnums.data(), trxsize, MPIType<IT>(), diagneigh, TRX, World, &status);
    
    int colneighs, colrank;
    MPI_Comm_size(ColWorld, &colneighs);
    MPI_Comm_rank(ColWorld, &colrank);
    vector<int> colsize(colneighs);
    colsize[colrank] = trxsize;
    MPI_Allgather(MPI_IN_PLACE, 1, MPI_INT, colsize.data(), 1, MPI_INT, ColWorld);
    vector<int> dpls(colneighs,0);	// displacements (zero initialized pid)
    std::partial_sum(colsize.data(), colsize.data()+colneighs-1, dpls.data()+1);
    int accsize = std::accumulate(colsize.data(), colsize.data()+colneighs, 0);
    vector<IT> RepMateC2R(accsize);
    MPI_Allgatherv(trxnums.data(), trxsize, MPIType<IT>(), RepMateC2R.data(), colsize.data(), dpls.data(), MPIType<IT>(), ColWorld);
    // -----------------------------------------------------------
    
    

    // -----------------------------------------------------------
    // replicate mate vectors for mateRow2Col
    // Communication cost: same as the first communication of SpMV
    //                      (minus the cost of tranposing vector)
    // -----------------------------------------------------------
    
    int rowneighs, rowrank;
    xsize = (int)  mateRow2Col.LocArrSize();
    MPI_Comm_size(RowWorld, &rowneighs);
    MPI_Comm_rank(RowWorld, &rowrank);
    vector<int> rowsize(rowneighs);
    rowsize[rowrank] = xsize;
    MPI_Allgather(MPI_IN_PLACE, 1, MPI_INT, rowsize.data(), 1, MPI_INT, RowWorld);
    vector<int> rdpls(rowneighs,0);	// displacements (zero initialized pid)
    std::partial_sum(rowsize.data(), rowsize.data()+rowneighs-1, rdpls.data()+1);
    accsize = std::accumulate(rowsize.data(), rowsize.data()+rowneighs, 0);
    vector<IT> RepMateR2C(accsize);
    MPI_Allgatherv(mateRow2Col.GetLocArr(), xsize, MPIType<IT>(), RepMateR2C.data(), rowsize.data(), rdpls.data(), MPIType<IT>(), RowWorld);
    
    vector<NT> RepMateR2C_val(accsize);
    // -----------------------------------------------------------

    
    
    
    // C requests
    // each row is for a processor where C requests will be sent to
    vector<vector<tuple<IT,IT,NT>>> tempTuples (nprocs);
    
    DER* spSeq = A.seqptr(); // local part of the matrix
    for(auto colit = spSeq->begcol(); colit != spSeq->endcol(); ++colit) // iterate over columns
    {
        IT lj = colit.colid(); // local numbering
        IT j = lj + noffset;
        IT mj = RepMateC2R[lj]; // mate of j
        //start nzit from mate colid;
        for(auto nzit = spSeq->begnz(colit); nzit < spSeq->endnz(colit); ++nzit)
        {
            IT li = nzit.rowid();
            IT i = li + moffset;
            IT mi = RepMateR2C[li];
            // TODO: use binary search to directly start from RepMateC2R[colid]
            if( i > mj)
            {
                //w = nzit.value()-W[i,M[i]]-W[M'[j],j];
                //TODO: fix me *********************
                double w=1; // testing now
                int owner = OwnerProcs(A, mj, mi); // think about the symmetry??
                tempTuples[owner].push_back(make_tuple(mj, mi, w));
            }
        }
    }
    
    //exchange C-request via All2All
    // there might be some empty mesages in all2all
    vector<tuple<IT,IT,NT>> recvTuples = ExchangeData(tempTuples, World);
    //tempTuples are cleared in ExchangeData function
    
    
    vector<vector<tuple<IT,IT, IT, NT>>> tempTuples1 (nprocs);
    // at the owner of (mj,mi)
    for(int k=0; k<recvTuples.size(); ++k)
    {
        IT mj = get<0>(recvTuples[k]) ;
        IT mi = get<1>(recvTuples[k]) ;
        IT i = RepMateR2C[mi - moffset];
        NT weight = get<2>(recvTuples[k]);
        DER temp = (*spSeq)(mj - moffset, mi - noffset);
        if(!temp.isZero()) // this entry exists
        {
            //TODO: fix this
            NT cw = weight + 1; //w+W[M'[j],M[i]];
            if (cw > 0)
            {
                IT j = RepMateR2C[mj - moffset];
                int owner = OwnerProcs(A,  mj, j); // (mj,j)
                tempTuples1[owner].push_back(make_tuple(mj, mi, i, cw)); // @@@@@ send i as well
                //tempTuples[owner].push_back(make_tuple(mj, j, cw));
            }
        }
    }
    
    vector< tuple<IT,IT,NT> >().swap(recvTuples);
    
    //exchange RC-requests via AllToAllv
    vector<tuple<IT,IT,IT,NT>> recvTuples1 = ExchangeData1(tempTuples1, World);
    
    
    // at the owner of (mj,j)
    
    vector<tuple<IT,IT,IT,NT>> bestTuples (spSeq->getncol());
    for(int k=0; k<spSeq->getncol(); ++k)
    {
        bestTuples[k] = make_tuple(-1,-1,-1,0); // fix this
    }
    
    for(int k=0; k<recvTuples1.size(); ++k)
    {
        IT mj = get<0>(recvTuples1[k]) ;
        IT mi = get<1>(recvTuples1[k]) ;
        IT i = get<2>(recvTuples1[k]) ;
        NT weight = get<3>(recvTuples1[k]);
        IT j = RepMateR2C[mj - moffset];
        IT lj = j - noffset;
        // how can I get i from here ?? ***** // receive i as well
        
        // we can get rid of the first check if edge weights are non negative
        if( (get<0>(bestTuples[lj]) == -1)  || (weight > get<3>(bestTuples[lj])) )
        {
            bestTuples[lj] = make_tuple(i,mi,j,weight);
        }
    }
    
    for(int k=0; k<spSeq->getncol(); ++k)
    {
        if( get<0>(bestTuples[k]) != -1)
        {
            //IT j = RepMateR2C[mj - moffset]; /// fix me
            int owner = OwnerProcs(A,  get<0>(bestTuples[k]), get<1>(bestTuples[k])); // (i,mi)
            tempTuples[owner].push_back(bestTuples[k]);
        }
    }

    vector< tuple<IT,IT,IT, NT> >().swap(recvTuples1);
    recvTuples1 = ExchangeData1(tempTuples1, World);
    
    vector<tuple<IT,IT,IT,IT, NT>> bestTuples1 (spSeq->getnrow());
    
    // Phase 4
    for(int k=0; k<spSeq->getnrow(); ++k)
    {
        bestTuples1[k] = make_tuple(-1,-1,-1,-1,0);
    }
    
    for(int k=0; k<recvTuples1.size(); ++k)
    {
        IT i = get<0>(recvTuples1[k]) ;
        IT mi = get<1>(recvTuples1[k]) ;
        IT j = get<2>(recvTuples1[k]) ;
        NT weight = get<3>(recvTuples1[k]);
        IT mj = RepMateC2R[j - moffset];
        
        IT lmj = mj - noffset;
        
        // we can get rid of the first check if edge weights are non negative
        if( (get<0>(bestTuples1[lmj]) == -1)  || (weight > get<4>(bestTuples1[lj])) )
        {
            bestTuples1[lmj] = make_tuple(i,j,mi,mj,weight);
        }
    }
    
    
    vector<vector<tuple<IT,IT,IT, IT>>> winnerTuples (nprocs);
    vector<tuple<IT,IT>> col2rowBcastTuples; //(mi,mj)
    vector<tuple<IT,IT>> row2colBcastTuples; //(i,j)

    
    for(int k=0; k<spSeq->getnrow(); ++k)
    {
        if( get<0>(bestTuples1[k]) != -1)
        {
            //int owner = OwnerProcs(A,  get<0>(bestTuples[k]), get<1>(bestTuples[k])); // (i,mi)
            //tempTuples[owner].push_back(bestTuples[k]);
            int owner = OwnerProcs(A,  get<3>(bestTuples[k]), get<1>(bestTuples[k]));
            winnerTuples[owner].push_back(make_tuple(get<0>(bestTuples[k]), get<1>(bestTuples[k]), get<2>(bestTuples[k]), get<3>(bestTuples[k])));
            col2rowBcastTuples.push_back(make_tuple(get<2>(bestTuples[k]), get<3>(bestTuples[k])));
            row2colBcastTuples.push_back(make_tuple(get<0>(bestTuples[k]), get<1>(bestTuples[k])));
        }
    }
    
    vector< tuple<IT,IT,IT, NT> >().swap(recvTuples1);
    
    
    
    vector<tuple<IT,IT,IT,IT>> recvWinnerTuples = ExchangeData1(winnerTuples, World);
    
    for(int k=0; k<recvWinnerTuples.size(); ++k)
    {
        IT i = get<0>(recvTuples1[k]) ;
        IT j = get<1>(recvTuples1[k]) ;
        IT mi = get<2>(recvTuples1[k]) ;
        IT mj = get<3>(recvTuples1[k]);
        col2rowBcastTuples.push_back(make_tuple(j,i));
        row2colBcastTuples.push_back(make_tuple(mj,mi));
    }
    
    
}

int main(int argc, char* argv[])
{
    
    // ------------ initialize MPI ---------------
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_SERIALIZED, &provided);
    if (provided < MPI_THREAD_SERIALIZED)
    {
        printf("ERROR: The MPI library does not have MPI_THREAD_SERIALIZED support\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    int nprocs, myrank;
    MPI_Comm_size(MPI_COMM_WORLD,&nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD,&myrank);
    if(argc < 3)
    {
        ShowUsage();
        MPI_Finalize();
        return -1;
    }
    
    init = DMD;
    randMaximal = false;
    prune = false;
    mvInvertMate = false;
    randMM = true;
    moreSplit = false;
    fewexp=false;
    
    SpParHelper::Print("***** I/O and other preprocessing steps *****\n");
    // ------------ Process input arguments and build matrix ---------------
    {
        
        Par_DCSC_Bool * ABool;
        Par_DCSC_Double * AWighted;
        ostringstream tinfo;
        double t01, t02;
        if(string(argv[1]) == string("input")) // input option
        {
            AWighted = new Par_DCSC_Double();
            
            string filename(argv[2]);
            tinfo.str("");
            tinfo << "\n**** Reading input matrix: " << filename << " ******* " << endl;
            SpParHelper::Print(tinfo.str());
            t01 = MPI_Wtime();
            AWighted->ParallelReadMM(filename, false, maximum<double>());
            t02 = MPI_Wtime();
            AWighted->PrintInfo();
            tinfo.str("");
            tinfo << "Reader took " << t02-t01 << " seconds" << endl;
            SpParHelper::Print(tinfo.str());
            //GetOptions(argv+3, argc-3);
            
        }
        else if(argc < 4)
        {
            ShowUsage();
            MPI_Finalize();
            return -1;
        }
        else
        {
            
            unsigned scale = (unsigned) atoi(argv[2]);
            unsigned EDGEFACTOR = (unsigned) atoi(argv[3]);
            double initiator[4];
            if(string(argv[1]) == string("er"))
            {
                initiator[0] = .25;
                initiator[1] = .25;
                initiator[2] = .25;
                initiator[3] = .25;
                if(myrank==0)
                    cout << "Randomly generated ER matric\n";
            }
            else if(string(argv[1]) == string("g500"))
            {
                initiator[0] = .57;
                initiator[1] = .19;
                initiator[2] = .19;
                initiator[3] = .05;
                if(myrank==0)
                    cout << "Randomly generated G500 matric\n";
            }
            else if(string(argv[1]) == string("ssca"))
            {
                initiator[0] = .6;
                initiator[1] = .4/3;
                initiator[2] = .4/3;
                initiator[3] = .4/3;
                if(myrank==0)
                    cout << "Randomly generated SSCA matric\n";
            }
            else
            {
                if(myrank == 0)
                    printf("The input type - %s - is not recognized.\n", argv[2]);
                MPI_Abort(MPI_COMM_WORLD, 1);
            }
            
            SpParHelper::Print("Generating input matrix....\n");
            t01 = MPI_Wtime();
            DistEdgeList<int64_t> * DEL = new DistEdgeList<int64_t>();
            DEL->GenGraph500Data(initiator, scale, EDGEFACTOR, true, true);
            AWighted = new Par_DCSC_Double(*DEL, false);
            // Add random weight ??
            delete DEL;
            t02 = MPI_Wtime();
            AWighted->PrintInfo();
            tinfo.str("");
            tinfo << "Generator took " << t02-t01 << " seconds" << endl;
            SpParHelper::Print(tinfo.str());
            
            Symmetricize(*AWighted);
            //removeIsolated(*ABool);
            SpParHelper::Print("Generated matrix symmetricized....\n");
            AWighted->PrintInfo();
            
            //GetOptions(argv+4, argc-4);
             
            
        }
        
        
        // randomly permute for load balance
        SpParHelper::Print("Performing random permutation of matrix.\n");
        FullyDistVec<int64_t, int64_t> prow(AWighted->getcommgrid());
        FullyDistVec<int64_t, int64_t> pcol(AWighted->getcommgrid());
        prow.iota(AWighted->getnrow(), 0);
        pcol.iota(AWighted->getncol(), 0);
        prow.RandPerm();
        pcol.RandPerm();
        (*AWighted)(prow, pcol, true);
        SpParHelper::Print("Performed random permutation of matrix.\n");
        
        
        Par_DCSC_Bool A = *AWighted;
        Par_DCSC_Bool AT = A;
        AT.Transpose();
        
        // Reduce is not multithreaded, so I am doing it here
        FullyDistVec<int64_t, int64_t> degCol(A.getcommgrid());
        A.Reduce(degCol, Column, plus<int64_t>(), static_cast<int64_t>(0));
        
        int nthreads;
#ifdef _OPENMP
#pragma omp parallel
        {
            int splitPerThread = 1;
            if(moreSplit) splitPerThread = 4;
            nthreads = omp_get_num_threads();
            cblas_splits = nthreads*splitPerThread;
        }
        tinfo.str("");
        tinfo << "Threading activated with " << nthreads << " threads, and matrix split into "<< cblas_splits <<  " parts" << endl;
        SpParHelper::Print(tinfo.str());
        A.ActivateThreading(cblas_splits); // note: crash on empty matrix
        AT.ActivateThreading(cblas_splits);
#endif
        
        
        SpParHelper::Print("**************************************************\n\n");
        
        // compute the maximum cardinality matching
        FullyDistVec<int64_t, int64_t> mateRow2Col ( A.getcommgrid(), A.getnrow(), (int64_t) -1);
        FullyDistVec<int64_t, int64_t> mateCol2Row ( A.getcommgrid(), A.getncol(), (int64_t) -1);
        
        // using best options for the maximum cardinality matching
        init = DMD; randMaximal = false; randMM = true; prune = true;
        MaximalMatching(A, AT, mateRow2Col, mateCol2Row, degCol, init, randMaximal);
        maximumMatching(A, mateRow2Col, mateCol2Row, prune, mvInvertMate, randMM);

        TwoThirdApprox(*AWighted, mateRow2Col, mateCol2Row);

        
    }
    MPI_Finalize();
    return 0;
}

