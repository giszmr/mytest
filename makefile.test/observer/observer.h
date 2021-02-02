#ifndef OBSERVER_H
#define OBSERVER_H

#include <stdio.h>

class IObserver {
    public:
        //IObserver(){};
        virtual void Update(int msg) {};
};


#endif
