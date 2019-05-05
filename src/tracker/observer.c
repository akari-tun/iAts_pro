#include "stddef.h"
#include "observer.h"
#include "freertos/FreeRTOS.h"

static void ObserverDelete(void *pObserver)
{
    free(pObserver);
}

Observer *ObserverCreate(char *Name, void *pObj, size_t Size)
{
    Observer *pObserver = (Observer *)malloc(Size);
    pObserver->Name = Name;
    pObserver->Obj = pObj;
    pObserver->Delete = ObserverDelete;
    return pObserver;
}

static void SubjectDelete(void *pSubject)
{
    free(pSubject);
}

Subject *SubjectCreate(size_t Size)
{
    Subject *pSubject = (Subject *)malloc(Size);
    pSubject->Delete = SubjectDelete;
    return pSubject;
}

static void Notifier_Attach(void *pNotifier, void *pObserver)
{
    List *pList = ((Notifier *)pNotifier)->Observers;
    pList->Add(pList, pObserver);
}

static void Notifier_Detach(void *pNotifier, void *pObserver)
{
    List *pList = ((Notifier *)pNotifier)->Observers;
    pList->Remove(pList, pObserver);
}

static void Notifier_Notify(void *pNotifier, void *val)
{
    List *pList = ((Notifier *)pNotifier)->Observers;
    int i;
    for(i = 0; i < pList->Total; i++)
    {
        ((Observer *)(pList->pListPointArray[i]))->Update(((Observer *)(pList->pListPointArray[i])), val);
    }
}

static void Notifier_Delete(void *pNotifier)
{
    ((Notifier *)pNotifier)->Observers->Delete(((Notifier *)pNotifier)->Observers);
    free(pNotifier);
}

Notifier *Notifier_Create(size_t Size)
{
    Notifier *pNotifier = (Notifier *)SubjectCreate(Size);
    pNotifier->Observers = ListCreate();    
    //重写函数
    ((Subject *)pNotifier)->Attach = Notifier_Attach;
    ((Subject *)pNotifier)->Detach = Notifier_Detach;
    ((Subject *)pNotifier)->Notify = Notifier_Notify;
    ((Subject *)pNotifier)->Delete = Notifier_Delete;
    return pNotifier;
}