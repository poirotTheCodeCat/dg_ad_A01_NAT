/*
	Reliability and Flow Control Example
	From "Networking for Game Programmers" - http://www.gaffer.org/networking-for-game-programmers
	Author: Glenn Fiedler <gaffer@gaffer.org>
	Original File Name: example.cpp
*/

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "constants.h"
#include"FlowControl.h"

#include "Net.h"

#pragma warning(disable: 4996)

//#define SHOW_ACKS

using namespace std;
using namespace net;

const int ServerPort = 30000;
const int ClientPort = 30001;
const int ProtocolId = 0x11223344;
const float DeltaTime = 1.0f / 30.0f;
const float SendRate = 1.0f / 30.0f;
const float TimeOut = 10.0f;
const int PacketSize = 256;

struct packetStruct
{
	int packetNum;
	int totalPackets;
	char packetContent;
};

enum Mode
{
	Client,
	Server
};

void clearBuffer(char* buffer);
bool server(ReliableConnection connection);
bool client(ReliableConnection connection, char* fileName);

int main(int argc, char* argv[])
{
	// parse command line


	Mode mode = Server;
	Address address;
	char fileName[PacketSize] = "";
	char readBuffer[PacketSize] = "";
	char writeBuffer[PacketSize] = "";

	if (argc >= 3)
	{
		int a, b, c, d;
		if (sscanf_s(argv[1], "%d.%d.%d.%d", &a, &b, &c, &d))
		{
			mode = Client;
			address = Address(a, b, c, d, ServerPort);
			sscanf(argv[2], "%s", fileName);
		}
	}

	FILE* readFile = NULL;		// this will be read from if we are the client
	FILE* writeFile = NULL;		// this will be written to if we are the server

	// initialize

	if (!InitializeSockets())
	{
		printf("failed to initialize sockets\n");
		return error;
	}

	ReliableConnection connection(ProtocolId, TimeOut);

	const int port = mode == Server ? ServerPort : ClientPort;			// ? is like if statement - mode==Server -> true: ServerPort | false: ClientPort

	if (!connection.Start(port))		// starts as true - starts connection on specific port
	{
		printf("could not start connection on port %d\n", port);
		return error;
	}

	if (mode == Client)		// if we are in client mode then attempt to connect to a specified IP address
	{
		connection.Connect(address);
		client(connection, fileName);
	}
	else
	{
		connection.Listen();		// if it is a server, then listen for a connection
		server(connection);
	}

//	bool connected = false;
//	float sendAccumulator = 0.0f;
//	float statsAccumulator = 0.0f;
//
//	FlowControl flowControl;		// initialize a FlowControl Object
//
//
//	while (true)	//while not at the end of file		
//	{
//		// update flow control
//
//		if (connection.IsConnected())		// check if we are connected 
//			flowControl.Update(DeltaTime, connection.GetReliabilitySystem().GetRoundTripTime() * 1000.0f);		// if we are connected then update with a constant time && round trip time 
//
//		const float sendRate = flowControl.GetSendRate();		// set sendRate based on the flowControl state (good || bad)
//
//		// detect changes in connection state
//
//		if (mode == Server && connected && !connection.IsConnected())	// connected should initially be false  
//		{
//			flowControl.Reset();
//			printf("reset flow control\n");
//			connected = false;
//		}
//
//		if (!connected && connection.IsConnected())			// check if the client is connected to the server
//		{
//			printf("client connected to server\n");
//			connected = true;					// change the status of connected to reflect the state of the connection | we are connected so set connected=true
//		}
//
//		if (!connected && connection.ConnectFailed())		// check if the connection has been broken
//		{
//			printf("connection failed\n");
//			break;
//		}
//
//		// send packet containing fileName
//
//		// send and receive packets
//
//		sendAccumulator += DeltaTime;		// increase sendAccumulator every time while executes
//
//		while (sendAccumulator > 1.0f / sendRate)		// sendrate is a constant | send until sendAccumulator runs out
//		{
//			unsigned char packet[PacketSize];
//			strcpy((char*)packet, fileName);
//			connection.SendPacket(packet, sizeof(packet));
//			sendAccumulator -= 1.0f / sendRate;		// subtracts from sendAccumulator every time a packet is sent
//		}
//
//		while (true)
//		{
//			unsigned char packet[256];
//			int bytes_read = connection.ReceivePacket(packet, sizeof(packet));		// while true constantly recieve 
//			if (bytes_read == 0)							// if nothin is read, return to outer while loop
//			{
//				break;
//			}
//			else
//			{
//				 printf("Recieved: %s \n", packet);
//			}
//		}
//
//		// show packets that were acked this frame
//
//#ifdef SHOW_ACKS
//		unsigned int* acks = NULL;
//		int ack_count = 0;
//		connection.GetReliabilitySystem().GetAcks(&acks, ack_count);
//		if (ack_count > 0)
//		{
//			printf("acks: %d", acks[0]);
//			for (int i = 1; i < ack_count; ++i)
//				printf(",%d", acks[i]);
//			printf("\n");
//		}
//#endif
//
//		// update connection
//
//		connection.Update(DeltaTime);		// use this to determine if the connection has timed out
//
//		// show connection stats
//
//		statsAccumulator += DeltaTime;
//
//		// reset buffer
//
//		while ( statsAccumulator >= 0.25f && connection.IsConnected())		// continuously listens to server
//		{
//			float rtt = connection.GetReliabilitySystem().GetRoundTripTime();		// time to complete round trip
//
//			unsigned int sent_packets = connection.GetReliabilitySystem().GetSentPackets();
//			unsigned int acked_packets = connection.GetReliabilitySystem().GetAckedPackets();
//			unsigned int lost_packets = connection.GetReliabilitySystem().GetLostPackets();
//
//			float sent_bandwidth = connection.GetReliabilitySystem().GetSentBandwidth();
//			float acked_bandwidth = connection.GetReliabilitySystem().GetAckedBandwidth();
//
//			printf( "rtt %.1fms, sent %d, acked %d, lost %d (%.1f%%), sent bandwidth = %.1fkbps, acked bandwidth = %.1fkbps\n",
//				rtt * 1000.0f, sent_packets, acked_packets, lost_packets,
//				sent_packets > 0.0f ? (float) lost_packets / (float) sent_packets * 100.0f : 0.0f,
//				sent_bandwidth, acked_bandwidth );								// prints to screen
//
//			statsAccumulator -= 0.25f;
//		}
//		net::wait(DeltaTime);
//
//
//	}

	ShutdownSockets();			// built in socket function

	return 0;
}


