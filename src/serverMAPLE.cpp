/*
    serverMAPLE.cpp
    Created by: 2022/11/06
        Author: changqi sun
        Email: changqischolar@qq.com
*/
#include "serverMAPLE.hpp"
#include "Utils.hpp"
#include "struct_socket.h"
#include "config.hpp"
#include "MAPLE.hpp"
#include "struct_thread_loadData.h"
#include "struct_thread_computation.h"

#include <iostream>
#include <fstream>
#include <cmath>
#include <sys/socket.h>
#include <sys/types.h>
#include <sched.h>
#include <mach/mach_init.h>
#include <mach/thread_policy.h>
#include <pthread.h>
static TYPE_SIZE totalNumBlocks_static;
static TYPE_SIZE lenNumBlocks_static;
static TYPE_INDEX dataChunks_static;
static std::__fs::filesystem::path path_static;
ServerMAPLE::ServerMAPLE(TYPE_SIZE DB_SIZE, TYPE_SIZE BLOCK_SIZE, TYPE_INDEX serverNo, int selectedThreads) {
    //=== DATA PARAMETER ================================
    dbSize = DB_SIZE;
    blockSize = BLOCK_SIZE;
    dataChunks = BLOCK_SIZE / sizeof(TYPE_DATA);
    realNumBlocks = this->dbSize % this->blockSize == 0 ? this->dbSize / this->blockSize : this->dbSize / this->blockSize + 1; 
    // lenNumBlocks equal to the value of the rounded up square root of realNumBlocks 
    lenNumBlocks = (TYPE_SIZE)ceil(sqrt(realNumBlocks));
    totalNumBlocks = lenNumBlocks * lenNumBlocks;
    totalNumBlocks_static = totalNumBlocks;
    lenNumBlocks_static = lenNumBlocks;
    dataChunks_static = dataChunks;
    //=== SOCKET PARAMETER ==============================
    this->CLIENT_IP = "tcp://*:" + std::to_string(SERVER_PORT+serverNo*NUM_SERVERS+serverNo);
    //=== THREAD PARAMETER ==============================
    this->numThreads = selectedThreads;
    this->thread_compute = new pthread_t[this->numThreads];
    this->thread_swap = new pthread_t[this->numThreads];
    //=== DIRECTORY ==============================
    this->dirPath = SERVER_DIR / ("server" + std::to_string(serverNo + 1));
    path_static = this->dirPath;
    //=== DATA BUFFER ==============================
    this->vector_buffer_in = new unsigned char[sizeof(bool) + 2*sizeof(TYPE_INDEX) + 2*sizeof(TYPE_DATA) * this->lenNumBlocks];
    this->block_buffer_out = new unsigned char[sizeof(TYPE_DATA) * this->dataChunks * 2];
    this->swapper_vector_buffer_in = new unsigned char[sizeof(TYPE_DATA) * 2 * this->lenNumBlocks];
    this->dot_product_vector = new zz_p**[2];
    for (int i = 0; i < 2; i++) {
        this->dot_product_vector[i] = new zz_p*[this->dataChunks];
        for (int j = 0; j < this->dataChunks; j++) {
            this->dot_product_vector[i][j] = new zz_p[this->lenNumBlocks];
        }
    }
    // this->dot_product_vector = new zz_p*[this->dataChunks];
    // for (int i = 0; i < this->dataChunks; i++) {
    //     this->dot_product_vector[i] = new zz_p[this->lenNumBlocks];
    // }
    // this->dot_product_vector_copy = new TYPE_DATA*[this->dataChunks];
    // for (int i = 0; i < this->dataChunks; i++) {
    //     this->dot_product_vector_copy[i] = new TYPE_DATA[this->lenNumBlocks];
    // }
    this->dot_product_vector_copy = new TYPE_DATA**[2];
    for (int i = 0; i < 2; i++) {
        this->dot_product_vector_copy[i] = new TYPE_DATA*[this->dataChunks];
        for (int j = 0; j < this->dataChunks; j++) {
            this->dot_product_vector_copy[i][j] = new TYPE_DATA[this->lenNumBlocks];
        }
    }
    this->swapper_shares_vector = new TYPE_DATA[2 * this->lenNumBlocks];
    this->sumBlock = new TYPE_DATA[this->dataChunks * 2];
    fullLineId = new TYPE_INDEX[this->lenNumBlocks];
    randFullLinedId = new TYPE_INDEX[this->lenNumBlocks];
    std::cout << std::endl;
    std::cout << "==============================" << std::endl;
    std::cout << "Starting ServerMAPLE..." << serverNo+1 << std::endl;
    std::cout << "==============================" << std::endl;
    this->serverNo = serverNo;
    
}
ServerMAPLE::ServerMAPLE() {

}
ServerMAPLE::~ServerMAPLE() {
}

