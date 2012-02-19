/****************************************************************/
/* Parallel Combinatorial BLAS Library (for Graph Computations) */
/* version 1.2 -------------------------------------------------*/
/* date: 10/06/2011 --------------------------------------------*/
/* authors: Aydin Buluc (abuluc@lbl.gov), Adam Lugowski --------*/
/****************************************************************/
/*
 Copyright (c) 2011, Aydin Buluc
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 */

#ifndef _COMBBLAS_FILE_HEADER_
#define _COMBBLAS_FILE_HEADER_

struct HeaderInfo
{
	bool fileexists;
	bool headerexists;
	int16_t version;
	int16_t objsize;
	int16_t format;	// 0: binary, 1: ascii
	
	uint64_t m;
	uint64_t n;
	uint64_t nnz;
};
	

HeaderInfo ParseHeader(const string & inputname, FILE * f)
{
	f = fopen(inputname.c_str(), "r");
	HeaderInfo hinfo;
	memset(&hinfo, 0, sizeof(hinfo));
	if(!f)
	{
		cerr << "Problem reading binary input file\n";
		f = NULL;
		return hinfo;
	}
	char firstletter;
	fread(&firstletter, sizeof(firstletter), 1, f);
	if(firstletter != 'H')
	{
		cout << "First letter is " << firstletter << endl;
		cout << "Reverting to text mode" << endl;
		rewind(f);
		fclose(f);
		hinfo.fileexists = true;
		return hinfo;
	}
	else 
	{
		hinfo.fileexists = true;
		hinfo.headerexists = true;
	}

	
	fread(&(hinfo.version), sizeof(hinfo.version), 1, f);
	fread(&(hinfo.objsize), sizeof(hinfo.objsize), 1, f);
	fread(&(hinfo.format), sizeof(hinfo.format), 1, f);
	
	fread(&(hinfo.m), sizeof(hinfo.m), 1, f);
	fread(&(hinfo.n), sizeof(hinfo.n), 1, f);
	fread(&(hinfo.nnz), sizeof(hinfo.nnz), 1, f);
	return hinfo;
}
				  
#endif 

