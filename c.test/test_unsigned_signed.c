#include <stdio.h>

int main(int argc, char* argv[])
{
    int a = -2147483647;
    unsigned int b = 1u;
    unsigned c = a - b;
    printf("c=%u a=%u, a=%x\n", c, a, a);
    return 0;
}