/**
 * Function Name: run
 *
 * Description: Run the server to wait for a command from the client. 
 * According to the command, server performs certain subroutines for distributed ORAM operations.
 * 
 * @return 0 if successful
 */
int ServerMAPLE::run() {
    // prepare the initialzation of the socket
    int CMD;
    zmq::context_t context(1);
    zmq::socket_t socket(context, ZMQ_REP);
    std::cout << "[ServerMAPLE] Binding to " << this->CLIENT_IP << std::endl;
    socket.bind(this->CLIENT_IP.c_str());

    while (true) {
        std::cout << "[ServerMAPLE] Waiting for command from client..." << std::endl;
        zmq::message_t request_cmd;
        socket.recv(&request_cmd);
        CMD = *(int *)request_cmd.data();
        std::cout << "[ServerMAPLE] Received command: " << CMD << std::endl;
        zmq::message_t reply_cmd(CMD_SUCCESS, sizeof(CMD_SUCCESS));
        socket.send(reply_cmd);
        switch (CMD) {
            case CMD_SEND_MAPLE_MATRIX:
                std::cout << std::endl;
                std::cout << "==============================" << std::endl;
                std::cout << "[ServerMAPLE] Receiving Matrixed DataBase..." << std::endl;
                std::cout << "==============================" << std::endl;
                this->recvMatrixDatabase(socket);
                std::cout << std::endl;
                std::cout << "==============================" << std::endl;
                std::cout << "[ServerMAPLE] Matrixed DataBase Received!" << std::endl;
                std::cout << "==============================" << std::endl;
                break;
            case CMD_ACCESS_BLOCK:
                std::cout << std::endl;
                std::cout << "==============================" << std::endl;
                std::cout << "[ServerMAPLE] Accessing Block..." << std::endl;
                std::cout << "==============================" << std::endl;
                this->returnBlock(socket);
                std::cout << std::endl;
                std::cout << "==============================" << std::endl;
                std::cout << "[ServerMAPLE] Block Accessed!" << std::endl;
                std::cout << "==============================" << std::endl;
                break;
            case CMD_SWAP_BLOCKS:
                std::cout << std::endl;
                std::cout << "==============================" << std::endl;
                std::cout << "[ServerMAPLE] Swapping Blocks..." << std::endl;
                std::cout << "==============================" << std::endl;
                this->swapBlocks(socket);
                std::cout << std::endl;
                std::cout << "==============================" << std::endl;
                std::cout << "[ServerMAPLE] Blocks Swapped!" << std::endl;
                std::cout << "==============================" << std::endl;
                break;
            default:
                std::cout << "[ServerMAPLE] Unknown command: " << CMD << std::endl;
                break;
        }
    }
}

/**
 * Function Name: recvMatrixDatabase
 *
 * Description: Receive the matrixed database from the client.
 * 
 * @return 0 if successful
 */
int ServerMAPLE::recvMatrixDatabase(zmq::socket_t &socket) {
    TYPE_INDEX remain = this->totalNumBlocks % BUCKET_SIZE;
    TYPE_INDEX splitNum = remain == 0 ? this->totalNumBlocks / BUCKET_SIZE : this->totalNumBlocks / BUCKET_SIZE + 1; 
    TYPE_INDEX splitSize = this->blockSize * BUCKET_SIZE;
    TYPE_INDEX splitSizeLast = this->blockSize * (this->totalNumBlocks % BUCKET_SIZE);
    std::string fileName = "matrixedDB";
    std::ofstream ofs(this->dirPath / fileName, std::ios::binary);
    for (TYPE_INDEX i = 0; i < splitNum; i++) {
        TYPE_INDEX recvSize = (i == splitNum -1 && remain != 0) ? splitSizeLast : splitSize;
        zmq::message_t request_data(recvSize);
        socket.recv(&request_data);
        socket.send(zmq::message_t(CMD_SUCCESS, sizeof(CMD_SUCCESS)));
        TYPE_DATA *recvData = (TYPE_DATA *)request_data.data();
        ofs.write((char *)recvData, recvSize);
    }
    ofs.close();
    return 0;
}

