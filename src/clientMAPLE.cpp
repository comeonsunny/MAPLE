#include "clientMAPLE.hpp"
#include "blockMAPLE.hpp"
#include "MAPLE.hpp"
#include "progressbar.hpp"
#include <cmath>
#include <random>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <cassert>
#include <sstream>
using namespace std;
// Path: src\clientMAPLE.cpp
ClientMAPLE::ClientMAPLE(TYPE_SIZE DB_SIZE, TYPE_SIZE BLOCK_SIZE)
{
    dbSize = DB_SIZE;
    blockSize = BLOCK_SIZE;
    dataChunks = BLOCK_SIZE / sizeof(TYPE_DATA);
    realNumBlocks = this->dbSize % this->blockSize == 0 ? this->dbSize / this->blockSize : this->dbSize / this->blockSize + 1; 
    // lenNumBlocks equal to the value of the rounded up square root of realNumBlocks 
    lenNumBlocks = (TYPE_SIZE)ceil(sqrt(realNumBlocks));
    totalNumBlocks = lenNumBlocks * lenNumBlocks;
    // metadata
    positionMap = new TYPE_POS_MAP[this->totalNumBlocks];
    shuffle_id = new TYPE_ID[this->totalNumBlocks];
    rowMatrix = new TYPE_INDEX*[this->lenNumBlocks];
    colMatrix = new TYPE_INDEX*[this->lenNumBlocks];
    for (TYPE_SIZE i = 0; i < this->lenNumBlocks; i++) {
        rowMatrix[i] = new TYPE_INDEX[this->lenNumBlocks];
        colMatrix[i] = new TYPE_INDEX[this->lenNumBlocks];
    }
    // data storage
    blockShares = new TYPE_DATA*[NUM_SERVERS];
    for (int i = 0; i < NUM_SERVERS; i++) {
        blockShares[i] = new TYPE_DATA[this->dataChunks];
    }
    sharesVector = new TYPE_DATA*[NUM_SERVERS];
    for (int i = 0; i < NUM_SERVERS; i++) {
        sharesVector[i] = new TYPE_DATA[this->lenNumBlocks * 2];
        memset(sharesVector[i], 0, this->lenNumBlocks * sizeof(TYPE_DATA) * 2);
    }
    vector_buffer_out = new unsigned char*[NUM_SERVERS];
    for (int i = 0; i < NUM_SERVERS; i++) {
        vector_buffer_out[i] = new unsigned char[sizeof(bool) + 2 * sizeof(TYPE_INDEX) + 2 * this->lenNumBlocks * sizeof(TYPE_DATA)];
    }
    blocks_buffer_in = new unsigned char*[NUM_SERVERS];
    for (int i = 0; i < NUM_SERVERS; i++) {
        blocks_buffer_in[i] = new unsigned char[2 * this->dataChunks * sizeof(TYPE_DATA)];
    }
    retrievedTargetShare = new TYPE_DATA*[NUM_SERVERS];
    retrievedSwapperShare = new TYPE_DATA*[NUM_SERVERS];
    for (int i = 0; i < NUM_SERVERS; i++) {
        retrievedTargetShare[i] = new TYPE_DATA[this->dataChunks];
        retrievedSwapperShare[i] = new TYPE_DATA[this->dataChunks];
    }
    recoveredTargetBlock = new TYPE_DATA[this->dataChunks];
    recoveredSwapperBlock = new TYPE_DATA[this->dataChunks];
    // directory
    dbPath = CLIENT_DIR;
    // selection vector
    selectionVector = new TYPE_DATA[2 * this->lenNumBlocks];
    swapperVector = new TYPE_DATA[2*this->lenNumBlocks];
    swapperSharesVector = new TYPE_DATA*[NUM_SERVERS];
    for (int i = 0; i < NUM_SERVERS; i++) {
        swapperSharesVector[i] = new TYPE_DATA[2 * this->lenNumBlocks];
    }
}
ClientMAPLE::~ClientMAPLE() {
    delete[] positionMap;
    delete[] shuffle_id;
    for (TYPE_SIZE i = 0; i < this->lenNumBlocks; i++) {
        delete[] rowMatrix[i];
        delete[] colMatrix[i];
    }
    delete[] rowMatrix;
    delete[] colMatrix;
    for (TYPE_SIZE i = 0; i < NUM_SERVERS; i++) {
        delete[] blockShares[i];
    }
    delete[] blockShares;
    for (TYPE_SIZE i = 0; i < NUM_SERVERS; i++) {
        delete[] sharesVector[i];
    }
    delete[] sharesVector;
    for (TYPE_SIZE i = 0; i < NUM_SERVERS; i++) {
        delete[] vector_buffer_out[i];
    }
    delete[] vector_buffer_out;
    for (TYPE_SIZE i = 0; i < NUM_SERVERS; i++) {
        delete[] blocks_buffer_in[i];
    }
    delete[] blocks_buffer_in;
    for (TYPE_SIZE i = 0; i < NUM_SERVERS; i++) {
        delete[] retrievedTargetShare[i];
        delete[] retrievedSwapperShare[i];
    }
    delete[] retrievedTargetShare;
    delete[] retrievedSwapperShare;
    delete[] recoveredTargetBlock;
    delete[] recoveredSwapperBlock;
    delete[] selectionVector;
    delete[] swapperVector;
    for (TYPE_SIZE i = 0; i < NUM_SERVERS; i++) {
        delete[] swapperSharesVector[i];
    }
    delete[] swapperSharesVector;
}
/**
 * Function Name: init()
 *
 * Description: Initialize the client side.
 *             1. Generate the position map.
 *             2. create the database file according to the position map.
 *             3. create a given number of shares for each block.
 *             4. send the shares database to corresponding servers.
 * 
 * @return 0 if successful
 */
