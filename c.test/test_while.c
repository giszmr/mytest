#include <stdio.h>
#include <stdlib.h>


int main(int argc, char* argv[])
{
    int a = -10, b = 1, c = 0;
    while(a&a)
    {
        a = a + 1;
        printf("loop a(%d), a&a=%d\n", a, a&a);
    }
    printf("out. a(%d)\n", a);

    
    c = b ? 2 : -2;
    printf("c = %d\n", c);
    return 0;
}
