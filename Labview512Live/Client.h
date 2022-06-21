#pragma once
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <numeric>
#include <vector>
#include <bitset>
#include <stdexcept>
#include <string>
#include <sstream>
#include <mutex>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

//#include "Application.h"

class Client
{
public:
  /*Data* data_ptr*/
  Client();
  ~Client();
  
  void Cleanup();
  int Connect(std::string address, int port);
  int Send(const char* send_buf);
  int Receive();
  int ParseData();
  void SetStopReceiving(bool b);

  char* getBuffer();
private:
  // Constants
  static const int MAX_BUFFER_SIZE = (256*3+2)*10000 + 770;
  static const int BYTES_PER_SAMPLE = 770;
  static const int ELECTRODE_ARRAY_SIZE = 512;
  static const int SAMPLE_RATE = 20000;
  // Network Info
  SOCKET sock;
  std::string server_ipAddress;
  int server_port;
  // Buffer
  char* buffer; // has to be a C-style array because of the nature of recv()
  char* buffer_overflow;
  int bytes_received;
  int buffer_size;
  int overflow_bytes = 0;
  bool b_header_is_parsed = false;
  bool b_stop_receiving = false;
  // Data
  unsigned int samples_parsed = 0;
  //int data_sample_index = 0;
  //Data* data; 


  int InitializeWinsock();
  int CreateSocket();
  int ParseHeader();
  int Unpack(int sample_num);
  int AnalyzeData();
//  int WriteToFile();
};

