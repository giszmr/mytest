#include <stdio.h>

const int val = 10;

int main(int argc, char* argv[])
{
    int val1 = val;
    int val2 = val;
    int valList[] = {0,1,2,3,4,5,6};

    printf("sizeof valList %d\n", sizeof(valList)/sizeof(int));

    printf("val1[%p][%d],\nval2[%p][%d]\n", &val1, val1, &val2, val2);
    return 0;
}
