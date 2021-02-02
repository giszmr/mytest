#include <stdio.h>
#include "octonary_observer.h"

void COctonaryObserver::Update(int msg)
{
    printf("Octonary string: %o\n", msg);
}