// =================================================================
// =================================================================
//			This can go in a new .cpp file
//	These are functions not included in the original
//					example file
// =================================================================
// =================================================================

/*
Function:
Parameters:
Description:
Returns:
*/
bool server(ReliableConnection connection)
{
	Mode mode = Client;
	bool fileNameRecieved = false;
	bool connected = false;
	float sendAccumulator = 0.0f;
	float statsAccumulator = 0.0f;
	char RecieveLine[PacketSize] = "";
	char fileName[PacketSize] = "";

	FlowControl flowControl;		// initialize a FlowControl Object

	// wait to recieve first packet containing fileName
	while (!fileNameRecieved)	//while not at the end of file		
	{
		// update flow control

		if (connection.IsConnected())		// check if we are connected 
			flowControl.Update(DeltaTime, connection.GetReliabilitySystem().GetRoundTripTime() * 1000.0f);		// if we are connected then update with a constant time && round trip time 

		const float sendRate = flowControl.GetSendRate();		// set sendRate based on the flowControl state (good || bad)

		if (!connected && connection.IsConnected())			// check if the client is connected to the server
		{
			printf("client connected to server\n");
			connected = true;					// change the status of connected to reflect the state of the connection | we are connected so set connected=true
		}

		if (!connected && connection.ConnectFailed())		// check if the connection has been broken
		{
			printf("connection failed\n");
			return false;
		}

		// send packet containing fileName

		// send and receive packets

		while (true)
		{
			unsigned char packet[256];
			int bytes_read = connection.ReceivePacket(packet, sizeof(packet));		// while true constantly recieve 
			if (bytes_read != 0)							// if nothin is read, return to outer while loop
			{
				printf("Recieved: %s \n", packet);
				fileNameRecieved = true;
				strcpy(fileName, (char* )packet);
				break;
			}
		}

		sendAccumulator += DeltaTime;		// increase sendAccumulator every time while executes

		while (sendAccumulator > 1.0f / sendRate)		// sendrate is a constant | send until sendAccumulator runs out
		{
			unsigned char packet[PacketSize];
			strcpy((char*)packet, fileName);
			connection.SendPacket(packet, sizeof(packet));
			sendAccumulator -= 1.0f / sendRate;		// subtracts from sendAccumulator every time a packet is sent
		}

		// show packets that were acked this frame

#ifdef SHOW_ACKS
		unsigned int* acks = NULL;
		int ack_count = 0;
		connection.GetReliabilitySystem().GetAcks(&acks, ack_count);
		if (ack_count > 0)
		{
			printf("acks: %d", acks[0]);
			for (int i = 1; i < ack_count; ++i)
				printf(",%d", acks[i]);
			printf("\n");
		}
#endif

		// update connection

		connection.Update(DeltaTime);		// use this to determine if the connection has timed out
	}

	FILE* writeFile = NULL;
	if ((writeFile = fopen(fileName, writeBin)) == NULL)
	{
		printf("could not open %s to write \n", fileName);
		return false;
	}

	sendAccumulator = 0.0f;	// reset the sendAccumulator
	// Recieve Lines to write

	while (true)
	{
		// update flow control

		if (connection.IsConnected())		// check if we are connected 
			flowControl.Update(DeltaTime, connection.GetReliabilitySystem().GetRoundTripTime() * 1000.0f);		// if we are connected then update with a constant time && round trip time 

		const float sendRate = flowControl.GetSendRate();		// set sendRate based on the flowControl state (good || bad)

		if (!connected && connection.IsConnected())			// check if the client is connected to the server
		{
			printf("client connected to server\n");
			connected = true;					// change the status of connected to reflect the state of the connection | we are connected so set connected=true
		}

		if (!connected && connection.ConnectFailed())		// check if the connection has been broken
		{
			printf("connection failed\n");
			return false;
		}

		// send packet containing fileName

		// send and receive packets

		sendAccumulator += DeltaTime;		// increase sendAccumulator every time while executes

		while (sendAccumulator > 1.0f / sendRate)		// sendrate is a constant | send until sendAccumulator runs out
		{
			unsigned char packet[PacketSize];
			strcpy((char*)packet, fileName);
			connection.SendPacket(packet, sizeof(packet));
			sendAccumulator -= 1.0f / sendRate;		// subtracts from sendAccumulator every time a packet is sent
		}

		while (true)
		{
			unsigned char packet[PacketSize];
			int bytes_read = connection.ReceivePacket(packet, sizeof(packet));		// while true constantly recieve 
			if (bytes_read != 0)							// if nothin is read, return to outer while loop
			{
				printf("Recieved: %s \n", packet);
				strcpy(RecieveLine, (char*)packet);
				break;
			}
		}

		if (strcmp(RecieveLine, "") != 0)
		{
			fwrite(RecieveLine, sizeof(char), PacketSize, writeFile);
		}

		// show packets that were acked this frame

#ifdef SHOW_ACKS
		unsigned int* acks = NULL;
		int ack_count = 0;
		connection.GetReliabilitySystem().GetAcks(&acks, ack_count);
		if (ack_count > 0)
		{
			printf("acks: %d", acks[0]);
			for (int i = 1; i < ack_count; ++i)
				printf(",%d", acks[i]);
			printf("\n");
		}
#endif

		// update connection

		connection.Update(DeltaTime);		// use this to determine if the connection has timed out
		clearBuffer(RecieveLine);
	}

	fclose(writeFile);
	return true;
}

