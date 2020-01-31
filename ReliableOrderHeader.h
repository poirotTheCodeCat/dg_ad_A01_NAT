/*
*	FILE			: ReliableOrderHeader.h
*	PROJECT			: SENG204 - Network Application Development
*	PROGRAMMER		: Amy Dayasundara, Daniel Grew
*	FIRST VERSION	: 2020 - 01 -28
*	DESCRIPTION		:
*		Header that includes the prototypes. This will use the theory from the 
*		website (https://gafferongames.com/post/reliable_ordered_messages/), using
*		the structs for packet level acks
*/

#include "NET.h"
#pragma once
#define BUFFER_SIZE 1024

//STRUCTS

//Packet Level Acks (https://gafferongames.com/post/reliable_ordered_messages/)
struct Header
{
	uint16_t sequence;
	uint16_t ack;
	uint32_t ack_bits;
};

struct PacketData
{
	bool acked;
};
//PROTOTYPES
void test();
void clientLove();
char* dataFromFile(char* fileName);