/**
 * Function Name: returnBlock
 *
 * Description: Starts retrieve operation for a block by receiving is_row, line_id, and logical access vector from the client. 
 * According to is_row and line_id, server performs dot-product operation between its block shares on the line_id and logical access vector.
 * The result of the dot-product is send back to the client.
 * 
 * @param socket: (input) ZeroMQ socket instance for communication with the client
 * @return 0 if successful
 */  
int ServerMAPLE::returnBlock(zmq::socket_t& socket)
{	
	int ret = 1;
    // receive logical access vector and path ID from the client
	socket.recv(vector_buffer_in,sizeof(bool) + 2*sizeof(TYPE_INDEX) + 2 * this->lenNumBlocks*sizeof(TYPE_DATA), 0);
	
    bool is_row = *(bool *)vector_buffer_in;
	TYPE_INDEX line_id = *(TYPE_INDEX *)(vector_buffer_in + sizeof(bool));
    TYPE_INDEX random_line_id = *(TYPE_INDEX *)(vector_buffer_in + sizeof(bool) + sizeof(TYPE_INDEX));
    int num_vector = 2;
    zz_p** sharesVector = new zz_p*[num_vector];
    for (int i = 0; i < num_vector; i++) {
        sharesVector[i] = new zz_p[this->lenNumBlocks];
        memcpy(sharesVector[i], &vector_buffer_in[sizeof(bool) + 2*sizeof(TYPE_INDEX) + i*this->lenNumBlocks*sizeof(TYPE_DATA)], this->lenNumBlocks*sizeof(TYPE_DATA));
    }
    std::cout << "[serverMAPLE] is_row: " << (is_row ? "Row" : "Column") << ", line_id: " << line_id << std::endl;
    
    MAPLE MAPLE;
    MAPLE.getFullLineIndex(is_row, line_id, fullLineId, this->lenNumBlocks);
	MAPLE.getFullLineIndex(!is_row, random_line_id, randFullLinedId, this->lenNumBlocks);
    //use thread to load data from files
    int step = std::ceil((double)this->dataChunks/(double)numThreads);
    int endIdx;
    THREAD_LOADDATA loadData_args[numThreads];
    for (int i = 0, startIdx = 0; i < numThreads && startIdx < this->dataChunks; i++, startIdx += step) {
        if (startIdx + step > this->dataChunks)
            endIdx = this->dataChunks;
        else
            endIdx = startIdx + step;

        loadData_args[i] = THREAD_LOADDATA(this->serverNo, startIdx, endIdx, this->dot_product_vector[1], randFullLinedId, this->lenNumBlocks);
        pthread_create(&thread_compute[i], NULL, &ServerMAPLE::thread_loadRetrievalData_func, (void*)&loadData_args[i]);

        // Set thread affinity
        thread_affinity_policy_data_t policy = { i };
        thread_policy_set(pthread_mach_thread_np(thread_compute[i]), THREAD_AFFINITY_POLICY, (thread_policy_t)&policy, 1);
    }
    
    for(int i = 0, startIdx = 0 ; i < numThreads , startIdx < this->dataChunks; i ++, startIdx+=step)
    {
        pthread_join(thread_compute[i],NULL);
    }
    // Get the column of shares corresponding to the random or swapper block
    for(int i = 0, startIdx = 0; i < numThreads, startIdx < this->dataChunks; i++, startIdx+=step)
    {
        if(startIdx+step > this->dataChunks)
            endIdx = this->dataChunks;
        else
            endIdx = startIdx+step;
            
        loadData_args[i] = THREAD_LOADDATA(this->serverNo, startIdx, endIdx, this->dot_product_vector[1], randFullLinedId,this->lenNumBlocks);
        pthread_create(&thread_compute[i], NULL, &ServerMAPLE::thread_loadRetrievalData_func, (void*)&loadData_args[i]);
        
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(i, &cpuset);
        pthread_setaffinity_np(thread_compute[i], sizeof(cpu_set_t), &cpuset);
    }
    
    for(int i = 0, startIdx = 0 ; i < numThreads , startIdx < this->dataChunks; i ++, startIdx+=step)
    {
        pthread_join(thread_compute[i],NULL);
    }
    //Multithread for dot product computation
    THREAD_COMPUTATION dotProduct_args[numThreads];
    endIdx = 0;
    step = ceil((double)this->dataChunks/(double)numThreads);
    for(int i = 0, startIdx = 0 ; i < numThreads , startIdx < this->dataChunks; i ++, startIdx+=step)
    {
        if(startIdx+step > this->dataChunks)
            endIdx = this->dataChunks;
        else
            endIdx = startIdx+step;
			
        dotProduct_args[i] = THREAD_COMPUTATION( startIdx, endIdx, this->dot_product_vector, sharesVector, sumBlock);
        pthread_create(&thread_compute[i], NULL, &ServerMAPLE::thread_dotProduct_func, (void*)&dotProduct_args[i]);
		
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(i, &cpuset);
        pthread_setaffinity_np(thread_compute[i], sizeof(cpu_set_t), &cpuset);
    }
    
    for(int i = 0, startIdx = 0 ; i < numThreads , startIdx < this->dataChunks; i ++, startIdx+=step)
    {
        pthread_join(thread_compute[i],NULL);
    }
    

    memcpy(block_buffer_out,sumBlock,sizeof(TYPE_DATA)*this->dataChunks * 2);
    
    cout<< "[serverMAPLE] Sending Block Share with ID-" << sumBlock[0] <<endl;
    socket.send(block_buffer_out,sizeof(TYPE_DATA)*this->dataChunks*2, 0);
    for (int i = 0; i < num_vector; i++) {
        delete[] sharesVector[i];
    }
    delete[] sharesVector;
    ret = 0;
    return ret;
}

