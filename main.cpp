#include "src/config.hpp"
#include "src/clientMAPLE.hpp"
#include "src/serverMAPLE.hpp"
#include "src/Log.hpp"
#include <iostream>
#include <thread>
#include <cstdlib>
#include <random>
using namespace std;
unsigned int nthreads = std::thread::hardware_concurrency();
int main(int argc, char *argv[])
{
    // create the data directory if it does not exist
    if (!std::experimental::filesystem::exists(DATA_DIR)) {
        std::experimental::filesystem::create_directory(DATA_DIR);
    }
    if (!std::experimental::filesystem::exists(LOG_DIR)) {
        std::experimental::filesystem::create_directory(LOG_DIR);
    }
    if (!std::experimental::filesystem::exists(DB_DIR)) {
        std::experimental::filesystem::create_directory(DB_DIR);
    }
    if (!std::experimental::filesystem::exists(SERVER_DIR)) {
        std::experimental::filesystem::create_directory(SERVER_DIR);
    }
    for (int i = 0; i < NUM_SERVERS; i++) {
        std::string server_dir = "server" + std::to_string(i + 1);
        if (!std::experimental::filesystem::exists(SERVER_DIR / server_dir)) {
            std::experimental::filesystem::create_directory(SERVER_DIR / server_dir);
        }
    }
    zz_p::init(P);
    //set random seed for NTL
    ZZ seed = conv<ZZ>("123456789101112131415161718192021222324");
    SetSeed(seed);
    // prepare the  directories 
    std::cout << "[Main] Number of threads: " << nthreads << std::endl;
    //==PARAMETERS ABOUT DATA SIZE=============================================
    TYPE_SIZE DB_SIZE = argc > 1 ? atoll(argv[1]) : 1; // unit: MB
    DB_SIZE *= 1024 * 1024; // unit: B
    TYPE_SIZE BLOCK_SIZE = argc > 2 ? atoll(argv[2]): 1; // unit: KB
    BLOCK_SIZE *= 1024; // unit: B
    TYPE_SIZE REALNUMBLOCKS = DB_SIZE % BLOCK_SIZE == 0 ? DB_SIZE / BLOCK_SIZE : DB_SIZE / BLOCK_SIZE + 1;
    int selectedThreads = argc > 5 ? atoi(argv[5]) : nthreads;
    int choice;
    if (argc > 3) {
        choice = atoi(argv[3]);
    } else {  
        cout << "Client or Server? (1/2): ";
        cin >> choice;
        cout << endl;
    }
    if (choice == 1)
    {
        if (std::experimental::filesystem::exists(CLIENT_DIR)) {
            std::experimental::filesystem::remove_all(CLIENT_DIR);
        }
        if (!std::experimental::filesystem::exists(CLIENT_DIR)) {
            std::experimental::filesystem::create_directory(CLIENT_DIR);
        }
        for (int i = 0; i < NUM_SERVERS; i++) {
            std::string server_dir = "server" + std::to_string(i + 1);
            if (!std::experimental::filesystem::exists(CLIENT_DIR / server_dir)) {
                std::experimental::filesystem::create_directory(CLIENT_DIR / server_dir);
            }
        }
        cout << "**************************************************" << endl;
        cout << "Client Starting ..." << endl;
        cout << "**************************************************" << endl;
        ClientMAPLE client(DB_SIZE, BLOCK_SIZE);
        // client.test();
        int accessCnt = argc > 4 ? atoi(argv[4]) : 1000000000;
        bool isRow = true;
        TYPE_DATA* data = new TYPE_DATA[BLOCK_SIZE];
        // get the current directory
        
        // change to the upper directory to find the directory named original_data_graph/threads_effect
        int dis_index = argc > 6 ? atoi(argv[6]) : 0;
        string third_layer_name = argc > 7 ? argv[7] : "database_size_1024";
        string dis_names[4] = {"single", "sequential", "even", "normal"};
        string log_path = ROOT_DIR.parent_path().parent_path().string() + "/log_files/" + dis_names[dis_index] + "/" + third_layer_name + "/MAPLE_v2/";
        string log_init_path;
        if (third_layer_name == "database_size_1024") {
            log_init_path = log_path + "init_block_size_" + to_string(BLOCK_SIZE / 1024) + ".txt";
            log_path += "block_size_" + to_string(BLOCK_SIZE / 1024) + ".txt";
        } else {
            log_init_path = log_path + "init_database_size_" + to_string(DB_SIZE / 1024 / 1024) + ".txt";
            log_path += "database_size_" + to_string(DB_SIZE / 1024 / 1024) + ".txt";
        }
        Log log_init(log_init_path);
        log_init.start();

        client.init();
        client.sendMAPLEmatrix();
        
        log_init.end();
        log_init.write_to_file();
        Log log(log_path);
        cout << "**************************************************" << endl;
        for (int i = 0; i < accessCnt; i++) {
            cout << "Access " << i + 1 << " times" << endl;
            TYPE_INDEX randIndex = 2;
            if (dis_index == 0) {
                randIndex = 4;
            } else if (dis_index == 1) {
                randIndex = i % REALNUMBLOCKS;
            }  else if (dis_index == 2) {
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> dis(0, REALNUMBLOCKS - 1);
                randIndex = dis(gen);
            } else {
                std::random_device rd;
                std::mt19937 gen(rd());
                std::normal_distribution<> dis(REALNUMBLOCKS / 2, REALNUMBLOCKS / 4);
                randIndex = static_cast<int>(dis(gen));
                randIndex = std::max(0, static_cast<int>(randIndex));
                randIndex = std::min(static_cast<int>(REALNUMBLOCKS - 1), static_cast<int>(randIndex));
            }
            log.start();
            client.access(randIndex, data, false, isRow);
            log.end();
        }
        log.write_to_file();
        delete[] data;
    }
    else if (choice == 2)
    {
        cout << "**************************************************" << endl;
        cout << "Server Starting ..." << endl;
        cout << "**************************************************" << endl;
        TYPE_INDEX server_id;
        if (argc > 4) {
            server_id = atoi(argv[4]);
        } else {
            cout << "Server ID: ";
            cin >> server_id;
            cout << endl;
        }
        ServerMAPLE server(DB_SIZE, BLOCK_SIZE, server_id - 1, selectedThreads);
        server.run();
    }
    else
    {
        cout << "Invalid choice" << endl;
    }
    return 0;
}
