#include <cstdio>
#include <cstdlib>
#include <new>

// new and delete will be overloaded in this process
void* operator new(size_t size)
{
    printf("operator new: %d Bytes\n", size);
    void *m = malloc(size);
    if(!m) puts("Out of memory\n");
    return m;
}
void operator delete(void* m)
{
    puts("operator delete\n");
    free(m);
}

class Base
{
public:
    Base(){puts("Base::Base()");}
    virtual ~Base(){puts("Base::~Base()");}

    //new and delete will be overloaded in class Base;
    /*virtual*/ void* operator new(size_t size)//'operator new' cannot be declared virtual, since it is always static
    {
        printf("Base::operator new: %d Bytes\n", size);
        void *m = malloc(size);
        if(!m) puts("Out of memory\n");
        return m;
    }
    void operator delete(void* m)
    {
        puts("Base::operator delete\n");
        free(m);
    }

private:
    int i[200];
};

class Derive : public Base
{
public:
    Derive(){puts("Derive::Derive()");}
    ~Derive(){puts("Derive::~Derive()");}

    //Derive will overwrite Base's new() and delete()
    //If Derive doesn't overload new and delete, Base's new and delete will be used.
    void* operator new(size_t size)
    {
        printf("Derive::operator new: %d Bytes\n", size);
        void *m = malloc(size);
        if(!m) puts("Out of memory\n");
        return m;
    }
    void operator delete(void* m)
    {
        puts("Derive::operator delete\n");
        free(m);
    }

private:
    int j[100];
};


int main(int argc, char* argv[])
{
    int *a = new int(100);
    delete a;

    Base* bb = new Base();
    delete bb;

    Derive *dd = new Derive();
    delete dd;

    int *b = new int(11);
    delete b;

    return 0;
}




