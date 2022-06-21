#pragma once
#include <iostream>
#include <stdexcept>
#include <string>
#include <sstream>
#include <ws2tcpip.h>

#include "Logger.h"

class Server
{
public:
  Server(int port);
  ~Server();

  int WaitForClientConnection();
  int WaitForClientData();
  inline bool IsWaiting() { return is_waiting; };

  void SetStopWaiting(bool b);
private:
  int port;
  SOCKET listen_sock;
  SOCKET client_sock;

  int CreateSocket();
  int BindSocket();
  int StartListen();
  int StopListen();

  std::atomic<bool> is_waiting = false;
  std::atomic<bool> stop_waiting = false;
};

