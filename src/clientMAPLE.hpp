#ifndef CLIENTMAPLE_HPP
#define CLIENTMAPLE_HPP
#include "config.hpp"
#include "struct_socket.h"
#include "zmq.hpp"
#include <pthread.h>
class ClientMAPLE {
public:
    ClientMAPLE(TYPE_SIZE DB_SIZE, TYPE_SIZE BLOCK_SIZE);
    ~ClientMAPLE();
    int init();
    int sendMAPLEmatrix();
    TYPE_DATA* access(TYPE_INDEX blockIndex, TYPE_DATA* blockData, bool isWrite, bool isRow);
    int test();
    //socket
	static void* thread_socket_func(void* args);	
    static int sendNrecv(std::string ADDR, unsigned char* data_out, size_t data_out_size, unsigned char* data_in, size_t data_in_size, int CMD);

private:
    //=== DATA STRUCTURE =================================
    TYPE_POS_MAP* positionMap;
    TYPE_INDEX** rowMatrix;
    TYPE_INDEX** colMatrix;
    TYPE_ID* shuffle_id;
    //== DATA SIZE =======================================
    TYPE_SIZE dbSize;
    TYPE_SIZE blockSize;
    TYPE_SIZE dataChunks;
    TYPE_SIZE realNumBlocks;
    TYPE_SIZE lenNumBlocks;
    TYPE_SIZE totalNumBlocks;
    //== DATA STORAGE ====================================
    TYPE_DATA **blockShares; // blockShares[i][j] is the ith server's jth share of the dataChunks
    TYPE_DATA **sharesVector;

    unsigned char** vector_buffer_out;
    unsigned char** blocks_buffer_in;

    TYPE_DATA** retrievedTargetShare;
    TYPE_DATA** retrievedSwapperShare;
    TYPE_DATA* recoveredTargetBlock;
    TYPE_DATA* recoveredSwapperBlock;
    //== DIRECTORY =======================================
    std::__fs::filesystem::path dbPath;
    //== THREADS =========================================
    pthread_t thread_sockets[NUM_SERVERS];
    //== SELECTION VECTOR ================================
    TYPE_DATA* selectionVector;
    TYPE_DATA* swapperVector;
    TYPE_DATA** swapperSharesVector;
    
};
#endif // CLIENTMAPLE_HPP