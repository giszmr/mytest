#include <array>
#include <string>
#include <iostream>
using namespace std;

class State {
    public:
       State() {}
       //array<int, 2> myarray;

};

class MyConst {
    public:
        MyConst(int val):m_val(val){}
        ~MyConst(){}
        const int &getConst()
        {
            return m_val;
        }
    private:
        int m_val;
};

int main(int argc, char* argv[])
{
    State mystate;
    
    MyConst my(2);
    my.getConst();
    
    return 0;
}
