#ifndef BINARY_OBSERVER_H
#define BINARY_OBSERVER_H

#include "../observer/observer.h"

class CBinaryObserver : public IObserver {
    public:
        //CBinaryObserver() {};
        void Update(int msg);
};



#endif
