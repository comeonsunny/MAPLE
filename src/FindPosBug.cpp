#include <iostream>
#include "MAPLE.hpp"
#include "config.hpp"
#include <fstream>
#include <cstring>

void readVectorFromFile(const std::string& filename) {
    TYPE_SIZE DB_SIZE = 16 * 1024;
    TYPE_SIZE BLOCK_SIZE = 1024;
    TYPE_SIZE dataChunks = BLOCK_SIZE / sizeof(TYPE_DATA);
    TYPE_SIZE realNumBlocks = DB_SIZE % BLOCK_SIZE == 0 ? DB_SIZE / BLOCK_SIZE : DB_SIZE / BLOCK_SIZE + 1;
    TYPE_SIZE lenNumBlocks = static_cast<TYPE_SIZE>(ceil(sqrt(realNumBlocks)));
    int cnt = 2;
    TYPE_DATA** dbShares = new TYPE_DATA*[NUM_SERVERS];
    for (int i = 0; i < NUM_SERVERS; i++) {
        dbShares[i] = new TYPE_DATA[cnt * lenNumBlocks];
    }

    std::cout << "The value of lenNumBlocks is " << lenNumBlocks << std::endl;
    for (int i = 0; i < NUM_SERVERS; i++) {
        // std::string fileToRead = "../build/"+ filename + std::to_string(i) + ".txt";
        // std::cout << "The file to read is " << fileToRead << std::endl;
        std::string fileToRead = "../build/dot_product_vector_copy" + std::to_string(i) + ".txt";
        std::ifstream file(fileToRead);
        if (file.is_open()) {
            file.read(reinterpret_cast<char*>(dbShares[i]), cnt * lenNumBlocks * sizeof(TYPE_DATA));
        }
        // if (file.is_open()) {
        //     file.read(reinterpret_cast<char*>(dbShares[i]), lenNumBlocks * sizeof(TYPE_DATA));
        //     file.seekg(dataChunks * lenNumBlocks * sizeof(TYPE_DATA), std::ios::beg);
        //     file.read((reinterpret_cast<char*>(dbShares[i]) + lenNumBlocks * sizeof(TYPE_DATA)), lenNumBlocks * sizeof(TYPE_DATA));
        // }
        file.close();
    }

    // for (int i = 0; i < NUM_SERVERS; i++) {
    //     for (int j = 0; j < cnt; j++) {
    //         std::cout << "The Part " << i << " of the dbShares is " << std::endl;
    //         for (int k = 0; k < lenNumBlocks; k++) {
    //             std::cout << dbShares[i][j * lenNumBlocks + k] << " ";
    //         }
    //         std::cout << std::endl;
    //     }
    //     std::cout << std::endl;
    // }

    TYPE_DATA* result = new TYPE_DATA[cnt * lenNumBlocks];
    std::memset(result, 0, cnt * lenNumBlocks * sizeof(TYPE_DATA));
    MAPLE maple;
    maple.simpleRecover(dbShares, result, cnt * lenNumBlocks);
    for (int i = 0; i < cnt; i++) {
        std::cout << "The result of the " << i << "th part is " << std::endl;
        for (int j = 0; j < lenNumBlocks; j++) {
            std::cout << result[j] << " ";
        }
        std::cout << std::endl;
    }

    std::cout << std::endl;

    delete[] result;
    for (int i = 0; i < NUM_SERVERS; i++) {
        delete[] dbShares[i];
    }
    delete[] dbShares;
}
void readDataFromFile(const std::string& filename, int target_val) {
    TYPE_SIZE DB_SIZE = 16 * 1024;
    TYPE_SIZE BLOCK_SIZE = 1024;
    TYPE_SIZE dataChunks = BLOCK_SIZE / sizeof(TYPE_DATA);
    TYPE_SIZE realNumBlocks = DB_SIZE % BLOCK_SIZE == 0 ? DB_SIZE / BLOCK_SIZE : DB_SIZE / BLOCK_SIZE + 1;
    TYPE_SIZE lenNumBlocks = static_cast<TYPE_SIZE>(ceil(sqrt(realNumBlocks)));
    int cnt = dataChunks;
    TYPE_DATA** dbShares = new TYPE_DATA*[NUM_SERVERS];
    for (int i = 0; i < NUM_SERVERS; i++) {
        dbShares[i] = new TYPE_DATA[lenNumBlocks * lenNumBlocks];
    }

    std::cout << "The value of lenNumBlocks is " << lenNumBlocks << std::endl;
    for (int i = 0; i < NUM_SERVERS; i++) {
        // std::string fileToRead = "../build/"+ filename + std::to_string(i) + ".txt";
        std::string baseDirectory = "../build/data/db/server_side/server";
        // std::string baseDirectory = "../build/data/db/client_side/server";
        std::string fileToRead = baseDirectory + std::to_string(i + 1) + "/matrixedDB";
        // std::string fileToRead = baseDirectory + std::to_string(i + 1) + "/db";
        std::ifstream file(fileToRead);
        if (file.is_open()) {
            file.read(reinterpret_cast<char*>(dbShares[i]), lenNumBlocks * lenNumBlocks * sizeof(TYPE_DATA));
        }
        // if (file.is_open()) {
        //     file.read(reinterpret_cast<char*>(dbShares[i]), lenNumBlocks * sizeof(TYPE_DATA));
        //     file.seekg(dataChunks * lenNumBlocks * sizeof(TYPE_DATA), std::ios::beg);
        //     file.read(reinterpret_cast<char*>(dbShares[i] + lenNumBlocks), lenNumBlocks * sizeof(TYPE_DATA));
        // }
        file.close();
    }

    // for (int i = 0; i < NUM_SERVERS; i++) {
    //     for (int j = 0; j < cnt; j++) {
    //         std::cout << "The Part " << i << " of the dbShares is " << std::endl;
    //         for (int k = 0; k < lenNumBlocks; k++) {
    //             std::cout << dbShares[i][j * lenNumBlocks + k] << " ";
    //         }
    //         std::cout << std::endl;
    //     }
    //     std::cout << std::endl;
    // }

    TYPE_DATA* result = new TYPE_DATA[lenNumBlocks * lenNumBlocks];
    std::memset(result, 0, lenNumBlocks * lenNumBlocks * sizeof(TYPE_DATA));
    MAPLE maple;
    maple.simpleRecover(dbShares, result, cnt * lenNumBlocks);
    // get the times of the appearance of integer of 4 in the result
    // int times = 0;
    // for (int i = 0; i < cnt * lenNumBlocks; i++) {
    //     if (result[i] == target_val) {
    //         times++;
    //     }
    // }
    // std::cout << "The times of the appearance of integer of 4 in the result is " << times << std::endl;
    for (int i = 0; i < lenNumBlocks; i++) {
        std::cout << "The result of the " << i << "th part is " << std::endl;
        for (int j = 0; j < lenNumBlocks; j++) {
            std::cout << result[i * lenNumBlocks + j] << " ";
        }
        std::cout << std::endl;
    }

    std::cout << std::endl;

    delete[] result;
    for (int i = 0; i < NUM_SERVERS; i++) {
        delete[] dbShares[i];
    }
    delete[] dbShares;
}
int main(int argc, char** argv) {
    if (argc < 3) {
        std::cout << "Usage: ./a.out <filename> <target_val>" << std::endl;
        return 0;
    }
    std::string filename = argv[1];
    int target_val = atoi(argv[2]);
    readDataFromFile(filename, target_val);
    // readVectorFromFile("dot_product_vector_copy");
    // std::cout << "The updated vector is " << std::endl;
    // readVectorFromFile("dot_product_vector_copy_updated");
    return 0;
}
