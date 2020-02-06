/*
 * File Name: main.cpp
 * Program Name: dg_ad_A01_NAT
 * Programmers: Amy Dayasundara, Daniel Grew
 * First Version: 2020-02-04
 * Description: 
 *		This main contains the execution of the client and the server
 *		sending packets between the network. It should calculate
 *		the number of packets to the server and the server will respond 
 *		with the exact number that was recieved
 */

 //This fails at Line 200. Reason:
 //
 //There is an issue with the object for connecting - probable assumption
 //for not recieving information is that there is a direct loss of the packets
 //in the server
 //What we could have improved on is separating separating the Client and
 //Server into appropriate while loops rather than having them weave within
 //one loop. 


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

int calculateNumPackets(FILE* file);

int main(int argc, char* argv[])
{
	enum Mode
	{
		Client,
		Server
	};

	Mode mode = Server;					// initially set the mode to server
	Address address;					// used to store the IP address

	int packetsSent = 0;				// used to kep track of the number of packets sent

	char fileName[fileNameSize] = "";

	if (argc == 3)			// check for arguments 
	{
		int a, b, c, d;

		if ((sscanf_s(argv[1], "%d.%d.%d.%d", &a, &b, &c, &d)) != 0)		// read in the specified IP Address
		{
			mode = Client;
			address = Address(a, b, c, d, ServerPort);
		}
		strcpy(fileName, argv[2]);
	}


	else if (argc > 3)		// check if the user entered too many values
	{
		printf("Too many commands in the command prompt \n");
	}

	if (!InitializeSockets())			// initialize the sockets to use 
	{
		printf("failed to initialize sockets\n");
		return error;
	}

	ReliableConnection connection(ProtocolId, TimeOut);

	const int port = mode == Server ? ServerPort : ClientPort;			// determine the port based on the current user's mode

	if (!connection.Start(port))						// check if we can connect over the port
	{
		printf("could not start connection on port %d\n", port);
		return error;
	}

	// Client connected 
	if (mode == Client)
		connection.Connect(address);		// connect to the specified address
	//Set the server to listen
	else
		connection.Listen();				// set connection state to Listening


	bool connected = false;
	float sendAccumulator = 0.0f;
	float statsAccumulator = 0.0f;

	FlowControl flowControl;
	FILE* readFile = NULL;					// file used by client
	FILE* writeFile = NULL;					// file used by server

	int expectedPackets = 0;
	
	//Client connected and opens the file that should be read
	if (mode == Client)
	{
		if ((readFile = fopen(fileName, readBin)) == NULL)				// check if file exists - if yes - open for writing 
		{
			printf("file does not exist \n");
			return error;
		}
	}

	//Keep running until server and client are forced closed
	while (true)				// constantly loop through send and recieve functionality
	{
		// gets the state of the connection of the connection object 
		if (connection.IsConnected())
			flowControl.Update(DeltaTime, connection.GetReliabilitySystem().GetRoundTripTime() * 1000.0f);

		const float sendRate = flowControl.GetSendRate();
	
		if (mode == Server && connected && !connection.IsConnected())
		{
			flowControl.Reset();
			printf("reset flow control\n");
			connected = false;
		}

		if (!connected && connection.IsConnected())
		{
			printf("client connected to server\n");
			connected = true;
		}

		if (!connected && connection.ConnectFailed())			// check if the connection has failed
		{
			printf("connection failed\n");
			break;
		}

		sendAccumulator += DeltaTime;

		if (mode == Client && packetsSent == 0)				// if we are the client, send the fileName to the server as the initial packet
		{
			// ================================
			// Construct Initial Packet
			// ================================
			char packet[sizeof(fileName)];

			char initialPacket[PacketSize] = "";
			sprintf(initialPacket, "%s-%d", fileName, calculateNumPackets(readFile));		// create initial

			// ================================

			strcpy(packet, initialPacket);		// copy the initial packet to the packet being sent
			connection.SendPacket((const unsigned char*)packet, sizeof(fileName));			// send the initial packet with the file name
			packetsSent++;
		}

		//Continually loop the accumulator for the client to send the packet to the 
		//server. The Client would read char 
		while (sendAccumulator > 1.0f / sendRate)				// send a packet 
		{
			char packet[PacketSize] = { 0 };

			if (mode == Client)				// if we are the client - read the contents of the file into a buffer
			{
				fread(packet, sizeof(char), PacketSize * sizeof(char), readFile);
			}
			connection.SendPacket((const unsigned char*)packet, sizeof(packet));		// send packet to server
			packetsSent++;
			sendAccumulator -= 1.0f / sendRate;
			break;
		}

		while (true) //Having issues receiving the packets from the client side
		{
			unsigned char packet[PacketSize];

			int bytes_read = connection.ReceivePacket(packet, sizeof(packet));

			if (bytes_read != 0)
			{
				printf("rec: %s \n", packet);
				if (mode == Server)							// check if we are 
				{
					if (packetsSent == initialPacketNum)				// check if we are recieving the file containing the fileName and file details
					{
						sscanf((char* )packet, "%s-%d", fileName, &expectedPackets);

						if ((writeFile = fopen(fileName, writeBin)) == NULL)			// check if we can open the file
						{
							printf("Could not write to file \n");
							break;
						}
						++expectedPackets;
						++packetsSent;
						break;
					}


					fwrite(packet, sizeof(char), PacketSize * sizeof(char), writeFile);			// write the file to the newly created file
					if (bytes_read != 0)
					{
						break;
					}
					expectedPackets++;
				}
			}
			else
			{
				break;
			}
		}

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
		connection.Update(DeltaTime);
		statsAccumulator += DeltaTime;

		while (statsAccumulator >= 0.25f && connection.IsConnected())
		{
			statsAccumulator -= 0.25f;
		}
		net::wait(DeltaTime);

		if (mode == Server && connected)				// check if we have recieved the last packet
		{
			if (packetsSent >= expectedPackets)
			{
				if (writeFile != NULL)
				{
					fclose(writeFile);
				}
				break;
			}
		}
	}
	if (mode == Server)
	{
		fclose(writeFile);
	}

	connection.Stop();
	net::ShutdownSockets();
	return 0;
}

/*
Function: calculateNumPackets
Parameters: FILE* file - send file analyse the number of bytes in the file
Description:
		Calculates the number of packets required to transfer a file
*/
int calculateNumPackets(FILE* file)
{
	fseek(file, 0L, SEEK_END);

	long int fileSize = ftell(file);				// get the size of the file

	rewind(file);			// go back to the beginning of the file

	int numPackets = 0;
	numPackets = (fileSize - (fileSize & PacketSize)) / PacketSize;

	return numPackets;
}