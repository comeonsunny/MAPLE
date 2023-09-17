#include "blockMAPLE.hpp"
#include "MAPLE.hpp"
BlockMAPLE::BlockMAPLE(TYPE_ID id, TYPE_SIZE dataChunks) {
    this->dataChunks = dataChunks;
    this->data = new TYPE_DATA[dataChunks];
    this->shares = new TYPE_DATA*[dataChunks];
    for (TYPE_SIZE i = 0; i < dataChunks; i++) {
        this->shares[i] = new TYPE_DATA[NUM_SERVERS];
    }
    for (TYPE_SIZE i = 0; i < dataChunks; i++) {
        data[i] = id;
    }
}
BlockMAPLE::~BlockMAPLE() {
    delete[] data;
    for (TYPE_SIZE i = 0; i < dataChunks; i++) {
        delete[] shares[i];
    }
    delete[] shares;
}
int BlockMAPLE::generateShares() {
    MAPLE MAPLE;
    for (TYPE_SIZE i = 0; i < dataChunks; i++) {
        MAPLE.createShares(data[i], shares[i]);
    }
    return 0;
}
TYPE_DATA** BlockMAPLE::getShares() {
    return shares;
}
DataChunkMAPLE::DataChunkMAPLE(TYPE_INDEX totalNumBlocks) {
    this->totalNumBlocks = totalNumBlocks;
    this->data = new TYPE_DATA[totalNumBlocks];
    for (TYPE_SIZE i = 0; i < totalNumBlocks; i++) {
        data[i] = i;
    }
    this->shares = new TYPE_DATA*[totalNumBlocks];
    for (TYPE_SIZE i = 0; i < totalNumBlocks; i++) {
        this->shares[i] = new TYPE_DATA[NUM_SERVERS];
    }
}
DataChunkMAPLE::~DataChunkMAPLE() {
    delete[] data;
    for (TYPE_SIZE i = 0; i < totalNumBlocks; i++) {
        delete[] shares[i];
    }
    delete[] shares;
}
int DataChunkMAPLE::generateShares() {
    MAPLE MAPLE;
    for (TYPE_SIZE i = 0; i < totalNumBlocks; i++) {
        MAPLE.createShares(data[i], shares[i]);
    }
    return 0;
}
TYPE_DATA** DataChunkMAPLE::getShares() {
    return shares;
}