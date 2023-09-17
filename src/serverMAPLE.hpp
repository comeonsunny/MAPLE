#ifndef SERVERMAPLE_HPP
#define SERVERMAPLE_HPP
#include "config.hpp"
#include "zmq.hpp"
#include <sched.h>
// include the thread library header
#include <pthread.h>
class ServerMAPLE {
private:
    //=== DATA PARAMETER ================================
    TYPE_SIZE dbSize;
    TYPE_SIZE blockSize;
    TYPE_SIZE dataChunks;
    TYPE_SIZE realNumBlocks;
    TYPE_SIZE lenNumBlocks;
    TYPE_SIZE totalNumBlocks;

    TYPE_INDEX serverNo;
    //=== SOCKET PARAMETER ==============================
    std::string CLIENT_IP;
    //=== THREAD PARAMETER ==============================
    int numThreads;
    pthread_t *thread_compute;
    pthread_t *thread_swap;
    //=== DIRECTORY ==============================
    std::__fs::filesystem::path dirPath;
    //=== DATA BUFFER ==============================
    unsigned char* vector_buffer_in;
    unsigned char* block_buffer_out;
    unsigned char* swapper_vector_buffer_in;
    //variables for retrieval
    zz_p*** dot_product_vector;
    // zz_p*  swapper_shares_vector;
    TYPE_DATA* swapper_shares_vector;
    TYPE_DATA*** dot_product_vector_copy;
    TYPE_DATA* sumBlock;
    TYPE_INDEX* fullLineId;
    TYPE_INDEX* randFullLinedId;
public:
    ServerMAPLE();
    ServerMAPLE(TYPE_SIZE DB_SIZE, TYPE_SIZE BLOCK_SIZE, TYPE_INDEX serverNo, int selectedThreads);
    ~ServerMAPLE();
    int run();
    //=== MAIN FUNCTION =================================
    int recvMatrixDatabase(zmq::socket_t &socket);
    int returnBlock(zmq::socket_t &socket);
    int swapBlocks(zmq::socket_t &socket);
    //=== THREAD FUNCTION ===============================
    static void* thread_loadRetrievalData_func(void* args);
    static void* thread_dotProduct_func(void* args);
    static void* thread_add_func(void* args);
    static void* thread_write_data_func(void* args);
};

#endif // SERVERMAPLE_HPP