/*
Function: 
Parameters: 
Description:
Returns: 
*/
bool client(ReliableConnection connection, char* fileName)
{
	Mode mode = Client;
	bool fileNameSent = false;
	bool connected = false;
	float sendAccumulator = 0.0f;
	float statsAccumulator = 0.0f;
	char sendLine[PacketSize] = "";

	FlowControl flowControl;		// initialize a FlowControl Object

	FILE* readFile = NULL;
	if ((readFile = fopen(fileName, readBin)) == NULL)
	{
		printf("%s does not exist \n", fileName);
		return false;
	}

	// Send fileName first
	while (!fileNameSent)	//while not at the end of file		
	{
		// update flow control

		if (connection.IsConnected())		// check if we are connected 
			flowControl.Update(DeltaTime, connection.GetReliabilitySystem().GetRoundTripTime() * 1000.0f);		// if we are connected then update with a constant time && round trip time 

		const float sendRate = flowControl.GetSendRate();		// set sendRate based on the flowControl state (good || bad)

		if (!connected && connection.IsConnected())			// check if the client is connected to the server
		{
			printf("client connected to server\n");
			connected = true;					// change the status of connected to reflect the state of the connection | we are connected so set connected=true
		}

		if (!connected && connection.ConnectFailed())		// check if the connection has been broken
		{
			printf("connection failed\n");
			return false;
		}

		// send packet containing fileName

		// send and receive packets

		sendAccumulator += DeltaTime;		// increase sendAccumulator every time while executes

		while (sendAccumulator > 1.0f / sendRate)		// sendrate is a constant | send until sendAccumulator runs out
		{
			unsigned char packet[PacketSize];
			strcpy((char*)packet, fileName);
			connection.SendPacket(packet, sizeof(packet));
			sendAccumulator -= 1.0f / sendRate;		// subtracts from sendAccumulator every time a packet is sent
			printf("Sent: %s", packet);
		}

		while (true)
		{
			unsigned char packet[PacketSize];
			int bytes_read = connection.ReceivePacket(packet, sizeof(packet));		// while true constantly recieve 
			if (bytes_read == 0)							// if nothin is read, return to outer while loop
			{
				break;
			}
			else
			{
				printf("Recieved: %s \n", packet);
				fileNameSent = true;
			}
		}

		// show packets that were acked this frame

#ifdef SHOW_ACKS
		unsigned int* acks = NULL;
		int ack_count = 0;
		connection.GetReliabilitySystem().GetAcks(&acks, ack_count);
		if (ack_count > 0)
		{
			printf("acks: %d", acks[0]);
			for (int i = 1; i < ack_count; ++i)
				printf(",%d", acks[i]);
			printf("\n");
		}
#endif

		// update connection

		connection.Update(DeltaTime);		// use this to determine if the connection has timed out
	}

	// ------------------------------
	// read from file and send content
	while (fread(sendLine, sizeof(char), (PacketSize), readFile) != 0)
	{
		if (connection.IsConnected())		// check if we are connected 
			flowControl.Update(DeltaTime, connection.GetReliabilitySystem().GetRoundTripTime() * 1000.0f);		// if we are connected then update with a constant time && round trip time 

		const float sendRate = flowControl.GetSendRate();		// set sendRate based on the flowControl state (good || bad)

		if (!connected && connection.IsConnected())			// check if the client is connected to the server
		{
			printf("client connected to server\n");
			connected = true;					// change the status of connected to reflect the state of the connection | we are connected so set connected=true
		}

		if (!connected && connection.ConnectFailed())		// check if the connection has been broken
		{
			printf("connection failed\n");
			return false;
		}

		// send packet containing fileName

		// send and receive packets

		sendAccumulator += DeltaTime;		// increase sendAccumulator every time while executes

		while (sendAccumulator > 1.0f / sendRate)		// sendrate is a constant | send until sendAccumulator runs out
		{
			connection.SendPacket((const unsigned char* )sendLine, sizeof(sendLine));
			sendAccumulator -= 1.0f / sendRate;		// subtracts from sendAccumulator every time a packet is sent
			printf("Sent: %s", sendLine);
		}

		while (true)
		{
			unsigned char packet[256];
			int bytes_read = connection.ReceivePacket(packet, sizeof(packet));		// while true constantly recieve 
			if (bytes_read == 0)							// if nothin is read, return to outer while loop
			{
				break;
			}
			else
			{
				printf("Recieved: %s \n", packet);
			}
		}

		// show packets that were acked this frame

#ifdef SHOW_ACKS
		unsigned int* acks = NULL;
		int ack_count = 0;
		connection.GetReliabilitySystem().GetAcks(&acks, ack_count);
		if (ack_count > 0)
		{
			printf("acks: %d", acks[0]);
			for (int i = 1; i < ack_count; ++i)
				printf(",%d", acks[i]);
			printf("\n");
		}
#endif

		// update connection

		connection.Update(DeltaTime);		// use this to determine if the connection has timed out
		clearBuffer(sendLine);
	}

	fclose(readFile);		// close the file for reading

	return true;
}

void clearBuffer(char* buffer)
{
	for (size_t i = 0; i < strlen(buffer); i++)
	{
		buffer[i] = '\0';
	}
}


