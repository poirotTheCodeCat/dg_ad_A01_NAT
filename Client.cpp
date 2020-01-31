/*
*	FILE			: Client.cpp
*	PROJECT			: SENG204 - Network Application Development
*	PROGRAMMER		: Amy Dayasundara, Daniel Grew
*	FIRST VERSION	: 2020 - 01 -28
*	DESCRIPTION		:
*		This contains the client side information where the file is 
*		opened, parsed and formed into packets. It will send how many 
*		packet are to be read and will cross reference to make sure each
*		packet is properly sent. 
*/
#include <stdio.h>
#pragma warning(disable: 4996)

using namespace std;

void clientLove()
{
	printf("Hello to you too love.\n");
}


char* dataFromFile(char* fileName)
{
	char data[1024] = { 0 };
	FILE* fileExtraction = NULL;
	int numbBytesRead = 0;
	char dataFromFile[1024] = { 0 };

	fileExtraction = fopen(fileName, "rb");

	if (fileExtraction == NULL)
	{
		printf("Issue with file");
	}
	else
	{
		while (!feof(fileExtraction))
		{
			if ((numbBytesRead = fread(dataFromFile, sizeof(char), 1024, fileExtraction)) != 0)
			{
				//Convert the png to Binary(?)
			}
		}
		if (fclose(fileExtraction) != 0)
		{
			printf("Error for closing file.\n");
		}
	}
	
	return data;
}