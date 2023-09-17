#ifndef CONFIG_HPP
#define CONFIG_HPP
#include <filesystem>
#include <string>
#include <iostream>
#define NTL_LIB 
//=== PARAMETERS =====================================
static const long long P =536871173; //prime field - should have length equal to the defined TYPE_DATA
#define PRIVACY_LEVEL 1
#define NUM_SERVERS 3
#define BUCKET_SIZE 256
const long long int vandermonde[NUM_SERVERS] = {3 , -3 + P, 1};
//=== DATA TYPE ======================================
using TYPE_DATA =  long long;
using TYPE_INDEX = long long  int;
using TYPE_ID = unsigned long long;
using TYPE_SIZE = long long int;
typedef struct type_position_map {
    TYPE_INDEX rowNum;
    TYPE_INDEX colNum;
} TYPE_POS_MAP;
//=== PATH DIRECTORY =================================
static std::__fs::filesystem::path ROOT_DIR = std::__fs::filesystem::current_path();
static std::__fs::filesystem::path DATA_DIR = ROOT_DIR / "data";
static std::__fs::filesystem::path LOG_DIR = DATA_DIR / "log_data";
static std::__fs::filesystem::path DB_DIR = DATA_DIR / "db";
static std::__fs::filesystem::path CLIENT_DIR = DB_DIR / "client_side";
static std::__fs::filesystem::path SERVER_DIR = DB_DIR / "server_side";

//=== NON-MODIFIABLE PARAMETER ================================================
#if defined(NTL_LIB)
    #include "NTL/ZZ.h"
    #include "NTL/tools.h"
    #include "NTL/GF2E.h"
    #include <NTL/WordVector.h>
    #include <NTL/vector.h>
    #include "NTL/lzz_p.h"
    #include "NTL/ZZ_p.h"
    using namespace NTL;
#endif
//=== SOCKET PARAMETER =================================
#define CMD_SEND_MAPLE_MATRIX 0x000010
#define CMD_ACCESS_BLOCK         0x000011
#define CMD_SWAP_BLOCKS          0x000012
#define CMD_SUCCESS              "OK"

//=== SERVER INFO ============================================================

	//SERVER IP ADDRESSES
// const std::string SERVER_ADDR[NUM_SERVERS] = {"tcp://192.168.31.86", "tcp://192.168.31.202", "tcp://192.168.31.39"}; 	
const std::string SERVER_ADDR[NUM_SERVERS] = {"tcp://localhost", "tcp://localhost", "tcp://localhost"};
// const std::string SERVER_ADDR[NUM_SERVERS] = {"tcp://localhost", "tcp://localhost", "tcp://localhost", "tcp://localhost", "tcp://localhost", "tcp://localhost", "tcp://localhost"}; 	
#define SERVER_PORT 6655        //define the first port to generate incremental ports for client-server /server-server communications

#endif // CONFIG_HPP
