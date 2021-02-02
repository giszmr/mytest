#include <stdio.h>

int main(int argc, char *argv[])
{
	int a = 1;
	int *b = (int *)&a;

	printf("hello\n");
	printf("a=%d, b=%d\n", a, *b);

	*b = 3;
	printf("a=%d, b=%d\n", a, *b);

    int arr[10] = {0};
    int i = 0;
    for (i; i < 10; i++)
    {
        printf("&arr[%d]=%lu, arr+[%d]=%lu\n", i, &arr[i], i, arr+i);
    }
    printf("&arr[5]-&arr[0]=%lu\n", &arr[5]-&arr[0]);

    int aaa = 10;
    int* ptA = &aaa;
    int* ptB = ptA;
    int* ptC = ptA;

    printf("ptA=%p ptB=%14p ptC=%p\n", ptA, ptB, ptC);
    ptB = NULL;
    printf("ptA=%p ptB=%14p ptC=%p\n", ptA, ptB, ptC);
    printf("ptA=%d ptB=%d ptC=%d\n", ptA, ptB, ptC);
	return 0;
}