/**
 * Function Name: thread_loadRetrievalData_func
 *
 * Description: Threaded load function to read buckets in a path from disk storage
 * 
 */  
void* ServerMAPLE::thread_loadRetrievalData_func(void* args)
{
    THREAD_LOADDATA* opt = (THREAD_LOADDATA*) args;
    std::string dbFileNames = "matrixedDB";
    std::ifstream retrievalStream(path_static / dbFileNames, std::ios::binary);
    if (!retrievalStream.is_open())
    {
        std::cout << "[SendBlock] Error opening retrieval file" << std::endl;
        exit(1);
    }
    for (TYPE_INDEX i = opt->startIdx; i < opt->endIdx; ++i) {
        for (TYPE_INDEX j = 0; j < opt->fullLineIdx_length; ++j) {
            retrievalStream.seekg((opt->fullLineIdx[j] + i * totalNumBlocks_static) * sizeof(TYPE_DATA), std::ios::beg);
            retrievalStream.read((char*)&opt->data_vector[i][j], sizeof(TYPE_DATA));
        }
    }
    retrievalStream.close();
}

/**
 * Function Name: thread_dotProduct_func
 *
 * Description: Threaded dot-product operation 
 * 
 */  
void *ServerMAPLE::thread_dotProduct_func(void* args)
{
    THREAD_COMPUTATION* opt = (THREAD_COMPUTATION*) args;
    
  
    int size = lenNumBlocks_static;
    for(int k = opt->startIdx; k < opt->endIdx; k++)
    {
        opt->dot_product_output[k] = InnerProd_LL(opt->data_vector_newest[0][k],opt->multi_select_vector[0],size,P,zz_p::ll_red_struct());
        opt->dot_product_output[k + dataChunks_static] = InnerProd_LL(opt->data_vector_newest[1][k],opt->multi_select_vector[1],size,P,zz_p::ll_red_struct());
    }
}
/*
* Function Name: swapBlocks
* Description: Swap the target block with random block in dot_product_vector and write the updated dot_product_vector to disk
* Return: 0 if success
*/
int ServerMAPLE::swapBlocks(zmq::socket_t &socket) {
    // /*Copy the content in dot_product_vector into the dot_product_vector_copy*/
    // for (int i = 0; i < 2; i++) {
    //     for (TYPE_INDEX j = 0; j < this->dataChunks; j++) {
    //         for (TYPE_INDEX k = 0; k < this->lenNumBlocks; k++) {
    //             memcpy(&this->dot_product_vector_copy[i][j][k], &this->dot_product_vector[i][j][k], sizeof(TYPE_DATA));
    //         }
    //     }
    // }
    // std::cout << "[serverMAPLE] Write dot_product_vector_copy to disk" << std::endl;
    // // std::string filePath = std::string(ROOT_DIR) + "/dot_product_vector_copy";
    // // filePath = filePath + std::to_string(this->serverNo) + ".txt";
    // // std::ofstream ofs(filePath, std::ios::binary);
    // // if (!ofs.is_open()) {
    // //     std::cout << "Error opening file" << std::endl;
    // //     exit(1);
    // // }
    // // for (int i = 0; i < 2; i++) {
    // //     for (TYPE_INDEX j = 0; j < this->dataChunks; j++) {
    // //         for (TYPE_INDEX k = 0; k < this->lenNumBlocks; k++) {
    // //             ofs.write((char*)&this->dot_product_vector[i][j][k], sizeof(TYPE_DATA));
    // //         }
    // //     }
    // // }
    // // ofs.close();
    // std::cout << "[serverMAPLE] Swap Blocks" << std::endl;
    socket.recv(swapper_vector_buffer_in, 2 * this->lenNumBlocks * sizeof(TYPE_DATA), 0);
    memcpy(this->swapper_shares_vector, swapper_vector_buffer_in, 2 * this->lenNumBlocks * sizeof(TYPE_DATA));
    // for (TYPE_INDEX i = 0; i < 2*this->lenNumBlocks; ++i) {
    //     memcpy(&this->swapper_shares_vector[i], &swapper_vector_buffer_in[i * sizeof(TYPE_DATA)], sizeof(TYPE_DATA));
    // }
    // multithread for addition of dot_product_vector and swapper_shares_vector
    // THREAD_COMPUTATION add_args[numThreads];
    int endIdx = 0;
    int step = ceil((double)this->dataChunks/(double)numThreads);
    // for (int i = 0, startIdx = 0; i < numThreads, startIdx < this->dataChunks; ++i, startIdx += step) {
    //     if (startIdx + step > this->dataChunks)
    //         endIdx = this->dataChunks;
    //     else
    //         endIdx = startIdx + step;
    //     add_args[i] = THREAD_COMPUTATION(startIdx, endIdx, this->dot_product_vector_copy, this->swapper_shares_vector, NULL);
    //     pthread_create(&thread_swap[i], NULL, &ServerMAPLE::thread_add_func, (void*)&add_args[i]);
    //     cpu_set_t cpuset;
    //     CPU_ZERO(&cpuset);
    //     CPU_SET(i, &cpuset);
    //     pthread_setaffinity_np(thread_swap[i], sizeof(cpu_set_t), &cpuset);
    // }
    // for (int i = 0, startIdx = 0; i < numThreads, startIdx < this->dataChunks; ++i, startIdx += step) {
    //     pthread_join(thread_swap[i], NULL);
    // }
    // std::cout << "[serverMAPLE] Write dot_product_vector_copy to disk" << std::endl;
    // filePath = std::string(ROOT_DIR) + "/dot_product_vector_copy_updated";
    // filePath = filePath + std::to_string(this->serverNo) + ".txt";
    // ofs.open(filePath, std::ios::binary);
    // if (!ofs.is_open()) {
    //     std::cout << "Error opening file" << std::endl;
    //     exit(1);
    // }
    // for (int i = 0; i < 2; i++) {
    //     for (TYPE_INDEX j = 0; j < this->dataChunks; j++) {
    //         for (TYPE_INDEX k = 0; k < this->lenNumBlocks; k++) {
    //             ofs.write((char*)&this->dot_product_vector[i][j][k], sizeof(TYPE_DATA));
    //         }
    //     }
    // }
    // ofs.close();
    std::cout << "[serverMAPLE] Swap Blocks Done" << std::endl;
    std::cout << "======================================================================" << std::endl;
    std::cout << "[serverMAPLE] Write updated dot_product_vector to disk" << std::endl;
    THREAD_LOADDATA write_args[numThreads];
    step = ceil((double)this->dataChunks/(double)numThreads);
    endIdx = 0;
    for (int i = 0, startIdx = 0; i < numThreads, startIdx < this->dataChunks; ++i, startIdx += step) {
        if (startIdx + step > this->dataChunks)
            endIdx = this->dataChunks;
        else
            endIdx = startIdx + step;
        // write_args[i] = THREAD_LOADDATA(this->serverNo, startIdx, endIdx, this->dot_product_vector_copy[0], this->fullLineId, this->lenNumBlocks);
        write_args[i] = THREAD_LOADDATA(this->serverNo, startIdx, endIdx, this->swapper_shares_vector, this->fullLineId, this->lenNumBlocks);
        pthread_create(&thread_compute[i], NULL, &ServerMAPLE::thread_write_data_func, (void*)&write_args[i]);
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(i, &cpuset);
        pthread_setaffinity_np(thread_compute[i], sizeof(cpu_set_t), &cpuset);
    }
    for (int i = 0, startIdx = 0; i < numThreads, startIdx < this->dataChunks; ++i, startIdx += step) {
        pthread_join(thread_compute[i], NULL);
    }
    for (int i = 0, startIdx = 0; i < numThreads, startIdx < this->dataChunks; ++i, startIdx += step) {
        if (startIdx + step > this->dataChunks)
            endIdx = this->dataChunks;
        else
            endIdx = startIdx + step;
        write_args[i] = THREAD_LOADDATA(this->serverNo, startIdx, endIdx, this->swapper_shares_vector + this->lenNumBlocks, this->randFullLinedId, this->lenNumBlocks);
        pthread_create(&thread_compute[i], NULL, &ServerMAPLE::thread_write_data_func, (void*)&write_args[i]);
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(i, &cpuset);
        pthread_setaffinity_np(thread_compute[i], sizeof(cpu_set_t), &cpuset);
    }
    for (int i = 0, startIdx = 0; i < numThreads, startIdx < this->dataChunks; ++i, startIdx += step) {
        pthread_join(thread_compute[i], NULL);
    }
    socket.send((unsigned char*)CMD_SUCCESS, sizeof(CMD_SUCCESS), 0);
    std::cout << "[serverMAPLE] Write updated dot_product_vector to disk Done" << std::endl;
    return 0;
}
void *ServerMAPLE::thread_add_func(void* args) 
{
    THREAD_COMPUTATION* opt = (THREAD_COMPUTATION*) args;
    for(int k = opt->startIdx; k < opt->endIdx; k++)
    {
        for (TYPE_INDEX j = 0; j < lenNumBlocks_static; ++j) {
            opt->data_vector_copy[0][k][j] = (opt->data_vector_copy[0][k][j] + opt->select_vector_copy[j]) % P;
            opt->data_vector_copy[1][k][j] = (opt->data_vector_copy[1][k][j] + opt->select_vector_copy[j + lenNumBlocks_static]) % P;
        }
    }
}
/**
 * Function Name: thread_write_data_func
 *
 * Description: Threaded function for writing data to disk
 * 
 * @return 0 if successful
 */
