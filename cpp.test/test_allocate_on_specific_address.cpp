#include <iostream>
#include <cstdio>

using namespace std;

class CMyClass
{
public:
    CMyClass(int v): m_v(v)
    {
        cout << "m_v     address: " << hex << &m_v << endl;
    }
    ~CMyClass(){}
private:
    int m_v;
};


int main(int argc, char* argv[])
{
    //operator new
    char* buffer = new char[sizeof(CMyClass)];
    printf("buffer  address: %p\n", buffer);
    //placement new
    CMyClass* myClass = new (buffer)CMyClass(99);
    cout << "myClass address: " << hex << myClass << endl;
    return 0;
}