int ClientMAPLE::init() {
    // 1. Generate the position map.
    /* step 1 fill up shuffle_id with sequential numbers*/
    for (TYPE_SIZE i = 0; i < totalNumBlocks; i++) {
        shuffle_id[i] = i;
    }
    /* step 2 shuffle the shuffle_id */
    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(shuffle_id, shuffle_id + totalNumBlocks, gen);

    /* step 3 fill up positionMap with the shuffled id */
    for (TYPE_SIZE i = 0; i < this->totalNumBlocks; i++) {
        TYPE_INDEX row_index = shuffle_id[i] / this->lenNumBlocks;
        TYPE_INDEX col_index = shuffle_id[i] % this->lenNumBlocks;
        positionMap[i].rowNum = row_index;
        positionMap[i].colNum = col_index;
        rowMatrix[row_index][col_index] = i;
        colMatrix[col_index][row_index] = i;
    }
    // 2. create the database file according to the position map and create a NUM_SERVERS number of shares for each block.
    /*step 1 generate original blocks and its corresponding NUM_SERVER shares and collect them into blockShares*/
    std::cout << std::endl << std::endl;
    std::cout << "===========================================================================================" << std::endl;
    std::cout << "Generating original blocks and its corresponding " << NUM_SERVERS << " shares..." << std::endl;
    TYPE_INDEX splitDataChunksNum = this->dataChunks / BUCKET_SIZE;
    TYPE_INDEX splitDataChunksRemainder = this->dataChunks % BUCKET_SIZE;
    splitDataChunksNum = splitDataChunksRemainder == 0 ? splitDataChunksNum : splitDataChunksNum + 1;
    TYPE_DATA*** dbShares = new TYPE_DATA**[NUM_SERVERS];
    for (TYPE_SIZE i = 0; i < NUM_SERVERS; i++) {
        dbShares[i] = new TYPE_DATA*[BUCKET_SIZE];
        for (TYPE_SIZE j = 0; j < BUCKET_SIZE; j++) {
            dbShares[i][j] = new TYPE_DATA[this->totalNumBlocks];
        }
    }
    progressbar bar_db(this->totalNumBlocks * NUM_SERVERS * this->dataChunks);
    for (TYPE_INDEX dc = 0; dc < splitDataChunksNum; dc++) {
        if (dc == splitDataChunksNum - 1 && splitDataChunksRemainder != 0) {
            for (TYPE_INDEX i = 0; i < splitDataChunksRemainder; i++) {
                DataChunkMAPLE dataChunk(this->totalNumBlocks);
                dataChunk.generateShares();
                TYPE_DATA** dataChunkShares = dataChunk.getShares();
                for (TYPE_SIZE j = 0; j < NUM_SERVERS; j++) {
                    for (TYPE_SIZE k = 0; k < this->totalNumBlocks; k++) {
                        dbShares[j][i][shuffle_id[k]] = dataChunkShares[k][j];
                        bar_db.update();
                    }
                }
            }
            // write the last splitDataChunksRemainder number of shares to the corresponding files
            for (TYPE_SIZE i = 0; i < NUM_SERVERS; i++) {
                std::string sharesDir = "server" + std::to_string(i + 1);
                std::__fs::filesystem::path dbPath = CLIENT_DIR / sharesDir / "db";
                std::ofstream dbFile(dbPath, std::ios::binary | std::ios::app);
                dbFile.seekp(dc * BUCKET_SIZE * this->totalNumBlocks * sizeof(TYPE_DATA), std::ios::beg);
                for (TYPE_SIZE j = 0; j < splitDataChunksRemainder; j++) {
                    dbFile.write((char*)dbShares[i][j], this->totalNumBlocks * sizeof(TYPE_DATA));
                }
            }
        } else {
            for (TYPE_INDEX i = 0; i < BUCKET_SIZE; i++) {
                DataChunkMAPLE dataChunk(this->totalNumBlocks);
                dataChunk.generateShares();
                TYPE_DATA** shares = dataChunk.getShares();
                for (TYPE_SIZE j = 0; j < NUM_SERVERS; j++) {
                    for (TYPE_SIZE k = 0; k < this->totalNumBlocks; k++) {
                        dbShares[j][i][shuffle_id[k]] = shares[k][j];
                        bar_db.update();
                    }
                }
            }
            // write the dbShares to the corresponding files
            for (TYPE_SIZE i = 0; i < NUM_SERVERS; i++) {
                std::string sharesDir = "server" + std::to_string(i + 1);
                std::__fs::filesystem::path dbPath = CLIENT_DIR / sharesDir / "db";
                std::ofstream dbFile(dbPath, std::ios::binary | std::ios::app);
                dbFile.seekp(dc * BUCKET_SIZE * this->totalNumBlocks * sizeof(TYPE_DATA), std::ios::beg);
                for (TYPE_SIZE j = 0; j < BUCKET_SIZE; j++) {
                    dbFile.write((char*)dbShares[i][j], this->totalNumBlocks * sizeof(TYPE_DATA));
                }
                dbFile.close();
            }
        }
    }
    std::cout << std::endl;
    std::cout << "===========================================================================================" << std::endl;
    for (TYPE_SIZE i = 0; i < NUM_SERVERS; i++) {
        for (TYPE_SIZE j = 0; j < BUCKET_SIZE; j++) {
            delete[] dbShares[i][j];
        }
        delete dbShares[i];
    }
    delete[] dbShares;
    // 3. send the shares database to corresponding servers.
    return 0;
}
int ClientMAPLE::test() {
    int cnt_ = 10;
    TYPE_DATA** dbShares = new TYPE_DATA*[NUM_SERVERS];
    for (TYPE_SIZE i = 0; i < NUM_SERVERS; i++) {
        dbShares[i] = new TYPE_DATA[cnt_*this->lenNumBlocks];
    }
    for (int i = 0; i < NUM_SERVERS; ++i) {
        std::fstream dbFile("/root/MAPLE_v2/build/dot_product_vector_1_"+std::to_string(i)+".txt", std::ios::in);
        std::string line;
        for (TYPE_INDEX j = 0; j < cnt_; ++j) {
            std::getline(dbFile, line);
            std::istringstream iss(line);
            // convert the iss to a vector
            std::vector<TYPE_DATA> vec((std::istream_iterator<TYPE_DATA>(iss)), std::istream_iterator<TYPE_DATA>());
            for (int k = 0; k < this->lenNumBlocks; ++k) {
                dbShares[i][j*this->lenNumBlocks+k] = vec[k];
            }
        }
    }
    TYPE_DATA* result_ = new TYPE_DATA[cnt_*this->lenNumBlocks];
    memset(result_, 0, cnt_*this->lenNumBlocks*sizeof(TYPE_DATA));
    MAPLE MAPLE_;
    MAPLE_.simpleRecover(dbShares, result_, cnt_*this->lenNumBlocks);
    for (int i = 0; i < cnt_*this->lenNumBlocks; ++i) {
        std::cout << result_[i] << " ";
    }
    cout << endl;
    for (TYPE_SIZE i = 0; i < NUM_SERVERS; i++) {
        delete[] dbShares[i];
    }
    delete[] dbShares;
    delete[] result_;
    return 0;
}
/**
 * Function Name: sendMAPLEmatrix()
 *
 * Description: Distributes generated and shared ORAM buckets to servers over network
 * 
 * @return 0 if successful
 */
