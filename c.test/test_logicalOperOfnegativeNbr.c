#include <stdio.h>

int main(int argc, char*argv[])
{
    int a = -1;
    int b = -2;
    int c = 1;
    int d = 0;

    if(a && b && c)
        printf("true\n");
    else
        printf("false\n");
    if (d)
        printf("!0\n");
    else
        printf("0\n");
    return 0;
}
