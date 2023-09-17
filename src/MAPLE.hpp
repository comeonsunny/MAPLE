#ifndef MAPLE_HPP
#define MAPLE_HPP
#include "config.hpp"
class MAPLE {
public:
    MAPLE();
    ~MAPLE();
    int createShares(TYPE_DATA input, TYPE_DATA* output);
    int simpleRecover(TYPE_DATA** shares, TYPE_DATA* result, TYPE_ID DATA_CHUNKS);
    //=== client functions ==============================
    int getSharesVector(TYPE_DATA* selectionVector, TYPE_DATA** sharesVector, TYPE_INDEX numBlocks);

    //=== server functions ==============================
    int getFullLineIndex(bool isRow, TYPE_INDEX line_id, TYPE_INDEX* fullLineIndex, TYPE_INDEX numLenBlocks);

};
#endif // MAPLE_HPP