int ClientMAPLE::sendMAPLEmatrix() {
    // 0. prepare the socket and data buffer
    /*socket preparation*/
    zmq::context_t context(1);
    zmq::socket_t socket(context, ZMQ_REQ);
    // struct_socket thread_args;
    /*data buffer preparation*/
    unsigned char* bucketShares_buff_out = new unsigned char[this->blockSize * BUCKET_SIZE];
    int CMD = CMD_SEND_MAPLE_MATRIX;
    unsigned char* CMD_buff_out = new unsigned char[sizeof(int)];
    memcpy(CMD_buff_out, &CMD, sizeof(int));
    unsigned char* CMD_buff_in = new unsigned char[sizeof(CMD_SUCCESS)];
    // 1. send CMD_SEND_MAPLE_MATRIX to server and wait for the response.
    // 2. open the database file and send the content to corresponding servers
    for (int i = 0; i < NUM_SERVERS; ++i) {
        string ADDR = SERVER_ADDR[i] + ":" + std::to_string(SERVER_PORT + i*NUM_SERVERS + i);
        cout << endl << endl;
        cout << "============================================================================" << endl;
        cout << "[Client::sendMAPLEmatrix] Connecting to server " << ADDR << "..." << endl;
        cout << "============================================================================" << endl;
        socket.connect(ADDR.c_str());
        // 1.1 send CMD_SEND_MAPLE_MATRIX
        zmq::message_t CMD_msg_out(CMD_buff_out, sizeof(int));
        socket.send(CMD_msg_out);
        cout << endl;
        cout << "============================================================================" << endl;
        cout << "[Client::sendMAPLEmatrix] CMD_SEND_MAPLE_MATRIX sent." << endl;
        cout << "============================================================================" << endl;
        // 1.2 receive CMD_SUCCESS
        zmq::message_t CMD_msg_in(CMD_buff_in, sizeof(CMD_SUCCESS));
        socket.recv(&CMD_msg_in);
        // assert(memcmp(CMD_buff_in, CMD_SUCCESS, sizeof(CMD_SUCCESS)) == 0 && "CMD_SUCCESS not received.");
        cout << endl;
        cout << "============================================================================" << endl;
        cout << "[Client::sendMAPLEmatrix] CMD_SUCCESS received." << endl;
        cout << "============================================================================" << endl;
        cout << endl << endl;
        // 2. send the content in the database file to corresponding servers
        cout << "============================================================================" << endl;
        cout << "[Client::sendMAPLEmatrix] Sending MAPLE matrix to server " << ADDR << "..." << endl;
        std::string sharesDir = "server" + std::to_string(i + 1);
        std::__fs::filesystem::path sharesPath = CLIENT_DIR / sharesDir / "db";
        TYPE_INDEX splitNum = this->totalNumBlocks % BUCKET_SIZE == 0 ? this->totalNumBlocks / BUCKET_SIZE : this->totalNumBlocks / BUCKET_SIZE + 1;
        TYPE_INDEX remain = this->totalNumBlocks % BUCKET_SIZE;
        progressbar bar_send(splitNum);
        for (TYPE_INDEX j = 0; j < splitNum; j++) {
            std::ifstream sharesFile(sharesPath, std::ios::binary);
            if (!sharesFile.is_open()) {
                std::cout << "Error: cannot open file " << sharesDir + std::to_string(0) << std::endl;
                return -1;
            }
            if (j == splitNum - 1 && remain != 0) {
                memset(bucketShares_buff_out, 0, this->blockSize * BUCKET_SIZE);
                sharesFile.seekg(j * BUCKET_SIZE * this->blockSize, std::ios::beg);
                sharesFile.read((char*)bucketShares_buff_out, remain * this->blockSize);
                zmq::message_t bucketShares_msg_out(bucketShares_buff_out, remain * this->blockSize);
                socket.send(bucketShares_msg_out);
                socket.recv(&CMD_msg_in);
            } else {
                sharesFile.seekg(j * BUCKET_SIZE * this->blockSize, std::ios::beg);
                sharesFile.read((char*)bucketShares_buff_out, BUCKET_SIZE * this->blockSize);
                zmq::message_t bucketShares_msg_out(bucketShares_buff_out, BUCKET_SIZE * this->blockSize);
                socket.send(bucketShares_msg_out);
                socket.recv(&CMD_msg_in);
            }
            sharesFile.close();
            bar_send.update();
        }
        socket.disconnect(ADDR.c_str());
        cout << endl;
        cout << "============================================================================" << endl;
    }
    socket.close();
    delete[] bucketShares_buff_out;
    delete[] CMD_buff_out;
    delete[] CMD_buff_in;
    return 0;
}  
/**
 * Function Name: TYPE_DATA* access(TYPE_INDEX blockIndex, TYPE_DATA* blockData, bool isWrite)
 *
 * Description: Access the block with blockID. If isWrite is true, write the blockData to the block.
 *              Otherwise, return the block data with blockID to the user.
 * 
 * @return correct block if successful
 */
