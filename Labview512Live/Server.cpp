#include "Server.h"

Server::Server(int port)
{
  Server::port = port;
  try
  {
    CreateSocket();
    BindSocket();
  }
  catch (std::runtime_error e) {
    throw e;
  }
}

Server::~Server()
{
  closesocket(listen_sock);
  closesocket(client_sock);
  WSACleanup();
}

int Server::WaitForClientConnection()
{
  StartListen();
  // Wait for a connection
  sockaddr_in client;
  int clientSize = sizeof(client);

  FD_SET read_set;
  FD_SET write_set;
  bool client_found = false;
  is_waiting = true;
  while (!stop_waiting & !client_found)
  {
    FD_ZERO(&read_set);
    FD_ZERO(&write_set);
    FD_SET(listen_sock, &read_set);
    TIMEVAL timeout = { 1, 0 };
    LOG("Waiting...");
    int select_return = select(0, &read_set, &write_set, NULL, &timeout);
    if (select_return == INVALID_SOCKET)
    {
      std::ostringstream error;
      error << "Select function failed, Err #" << WSAGetLastError();
      throw std::runtime_error(error.str());
    }
    else if (select_return != 0)
    {
      client_found = true;
    }
  }
  
  is_waiting = false;
  if (stop_waiting)
  {
    return -1;
  }
  
  LOG("PRE ACCEPT");
  //std::cout << "Waiting for client to connect..." << std::endl;
  client_sock = accept(listen_sock, (sockaddr*)&client, &clientSize);
  if (client_sock == INVALID_SOCKET)
  {
    std::ostringstream error;
    error << "Accept Client on socket failed, Err #" << WSAGetLastError();
    throw std::runtime_error(error.str());
  }
  else
  {
    LOG("CLIENT ACCEPTED");
  }
  char host[NI_MAXHOST];		// Client's remote name
  char service[NI_MAXSERV];	// Service (i.e. port) the client is connect on

  ZeroMemory(host, NI_MAXHOST); // same as memset(host, 0, NI_MAXHOST);
  ZeroMemory(service, NI_MAXSERV);

  if (getnameinfo((sockaddr*)&client, sizeof(client), host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0)
  {
    //std::cout << host << " connected on port " << service << std::endl;
  }
  else
  {
    inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
    //std::cout << host << " connected on port " << ntohs(client.sin_port) << std::endl;
  }
  StopListen();
  return 0;
}

int Server::WaitForClientData()
{
  // While loop: accept and echo message back to client
  char buf[4096];

  while (true)
  {
    ZeroMemory(buf, 4096);

    // Wait for client to send data
    int bytesReceived = recv(client_sock, buf, 4096, 0);
    if (bytesReceived == SOCKET_ERROR)
    {
      throw std::runtime_error("Error in recv(). Quitting");
      std::cerr << "Error in recv(). Quitting" << std::endl;
      break;
    }

    if (bytesReceived == 0)
    {
      std::cout << "Client disconnected " << std::endl;
      break;
    }

    std::cout << std::string(buf, 0, bytesReceived) << std::endl;

    // Echo message back to client
    //send(clientSocket, buf, bytesReceived + 1, 0);

  }
  return 0;
}

void Server::SetStopWaiting(bool b)
{
  stop_waiting = b;
}

int Server::CreateSocket()
{
  // Create a socket
  //std::cout << "Creating listen socket on server" << std::endl;
  listen_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_sock == INVALID_SOCKET)
  {
    std::ostringstream error;
    error << "Create listen socket on server failed, Err #" << WSAGetLastError();
    throw std::runtime_error(error.str());
  }
  return 0;
}

int Server::BindSocket()
{
  // Bind the ip address and port to a socket
  //std::cout << "Binding listen socket to local IP" << std::endl;
  sockaddr_in hint;
  hint.sin_family = AF_INET;
  hint.sin_port = htons(port);
  hint.sin_addr.S_un.S_addr = INADDR_ANY; // Could also use inet_pton .... 

  if (bind(listen_sock, (sockaddr*)&hint, sizeof(hint)) == SOCKET_ERROR)
  {
    std::ostringstream error;
    error << "Could not bind listen_socket, Err #" << WSAGetLastError();
    throw std::runtime_error(error.str());
  }
  return 0;
}

int Server::StartListen()
{
  // Tell Winsock the socket is for listening 
  if (listen(listen_sock, SOMAXCONN) == SOCKET_ERROR)
  {
    std::ostringstream error;
    error << "Listen failed with error #" << WSAGetLastError();
    throw std::runtime_error(error.str());
  }
  return 0;
}

int Server::StopListen()
{
  // Close listening socket
  closesocket(listen_sock);
  return 0;
}
