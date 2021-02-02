#include <stdio.h>
#include <sstream>
#include <cstring>


int main(int argc, char* argv[])
{
    char cmd[5][30] = {{0}};
    std::stringstream ss;
    int vlanid = 4094;
    int subnet = 2222;
    strcpy(cmd[0] , "command");
    strcpy(cmd[1] , "add");
    ss.str("");
    ss << vlanid;
    //cmd[3] = new char[20];
    strcpy(cmd[3], ss.str().c_str());

    ss.str("");
    ss << subnet;
    strcpy(cmd[2], ss.str().c_str());

    //cmd[2] = (ss.str().append('\0').c_str());
    printf("cmd=%s, %s, %s, %s hhhh\n", cmd[0], cmd[1], cmd[2], cmd[3]);



    return 0;
}