TYPE_DATA* ClientMAPLE::access(TYPE_INDEX blockIndex, TYPE_DATA* blockData, bool isWrite, bool isRow) {
    std::cout << "============================================================================" << std::endl;
    std::cout << "[Client::access] Accessing block " << blockIndex << "..." << std::endl;
    std::cout << "============================================================================" << std::endl;
    // 1.  the blockIndex to its corresponding position in the ORAM matrix
    TYPE_INDEX line_id = isRow ? positionMap[blockIndex].rowNum : positionMap[blockIndex].colNum;
    TYPE_INDEX block_pos = isRow ? positionMap[blockIndex].colNum : positionMap[blockIndex].rowNum;
    std::cout << "[Client::access] line_id: " << line_id << ", block_pos: " << block_pos << std::endl;
        /*Generate the random index the block indexed by which will swap with the target block*/
        std::random_device rd;
        std::mt19937 gen(rd());
        // std::uniform_int_distribution<> dis(0, this->lenNumBlocks - 2);
        std::uniform_int_distribution<> dis(0, this->totalNumBlocks - 1);
        // TYPE_INDEX random_line_id = dis(gen);
        // if (random_line_id >= line_id) {
        //     random_line_id++;
        // }
        // std::uniform_int_distribution<> dis2(0, this->lenNumBlocks - 1);
        // TYPE_INDEX random_block_pos = dis2(gen);
    std::cout << "**********************************************************************" << std::endl;
        // std::cout << "[Client::access] RandomIndex: " << randomIndex << std::endl;
        // TYPE_ID swapperId = isRow ? rowMatrix[line_id][randomIndex] : colMatrix[line_id][randomIndex];
        // TYPE_ID swapperId = isRow ? rowMatrix[random_block_pos][random_line_id] : colMatrix[random_block_pos][random_line_id];
        TYPE_ID swapperId = dis(gen);
        std::cout << "[Client::access] SwapperId: " << swapperId << std::endl;
        // TYPE_INDEX randomIndex = isRow ? positionMap[swapperId].rowNum : positionMap[swapperId].colNum;
        TYPE_INDEX random_line_id = isRow ? positionMap[swapperId].colNum : positionMap[swapperId].rowNum;
        TYPE_INDEX random_block_pos = isRow ? positionMap[swapperId].rowNum : positionMap[swapperId].colNum;
    std::cout << "**********************************************************************" << std::endl;
    // 2. create the selection vector and its shares vector based on the position in the ORAM matrix and the value of isRow
    /*create the raw selected vector*/
    memset(selectionVector, 0, 2 * this->lenNumBlocks * sizeof(TYPE_DATA));
    selectionVector[block_pos] = 1;
    selectionVector[this->lenNumBlocks + random_block_pos] = 1;
    std::cout << std::endl;
    /*create its corresponding shares vector*/
    MAPLE oram;
    oram.getSharesVector(selectionVector, this->sharesVector, 2 * this->lenNumBlocks);
    // 3. send the CMD_ACCESS_BLOCK command and the shares vector to the corresponding server
    struct_socket thread_args[NUM_SERVERS];
    for (TYPE_ID i = 0; i < NUM_SERVERS; ++i) {
        memcpy(&vector_buffer_out[i][0], &isRow, sizeof(bool));
        memcpy(&vector_buffer_out[i][sizeof(bool)], &line_id, sizeof(TYPE_INDEX));
        memcpy(&vector_buffer_out[i][sizeof(bool) + sizeof(TYPE_INDEX)], &random_line_id, sizeof(TYPE_INDEX));
        memcpy(&vector_buffer_out[i][sizeof(bool) + 2 * sizeof(TYPE_INDEX)], &this->sharesVector[i][0], 2 * this->lenNumBlocks * sizeof(TYPE_DATA));

        thread_args[i] = struct_socket(SERVER_ADDR[i]+ ":" + std::to_string(SERVER_PORT+i*NUM_SERVERS+i), vector_buffer_out[i], sizeof(bool) + 2 * sizeof(TYPE_INDEX) + 2 * this->lenNumBlocks * sizeof(TYPE_DATA), blocks_buffer_in[i], 2 * sizeof(TYPE_DATA)*this->dataChunks,CMD_ACCESS_BLOCK,NULL);
        pthread_create(&thread_sockets[i], NULL, &ClientMAPLE::thread_socket_func, (void*)&thread_args[i]);
    }
    // 4. receive the shares of block of interest from its corresponding servers and reconstruct the block
    memset(recoveredTargetBlock, 0, sizeof(TYPE_DATA)*this->dataChunks);
    memset(recoveredSwapperBlock, 0, sizeof(TYPE_DATA)*this->dataChunks);
    
    for (int i = 0; i < NUM_SERVERS; i++)
    {
        pthread_join(thread_sockets[i], NULL);
        memcpy(retrievedTargetShare[i],blocks_buffer_in[i],sizeof(TYPE_DATA)*this->dataChunks);
        memcpy(retrievedSwapperShare[i],blocks_buffer_in[i]+sizeof(TYPE_DATA)*this->dataChunks,sizeof(TYPE_DATA)*this->dataChunks);
        std::cout << "[ClientMAPLE] From Server-" << i+1 << " => BlockIDShares = " << retrievedTargetShare[i][0]<< std::endl;
    }
    oram.simpleRecover(retrievedTargetShare, recoveredTargetBlock, this->dataChunks);
    oram.simpleRecover(retrievedSwapperShare, recoveredSwapperBlock, this->dataChunks);
    std::cout << "[ClientMAPLE] Recovered BlockID = " << recoveredTargetBlock[0] << std::endl;
    std::cout << "[ClientMAPLE] Recovered Swapper BlockID = " << recoveredSwapperBlock[0] << std::endl;
    for (TYPE_INDEX i = 0; i < this->dataChunks; i++) {
        assert(recoveredTargetBlock[i] == blockIndex && "[ERROR] Cannot recover the block correctly!");
        assert(recoveredSwapperBlock[i] == swapperId && "[ERROR] Cannot recover the swapper block correctly!");
    }
    // // 5. create the swapper vector and its shares vector to swap the target block with the swapper block
    memset(swapperVector, 0, 2*this->lenNumBlocks * sizeof(TYPE_DATA));
    TYPE_DATA target_block_share, swapper_block_share;
    if (blockIndex == swapperId) {
         oram.getSharesVector(swapperVector, this->swapperSharesVector, 2 * this->lenNumBlocks);
    } else {
        swapperVector[block_pos] = swapperId - blockIndex;
        swapperVector[this->lenNumBlocks + random_block_pos] = blockIndex - swapperId;
        oram.getSharesVector(swapperVector, this->swapperSharesVector, 2 * this->lenNumBlocks);
    }
    // 6. send the swapper shares vector to its corresponding servers 
    struct_socket thread_args2[NUM_SERVERS];
    for (int i = 0; i < NUM_SERVERS; ++i) {
        memcpy(&vector_buffer_out[i][0], &swapperSharesVector[i][0], 2*this->lenNumBlocks * sizeof(TYPE_DATA));
        thread_args2[i] = struct_socket(SERVER_ADDR[i]+ ":" + std::to_string(SERVER_PORT+i*NUM_SERVERS+i), vector_buffer_out[i], 2*this->lenNumBlocks * sizeof(TYPE_DATA), NULL, 0,CMD_SWAP_BLOCKS,NULL);
        pthread_create(&thread_sockets[i], NULL, &ClientMAPLE::thread_socket_func, (void*)&thread_args2[i]);
    }
    for (int i = 0; i < NUM_SERVERS; i++)
    {
        pthread_join(thread_sockets[i], NULL);
    }
    // 7. update the local metadata such as the position of the target block and the swapper block
    std::swap(this->positionMap[blockIndex].rowNum, this->positionMap[swapperId].rowNum);
    std::swap(this->positionMap[blockIndex].colNum, this->positionMap[swapperId].colNum);
    // // done
    TYPE_DATA* block;
    return block;
}

