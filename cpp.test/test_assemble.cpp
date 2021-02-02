#include <cstdio>


int example(int arg1, int arg2, int arg3, int arg4, int arg5, char arg6, double arg7)
{

    return arg1+arg2;
}

int eg_if(int x)
{
    if(x < 0)
        return 0;
    else
        return 1;
}

int eg_for_sum(int len)
{
    int i;
    int sum  = 0;

    do{
        if(len <= 0)
            break;
        for(i = 0; i < len; i++)
            sum++;
    }while(0);
    return sum;
}

int eg_switch(int x)
{
    int ret = 0;
    switch(x)
    {
        case -10:
            ret = -1;
            break;
        case 0:
            ret = 0;
            break;
        case 10:
            ret = 1;
            break;
        default:
            ret = 0xFFFFFFFF;
            break;
    }
    return ret;
}


int main(int argc, char* argv[])
{
    int xx = 10;
    int ret = 0;

    ret = eg_if(xx);
    printf("ret = %d\n", ret);

    ret = eg_for_sum(xx);
    printf("ret = %d\n", ret);

    ret = eg_switch(xx);
    printf("ret = %d\n", ret);

    ret = example(1, 2, 3, 4, 5, 6, 7.0);

    return 0;
}
