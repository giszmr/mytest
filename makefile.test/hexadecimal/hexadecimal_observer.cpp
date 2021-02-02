#include <stdio.h>
#include "hexadecimal_observer.h"

void CHexadecimalObserver::Update(int msg)
{
    printf("Hexadecimal string: %x\n", msg);
}
