#include <stdio.h>
#include <string>
#include "../subject/subject.h"
#include "../binary/binary_observer.h"
#include "../octonary/octonary_observer.h"
#include "../hexadecimal/hexadecimal_observer.h"

int main(int argc, char* argv[])
{
    CSubject subject = CSubject();
    
    CBinaryObserver observerB;// = CBinaryObserver();
    subject.Attach(&observerB);

    COctonaryObserver observerO;// = COctonaryObserver();
    subject.Attach(&observerO);

    CHexadecimalObserver observerH;// = CHexadecimalObserver();
    subject.Attach(&observerH);

    subject.SetState(15);

    return 0;
}
