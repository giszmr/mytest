#ifndef SUBJECT_H
#define SUBJECT_H

#include <list>
#include "../observer/observer.h"

using namespace std;

class CSubject {
    public:
        int  GetState();
        void SetState(int state);
        virtual void Attach(IObserver *observer);
        void NotifyAllObservers();
    private:
        int m_State;
        list<IObserver*> m_Observers;
};


#endif