/**
 * Function Name: thread_socket_func & send
 *
 * Description: Generic threaded socket function for send and receive operations
 * 
 * @return 0 if successful
 */  
void* ClientMAPLE::thread_socket_func(void* args)
{
	struct_socket* opt = (struct_socket*) args;
	
	
	sendNrecv(opt->ADDR, opt->data_out, opt->data_out_size, opt->data_in, opt->data_in_size, opt->CMD);

		
    pthread_exit((void*)opt);
}
int ClientMAPLE::sendNrecv(std::string ADDR, unsigned char* data_out, size_t data_out_size, unsigned char* data_in, size_t data_in_size, int CMD)
{
	zmq::context_t context(1);
    zmq::socket_t socket(context,ZMQ_REQ);
    socket.connect(ADDR.c_str());
	
    unsigned char buffer_in[sizeof(CMD_SUCCESS)];
	unsigned char buffer_out[sizeof(CMD)];
	
    try
    {
        cout<< "	[ThreadSocket] Sending Command to"<< ADDR << endl;
        memcpy(buffer_out, &CMD,sizeof(CMD));
        socket.send(buffer_out, sizeof(CMD));
		cout<< "	[ThreadSocket] Command SENT! " << CMD <<endl;
        socket.recv(buffer_in, sizeof(CMD_SUCCESS));
		
		// auto start = time_now;
		cout<< "	[ThreadSocket] Sending Data..." << endl;
		socket.send (data_out, data_out_size);
		cout<< "	[ThreadSocket] Data SENT!" << endl;
        if(data_in_size == 0)
            socket.recv(buffer_in,sizeof(CMD_SUCCESS));
        else
            socket.recv(data_in,data_in_size);
            
		// auto end = time_now;
		// if(thread_max < std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count())
		// 	thread_max = std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count();
	}
    catch (exception &ex)
    {
        cout<< "	[ThreadSocket] Socket error!"<<endl;
		exit(0);
    }
	socket.disconnect(ADDR.c_str());
	return 0;
}
