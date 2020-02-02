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


int main(int argc, char* argv[])
{
	// parse command line

	enum Mode
	{
		Client,
		Server
	};

	Mode mode = Server;
	Address address;
	char fileName[PacketSize];

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

	//// check if the file exists
	//FILE* file = NULL;
	//file = fopen(fileName, readBin);		// open file as binary file
	//if (file == NULL)					// check if file exists
	//{
	//	printf("The file %s does not exist \n", fileName);
	//	return error;
	//}

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
		connection.Connect(address);
	else
		connection.Listen();		// if it is a server, then listen for a connection

	bool connected = false;
	float sendAccumulator = 0.0f;
	float statsAccumulator = 0.0f;

	FlowControl flowControl;		// initialize a FlowControl Object

	while (true)	//while not at the end of file		
	{
		// update flow control

		if (connection.IsConnected())		// check if we are connected 
			flowControl.Update(DeltaTime, connection.GetReliabilitySystem().GetRoundTripTime() * 1000.0f);		// if we are connected then update with a constant time && round trip time 

		const float sendRate = flowControl.GetSendRate();		// set sendRate based on the flowControl state (good || bad)

		// detect changes in connection state

		if (mode == Server && connected && !connection.IsConnected())	// connected should initially be false  
		{
			flowControl.Reset();
			printf("reset flow control\n");
			connected = false;
		}

		if (!connected && connection.IsConnected())			// check if the client is connected to the server
		{
			printf("client connected to server\n");
			connected = true;					// change the status of connected to reflect the state of the connection | we are connected so set connected=true
		}

		if (!connected && connection.ConnectFailed())		// check if the connection has been broken
		{
			printf("connection failed\n");
			break;
		}

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
			unsigned char packet[256];
			int bytes_read = connection.ReceivePacket(packet, sizeof(packet));		// while true constantly recieve 
			if (bytes_read == 0)		// if nothin is read, return to outer while loop
				break;
			else
			{
				printf("packet is %s \n", packet);
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

		connection.Update(DeltaTime);

		// show connection stats

		statsAccumulator += DeltaTime;

		// reset buffer

		while ( statsAccumulator >= 0.25f && connection.IsConnected())		// continuously listens to server
		{
			float rtt = connection.GetReliabilitySystem().GetRoundTripTime();		// time to complete round trip

			unsigned int sent_packets = connection.GetReliabilitySystem().GetSentPackets();
			unsigned int acked_packets = connection.GetReliabilitySystem().GetAckedPackets();
			unsigned int lost_packets = connection.GetReliabilitySystem().GetLostPackets();

			float sent_bandwidth = connection.GetReliabilitySystem().GetSentBandwidth();
			float acked_bandwidth = connection.GetReliabilitySystem().GetAckedBandwidth();

			printf( "rtt %.1fms, sent %d, acked %d, lost %d (%.1f%%), sent bandwidth = %.1fkbps, acked bandwidth = %.1fkbps\n",
				rtt * 1000.0f, sent_packets, acked_packets, lost_packets,
				sent_packets > 0.0f ? (float) lost_packets / (float) sent_packets * 100.0f : 0.0f,
				sent_bandwidth, acked_bandwidth );								// prints to screen

			statsAccumulator -= 0.25f;
		}
		net::wait(DeltaTime);
	}

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

