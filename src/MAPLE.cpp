#include "MAPLE.hpp"
#include "Utils.hpp"
MAPLE::MAPLE() {
}
MAPLE::~MAPLE() {
}

/**
 * Function Name: createShares
 *
 * Description: Creates shares from an input based on Shamir's Secret Sharing algorithm
 * 
 * @param input: (input) The secret to be shared
 * @param output: (output) The array of shares generated from the secret
 * @return 0 if successful
 */  
int MAPLE::createShares(TYPE_DATA input, TYPE_DATA* output)
{
    TYPE_DATA random[PRIVACY_LEVEL];
    for ( int i = 0 ; i < PRIVACY_LEVEL ; i++)
    {
    #if defined(NTL_LIB)
        zz_p rand;
        NTL::random(rand);
        memcpy(&random[i], &rand,sizeof(TYPE_DATA));
    #else
        random[i] = Utils::_LongRand()+1 % P;
    #endif
    }
    for(unsigned long int i = 1; i <= NUM_SERVERS; i++)
    {
        output[i-1] = input;
        TYPE_DATA exp = i;
        for(int j = 1 ; j <= PRIVACY_LEVEL ; j++)
        {
            output[i-1] = (output[i-1] + Utils::mulmod(random[j-1],exp)) % P;
            exp = Utils::mulmod(exp,i);
	    }
    }
	return 0;
}
/**
 * Function Name: simpleRecover
 *
 * Description: Recovers the secret from NUM_SERVERS shares by using first row of Vandermonde matrix
 * 
 * @param shares: (input) Array of shared secret as data chunks
 * @param result: (output) Recovered secret from given shares
 * @return 0 if successful
 */  
int MAPLE::simpleRecover(TYPE_DATA** shares, TYPE_DATA* result, TYPE_ID DATA_CHUNKS)
{
    for(int i = 0; i < NUM_SERVERS; i++)
    {
        for(TYPE_ID k = 0; k < DATA_CHUNKS; k++)
        {
            result[k] = (result[k] + Utils::mulmod_real(vandermonde[i],shares[i][k])) % P; 
            // std::cout << "Utils::mulmod(vandermonde[" << i << "],shares[" << i << "][" << k << "]) = " << Utils::mulmod(vandermonde[i],shares[i][k]) << std::endl;
        }
    
	}

	cout << "	[MAPLE] Recovery is Done" << endl;
	
	return 0;
}

/**
 * Function Name: getSharesVector
 *
 * Description: Creates shares for NUM_SERVERS of servers from 1D array of logic values
 * 
 * @param logicVector: (input) 1D array of logical addresses
 * @param sharedVector: (output) 2D array of shares from array input
 * @return 0 if successful
 */  
int MAPLE::getSharesVector(TYPE_DATA* selectionVector, TYPE_DATA** sharesVector, TYPE_INDEX numBlocks)
{
	cout << "	[MAPLE] Creating shares for selection vector" << endl;

	TYPE_DATA* outputVector = new TYPE_DATA[NUM_SERVERS];

	for (TYPE_INDEX i = 0; i < numBlocks; ++i)
	{
		createShares(selectionVector[i],outputVector);
		for (int j = 0; j < NUM_SERVERS; j++){
			sharesVector[j][i] = outputVector[j];
		}
	}
    delete[] outputVector;
	return 0;
}
/**
 * Function Name: getFullLineIndex
 *
 * Description: Calculate the indexes of a row or a column in the full matrix with share block as unit
 * 
 * @param isRow: (input) true if the indexes are for a row, false if the indexes are for a column
 * @param line_id: (input) the id of the row or column
 * @param fullLineIndex: (output) the indexes of the row or column in the full matrix
 * @return 0 if successful
 */
int MAPLE::getFullLineIndex(bool isRow, TYPE_INDEX line_id, TYPE_INDEX* getFullLineIndex, TYPE_INDEX numLenBlocks) {
    if (isRow) {
        for (TYPE_INDEX i = 0; i < numLenBlocks; ++i) {
            getFullLineIndex[i] = line_id * numLenBlocks + i;
        }
    } else {
        for (TYPE_INDEX i = 0; i < numLenBlocks; ++i) {
            getFullLineIndex[i] = line_id + i * numLenBlocks;
        }
    }
    return 0;
}