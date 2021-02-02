#include <stdio.h>

struct led_descriptor {
	unsigned char initial_state;
	unsigned char instance;
	char *led_name;
};

int main(int argc, char* argv[])
{
	//printf("hello\n");
	struct led_descriptor led_descriptors[] = {
		{
			.initial_state = 0,
			.led_name = "first",
			.instance = 0,
		},
		{
			.initial_state = 1,
			.led_name = "second",
		}

	};

	int i = 0;

	for ( i = 0; i < 2; i++)
	{
		printf("name = %s, state = %d, instance = %d\n", 
			led_descriptors[i].led_name, 
			led_descriptors[i].initial_state, 
			led_descriptors[i].instance);
	}

	//printf("finished\n");
	return 0;
}

