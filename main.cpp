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

class FlowControl
{
public:
	/*
	Function: flowControl()
	Parameters:	None
	Description:	constructor
	returns: Nothing
	*/
	FlowControl()
	{
		printf("flow control initialized\n");
		Reset();
	}

	/*
	Function: Reset()
	Parameters: None
	Description: This function resets all private members in the class to default settings
	returns:
	*/
	void Reset()
	{
		mode = Bad;
		penalty_time = 4.0f;
		good_conditions_time = 0.0f;
		penalty_reduction_accumulator = 0.0f;
	}

	/*
	Function: Update()
	Parameters: float deltaTime, float rtt
	Description: modifies class variables based on the round trip time of a connection
	returns: nothing
	*/
	void Update(float deltaTime, float rtt)	// note: rtt stands for round trip time 
	{
		const float RTT_Threshold = 250.0f;		

		if (mode == Good) // checks if the state of the class variable "mode" is "Good"
		{
			if (rtt > RTT_Threshold)
			{
				printf("*** dropping to bad mode ***\n");
				mode = Bad;
				if (good_conditions_time < 10.0f && penalty_time < 60.0f)
				{
					penalty_time *= 2.0f;
					if (penalty_time > 60.0f)
						penalty_time = 60.0f;
					printf("penalty time increased to %.1f\n", penalty_time);
				}
				good_conditions_time = 0.0f;
				penalty_reduction_accumulator = 0.0f;
				return;
			}

			good_conditions_time += deltaTime;
			penalty_reduction_accumulator += deltaTime;

			if (penalty_reduction_accumulator > 10.0f && penalty_time > 1.0f)
			{
				penalty_time /= 2.0f;
				if (penalty_time < 1.0f)
					penalty_time = 1.0f;
				printf("penalty time reduced to %.1f\n", penalty_time);
				penalty_reduction_accumulator = 0.0f;
			}
		}

		if (mode == Bad)
		{
			if (rtt <= RTT_Threshold)
				good_conditions_time += deltaTime;
			else
				good_conditions_time = 0.0f;

			if (good_conditions_time > penalty_time)
			{
				printf("*** upgrading to good mode ***\n");
				good_conditions_time = 0.0f;
				penalty_reduction_accumulator = 0.0f;
				mode = Good;
				return;
			}
		}
	}

	/*
	Function:
	Parameters:
	Description:
	returns:
	*/
	float GetSendRate()
	{
		return mode == Good ? 30.0f : 10.0f;
	}

private:

	enum Mode
	{
		Good,
		Bad
	};

	Mode mode;
	float penalty_time;
	float good_conditions_time;
	float penalty_reduction_accumulator;
};

// ----------------------------------------------
// ==============================================
// ----------------------------------------------

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
	char message[PacketSize];

	if (argc >= 3)
	{
		int a, b, c, d;
		if (sscanf_s(argv[1], "%d.%d.%d.%d", &a, &b, &c, &d))
		{
			mode = Client;
			address = Address(a, b, c, d, ServerPort);
			sscanf(argv[2], "%s", message);
		}

	}

	// initialize

	if (!InitializeSockets())
	{
		printf("failed to initialize sockets\n");
		return 1;
	}

	ReliableConnection connection(ProtocolId, TimeOut);

	const int port = mode == Server ? ServerPort : ClientPort;			// ? is like if statement - mode==Server -> true: ServerPort | false: ClientPort

	if (!connection.Start(port))		// attempt to start a connection on the specified port | if false enter IF statement 
	{
		printf("could not start connection on port %d\n", port);
		return 1;
	}

	if (mode == Client)		// if we are in client mode then attempt to connect to a specified IP address
		connection.Connect(address);
	else
		connection.Listen();		// if it is a server, then listen for a connection

	bool connected = false;
	float sendAccumulator = 0.0f;
	float statsAccumulator = 0.0f;

	FlowControl flowControl;		// initialize a FlowControl Object

	while (true)
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
			strcpy((char*)packet, message);
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
				printf("%s", packet);
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

		/*
		while ( statsAccumulator >= 0.25f && connection.IsConnected() )		// continuously listens to client
		{
			float rtt = connection.GetReliabilitySystem().GetRoundTripTime();

			unsigned int sent_packets = connection.GetReliabilitySystem().GetSentPackets();
			unsigned int acked_packets = connection.GetReliabilitySystem().GetAckedPackets();
			unsigned int lost_packets = connection.GetReliabilitySystem().GetLostPackets();

			float sent_bandwidth = connection.GetReliabilitySystem().GetSentBandwidth();
			float acked_bandwidth = connection.GetReliabilitySystem().GetAckedBandwidth();

			printf( "rtt %.1fms, sent %d, acked %d, lost %d (%.1f%%), sent bandwidth = %.1fkbps, acked bandwidth = %.1fkbps\n",
				rtt * 1000.0f, sent_packets, acked_packets, lost_packets,
				sent_packets > 0.0f ? (float) lost_packets / (float) sent_packets * 100.0f : 0.0f,
				sent_bandwidth, acked_bandwidth );		// prints to screen

			statsAccumulator -= 0.25f;
		}
		*/
		net::wait(DeltaTime);
	}

	ShutdownSockets();			// built in socket function

	return 0;
}
