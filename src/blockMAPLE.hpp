#ifndef BLOCKMAPLE_HPP
#define BLOCKMAPLE_HPP
// the class BlockMAPLE is used to 
// 1) generate the block with the dataChunks size and its storage unit is TYPE_DATA
// 2) generate shares for each TYPE_DATA unit and store them in the shares array
// 3) return the shares element by specifying the index
#include "config.hpp"
class BlockMAPLE {
private:
    TYPE_DATA* data;
    TYPE_DATA** shares;
    TYPE_SIZE dataChunks;
public:
    BlockMAPLE(TYPE_ID id, TYPE_SIZE dataChunks);
    ~BlockMAPLE();
    int generateShares();
    TYPE_DATA** getShares();
};
class DataChunkMAPLE {
private:
    TYPE_DATA* data;
    TYPE_DATA** shares;
    TYPE_SIZE totalNumBlocks;
public:
    DataChunkMAPLE(TYPE_INDEX totalNumBlocks);
    ~DataChunkMAPLE();
    int generateShares();
    TYPE_DATA** getShares();
};

#endif // BLOCKMAPLE_HPP