void* ServerMAPLE::thread_write_data_func(void* args) {
    THREAD_LOADDATA* opt = (THREAD_LOADDATA*) args;
    std::string dbFileNames = "matrixedDB";
    std::fstream writeStream(path_static / dbFileNames, std::ios::binary | std::ios::in | std::ios::out);
    if (!writeStream.is_open()) {
        std::cout << "Error opening file" << std::endl;
        exit(1);
    }
    TYPE_DATA temp;
    for (TYPE_INDEX i = opt->startIdx; i < opt->endIdx; ++i) {
        for (TYPE_INDEX j = 0; j < opt->fullLineIdx_length; ++j) {
            writeStream.seekp((opt->fullLineIdx[j] + i * totalNumBlocks_static) * sizeof(TYPE_DATA), std::ios::beg);
            writeStream.read(reinterpret_cast<char*>(&temp), sizeof(TYPE_DATA));
            temp = (temp + opt->data_vector_swap[j]) % P;
            writeStream.seekp((opt->fullLineIdx[j] + i * totalNumBlocks_static) * sizeof(TYPE_DATA), std::ios::beg);
            writeStream.write(reinterpret_cast<const char*>(&temp), sizeof(TYPE_DATA));
            // writeStream.write(reinterpret_cast<const char*>(&opt->data_vector_swap[i][j]), sizeof(TYPE_DATA));
        }
    }
    // std::cout << "Thread "<< opt->endIdx << " finished writing data to disk" << std::endl;
    writeStream.close();
}