#include <cstdio>
#include <cstdlib>


namespace dynsync
{
    static void func(int a = 1)
    {
        printf("a = %d\n", a);
    }

}





int main(int argc, char* argv[])
{
    dynsync::func();
    return 0;
}

