#include <stdio.h>
#include <string.h>

int main(int argc, char* argv[])
{
    static unsigned char arr1[5];
    static unsigned short arr2[5];
    memset(arr1, 1, sizeof(arr1));
    memset(arr2, 255, sizeof(arr2));
    int i = 0;
    for (i = 0; i < 5; i++)
    {
        printf("arr1[%d]=%d\n", i, arr1[i]);
        printf("arr2[%d]=%u\n", i, arr2[i]);
    }

    return 0;
}
