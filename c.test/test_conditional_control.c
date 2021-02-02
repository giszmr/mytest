#include <stdio.h>

int main(int argc, char* argv[])
{
    return 0;
}

long absdiff(long x, long y)
{
    long result;
    if(x < y)
        result = y -x;
    else
        result = x - y;
    return result;
}

long switch_test(long x)
{
    switch (x)
    {
        case 1:
            x = 1;
            break;
        case 2:
            x = 2;
            break;
        case 3:
            x = 3;
            break;
        case 4:
            x = 4;
            break;
        case 5:
            x = 5;
            break;
        case 6:
            x = 6;
            break;
        case 10:
            x = 10;
            break;
        default:
            x = -1;
            break;
    }
    return 0;
}

