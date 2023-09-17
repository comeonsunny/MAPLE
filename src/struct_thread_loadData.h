/*
 * struct_thread_loadData.h
 *
 *  Created on: May 1, 2017
 *      Author: ceyhunozkaptan, thanghoang
 */
 
#ifndef STRUCT_THREAD_LOADDATA_H
#define  STRUCT_THREAD_LOADDATA_H

#include "config.hpp"
typedef struct struct_thread_loadData
{
    int serverNo;
	zz_p** data_vector;
    TYPE_DATA* data_vector_swap;
   
    //thread
    TYPE_INDEX startIdx,endIdx;
    
    //retrieval
    TYPE_INDEX* fullLineIdx; 
    int fullLineIdx_length;

    //eviction
    TYPE_INDEX idx[2]; //incld src & dest (or sibl) for matrix permutation
    
    //for retrieval
     struct_thread_loadData(int serverNo, TYPE_INDEX start, TYPE_INDEX end, zz_p** data_vector, TYPE_INDEX* fullLineIdx, int fullLineIdx_length)
    {
        this->serverNo = serverNo;
        this->startIdx = start;
        this->endIdx = end;
        this->data_vector = data_vector;
        this->fullLineIdx = fullLineIdx;
        this->fullLineIdx_length = fullLineIdx_length;
    }
    // for swappping
    struct_thread_loadData(int serverNo, TYPE_INDEX start, TYPE_INDEX end, TYPE_DATA* data_vector, TYPE_INDEX* fullLineIdx, int fullLineIdx_length)
    {
        this->serverNo = serverNo;
        this->startIdx = start;
        this->endIdx = end;
        this->data_vector_swap = data_vector;
        this->fullLineIdx = fullLineIdx;
        this->fullLineIdx_length = fullLineIdx_length;
    }
    //for triplet eviction
    struct_thread_loadData(int serverNo, TYPE_INDEX start, TYPE_INDEX end, zz_p** data_vector, TYPE_INDEX srcIdx, TYPE_INDEX destIdx)
	{
		this->serverNo = serverNo;
        this->startIdx = start;
        this->endIdx = end;
        this->data_vector = data_vector;    
        this->idx[0] = srcIdx;
        this->idx[1] = destIdx;
    }

	struct_thread_loadData()
	{
	}
	~struct_thread_loadData()
	{
	}

}THREAD_LOADDATA;
#endif