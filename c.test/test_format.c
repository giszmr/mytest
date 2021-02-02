#include <stdio.h>
#include <stdlib.h>


int main()
{
    unsigned char val = 208;
    int i = 0;
    for(; i < 2; i++) {
        do {
            printf("test1\n");
            break;
            printf("test2\n");
        } while(0);
        printf("test3\n");
    }
    printf("val=%u\n", val);
    printf("hello\n");
    printf("world\n");
    printf("3/4=%d\n", 3/4);
    printf("-3/4=%d\n", -3/4);
    return 0;
}
