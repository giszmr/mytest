#include <stdio.h>

int main(int argc, char *argv[])
{
#define HELLO
#define WORLD
#if (defined HELLO)
	printf("hello\n");
#elif (defined (WORLD))
	printf("world\n");
//#else
//	printf("hello world\n");
#endif
	return 0;
}
