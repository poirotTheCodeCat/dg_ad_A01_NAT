#pragma once
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
			if (rtt > RTT_Threshold)			// check if the round trip time is greater than threshold 
			{
				printf("*** dropping to bad mode ***\n");
				mode = Bad;						// change the mode to bad 
				if (good_conditions_time < 10.0f && penalty_time < 60.0f)
				{
					penalty_time *= 2.0f;
					if (penalty_time > 60.0f)
						penalty_time = 60.0f;
					printf("penalty time increased to %.1f\n", penalty_time);		// increase the "penalty time"
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