#include <stdio.h>
#include "test_derive.hpp"

typedef void (*VFunc)(void);

int main(int argc, char* argv[])
{
   /* Base base(1000);
    VFunc func = (VFunc)*(int*)*(int*)(&base);
    func();  //it works.
    func = (VFunc)*((int*)*(int*)(&base) + 2);
    func();*/


    printf("******************\n");
    Derive derive(2000);
    derive.getValue();
    /*VFunc dfunc = (VFunc)*(int*)*(int*)(&derive);
    dfunc();
    dfunc = (VFunc)*((int*)*(int*)(&derive) + 8);
    dfunc();
    dfunc = (VFunc)*((int*)*(int*)(&derive) + 10);
    dfunc();
    dfunc = (VFunc)*((int*)*(int*)(&derive) + 12);
    dfunc();*/
    printf("******************\n");

    float a = 1.999;
    int b = a;
    float c = -1.999;
    int d = c;
    printf("a=%d b=%d\n", (int)a, b);
    printf("c=%d d=%d\n", (int)c, d);
    return 0;
}
