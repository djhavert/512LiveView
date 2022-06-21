#include "Client.h"

#include "Application.h"

Client::Client()
{
	try
	{
		InitializeWinsock();
		CreateSocket();
	}
	catch (std::runtime_error e)
	{
		throw e;
	}
}

Client::~Client()
{
	Cleanup();
}

void Client::Cleanup()
{
	delete[] buffer;
	delete[] buffer_overflow;
	closesocket(sock);
	WSACleanup();
}

int Client::Connect(std::string address, int port)
{
	// Save address and port info
	Client::server_ipAddress = address;
	Client::server_port = port;
	Client::buffer = new char[MAX_BUFFER_SIZE];
	Client::buffer_overflow = new char[BYTES_PER_SAMPLE];
	
	// Fill in a hint structure
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(server_port);
	inet_pton(AF_INET, server_ipAddress.c_str(), &hint.sin_addr);

	do {
		//std::cout << "Connecting to " << server_ipAddress << ":" << server_port << " ..." << std::endl;
		int connResult = connect(sock, (sockaddr*)&hint, sizeof(hint));
		// If connected successfully, break out of loop
		if (connResult == 0)
		{
			break;
		}
		// Continue trying to connect if timed out or refused
		int wsa_error = WSAGetLastError();
		if (wsa_error == WSAETIMEDOUT || wsa_error == WSAECONNREFUSED)
		{
			continue;
		}
		else
		{
			// Print error and cleanup if any other error
			std::ostringstream error;
			error << "Can't connect to server, Err #" << WSAGetLastError();
			closesocket(sock);
			WSACleanup();
			throw std::runtime_error(error.str());
		}

	} while (1);
	std::cout << "Successfully connected to " << server_ipAddress << ":" << server_port << std::endl;
	return 0;
}

int Client::Send(const char* send_buf)
{
	int send_result = send(sock, send_buf, (int)strlen(send_buf), 0);
	if (send_result == SOCKET_ERROR)
	{
		std::ostringstream error;
		error << "Client send confirmation to LabView failed, Err #" << WSAGetLastError();
		throw std::runtime_error(error.str());
	}
	return 0;
}

int Client::Receive()
{
	ZeroMemory(buffer, MAX_BUFFER_SIZE);
	if (overflow_bytes != 0)
	{
		// Copy overflow_buffer into the beginning of buffer
		std::copy(buffer_overflow, buffer_overflow + overflow_bytes, buffer);
	}
	// Receive data into buffer beginning at end of overflow buffer
	bytes_received = recv(sock, buffer + overflow_bytes, MAX_BUFFER_SIZE - overflow_bytes, 0);
	buffer_size = bytes_received + overflow_bytes;
	return bytes_received;
}

int Client::ParseData()
{
	if (!b_header_is_parsed)
	{
		ParseHeader();
		b_header_is_parsed = true;
	}
	else
	{
		// calculate number of complete samples (includes data for all channels)
		// overflow_bytes is number of bytes remaining
		int samples = buffer_size / BYTES_PER_SAMPLE;
		overflow_bytes = buffer_size % BYTES_PER_SAMPLE;
		// for every complete sample in the buffer
		for (int sample = 0; sample < samples; sample++)
		{
			Unpack(sample);
			samples_parsed++;
			/*
			data_sample_index++;
			// perhaps do a consant analysis
			if (data_sample_index == SAMPLE_RATE) // Analysis every 1 second
			{
				//data.WriteToFile();
				AnalyzeData();
				data_sample_index = 0;
				std::cout << samples_parsed / SAMPLE_RATE << " seconds parsed." << '\n';
			}
			*/
		}
		// Analyze newly inputted data
		//Application::Get().AnalyzeData();
		// Copy overflow bytes into overflow buffer.
		if (overflow_bytes != 0)
		{
			ZeroMemory(buffer_overflow, BYTES_PER_SAMPLE);
			std::copy(buffer + buffer_size - overflow_bytes, buffer + buffer_size, buffer_overflow);
		}
	}
	return 0;
}

void Client::SetStopReceiving(bool b)
{
	b_stop_receiving = b;
}


char* Client::getBuffer()
{
	return buffer;
}

int Client::InitializeWinsock()
{
	//std::cout << "Initializing Winsock" << std::endl;
	WSAData data;
	WORD ver = MAKEWORD(2, 2);
	int wsResult = WSAStartup(ver, &data);
	if (wsResult != 0)
	{
		std::ostringstream error;
		error << "Can't start Winsock, Err #" << wsResult;
		throw std::runtime_error(error.str());
	}
	//std::cout << "Winsock Initialized successfully." << std::endl;
	return 0;
}

int Client::CreateSocket()
{
	std::cout << "Creating client socket" << std::endl;
	Client::sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
	{
		std::ostringstream error;
		error << "Can't create socket, Err #" << WSAGetLastError();
		WSACleanup();
		throw std::runtime_error(error.str());
	}
	return 0;
}

int Client::ParseHeader()
{
	//std::cout << buffer << '\n';
	return 0;
}

int Client::Unpack(int sample_num)
{
	/* Unpack and store data */
	// Initialize variables
	int offset = sample_num * BYTES_PER_SAMPLE;
	int byte = 2 + offset;
	int byte_end = offset + BYTES_PER_SAMPLE;
	char b1, b2, b3;
	short int s1, s2;
	while (byte < byte_end)
	{
		// unpack three bytes (chars) into two int16 channel voltages.
		b1 = buffer[byte];
		b2 = buffer[byte + 1];
		b3 = buffer[byte + 2];
		s1 = ((b1 << 4) & 0x0FF0) + ((b2 >> 4) & 0x000F) - 2048;
		s2 = ((b2 << 8) & 0x0F00) + (b3 & 0x00FF) - 2048;
		// store voltages for both channels
		Application::Get().SetRawData((byte - offset) / 3 * 2, samples_parsed, s1);
		Application::Get().SetRawData((byte - offset) / 3 * 2 + 1, samples_parsed, s2);
		//data->set((byte-offset) / 3 * 2, data_sample_index, s1);
		//data->set((byte-offset) / 3 * 2 + 1, data_sample_index, s2);
		// next set of 3 bytes
		byte = byte + 3;
	}
	return 0;
}

int Client::AnalyzeData()
{
	Application::Get().AnalyzeData();
	return 0;
}
