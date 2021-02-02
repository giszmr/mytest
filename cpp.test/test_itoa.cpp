#include <stdio.h>
#include <cstdlib>
#include <stdlib.h>


int main(int argc, char* argv[])
{
    int a = 10;
    char str[10] = {0};

    itoa(a, str, 10);

    printf("str=%s\n", str);
    return 0;

}
