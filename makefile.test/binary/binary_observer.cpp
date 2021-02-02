#include <stdio.h>
#include "binary_observer.h"

void CBinaryObserver::Update(int msg)
{
    printf("Binary string: ");
    for (int i = sizeof(int) * 8 - 1; i >= 0; i--)
    {
        if ( (1 << i) & msg)
            printf("1");
        else
            printf("0");
    }
    printf("\n");
}

