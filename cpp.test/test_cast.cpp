#include <cstdio>

class Base{
    public:
        Base(int base) : m_base(base) 
        {
            printf("Base::Base(%d)\n", base);
        };
        void Output() 
        {
            printf("Base::Output(%p, %d)\n", &m_base, m_base);
        };
        void Print() const
        {
            printf("Base::Print(%p, %d)\n", &m_base, m_base);
        };
    private:
        int m_base;
};

int main(int argc, char* argv[])
{
    int a = 0;
    int b = 1;
    int c = 2;

    printf("***const, pointer test***\n");
    const int* d = &a;
    printf("*d[%d], ", *d);
    d = &b;
    printf("*d[%d]\n", *d);
    //*d = c;  //error: assignment of read-only location

    int const* e = &a;
    printf("*e[%d], ", *e);
    e = &b;
    printf("*e[%d]\n", *e);
    //*e = c;  //error: assignment of read-only location

    int* const f = &a;
    printf("*f[%d], ", *f);
    //f = &b;  //error: assignment of read-only variable ‘f’
    *f = c;
    printf("*f[%d]\n", *f);

    printf("***c++ const_cast/dynamic_cast/reinterpret_cast/static_cast test***\n");
    //test object1 = object2
    Base base1(10);
    printf("base1[%p], ", &base1);
    base1.Output();
    Base base2 = base1;
    printf("base2[%p], ", &base2);
    base2.Output();

    //test const_cast<type>
    const Base base3(30);
    printf("base3[%p], ", &base3);
    //base3.Output();  //error: passing ‘const Base’ as ‘this’ argument of ‘void Base::Output()’ discards qualifiers
    base3.Print();
    Base *base4 = const_cast<Base*>(&base3); //there must be parentheses here.
    printf("base4[%p], ", base4);
    base4->Output();

    return 0;
}
