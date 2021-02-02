#include <stdio.h>
#include "subject.h"

int CSubject::GetState()
{
    return m_State;
}

void CSubject::SetState(int state)
{
    m_State = state;
    //printf("zhumengri %s\n", __FUNCTION__);
    NotifyAllObservers();
}

void CSubject::Attach(IObserver *observer)
{
    //printf("zhumengri %s\n", __FUNCTION__);
    m_Observers.push_back(observer);
    //observer->Update(15);
    //printf("zhumengri list size %d\n", m_Observers.size());
}

void CSubject::NotifyAllObservers()
{
    int i = 0;
    //printf("zhumengri %s\n", __FUNCTION__);

    list<IObserver*>::iterator iter;
    for ( iter = m_Observers.begin(); iter != m_Observers.end(); iter++)
    {
        //printf("zhumengri %d \n", i);
        (*iter)->Update(m_State);
        i++;
    }
}
