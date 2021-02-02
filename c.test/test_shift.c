#include <stdio.h>

int func1(unsigned word)
{
    return (int) ((word << 24) >> 24);
}

int func2(unsigned word)
{
    return ((int)word << 24) >> 24;
}

int main(int argc, char* argv[])
{
    int a = 0xc9;
    printf("a = 0x%x\n", a);
    printf("a << 24 = 0x%x\n", a << 24);
    printf("func1 0x%x\n", func1(a));
    printf("func2 0x%x\n", func2(a));
    return 0;
}
