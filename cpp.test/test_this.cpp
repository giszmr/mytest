#include <iostream>

namespace mynamespace {

class MyClass {
public:
	MyClass() {};
	~MyClass() {};
	MyClass* GetPtr()
	{
		return this;
	}
	MyClass& GetObject()
	{
		return *this;
	}
private:
};

}

int main()
{
	printf("hello\n");
	
	mynamespace::MyClass A;
	if(&A == A.GetPtr())
	{
		printf("get this\n");
	}
	else
	{
		printf("not get this\n");
	}

	if(&A == &A.GetObject())
	{
		printf("get object\n");
	}
	else 
	{
		printf("not get object\n");
	}
	
	return 0;
}


