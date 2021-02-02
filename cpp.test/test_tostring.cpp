#include <stdio.h>
#include <iostream>
#include <sstream>

int main(int argc, char* argv[])
{
    int a = 11;
    std::stringstream stream;
    stream << a;
    printf("value[%s]\n", stream.str().c_str());
    int b = 22;
    stream.str("");
    stream << b;
    printf("value[%s]\n", stream.str().c_str());
    return 0;
}
