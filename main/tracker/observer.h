#pragma once

#include "util/list.h"

typedef struct _Subject Subject;
typedef struct _Observer Observer;
typedef struct _Notifier Notifier;

//抽像观察者
struct _Observer
{
    char *Name;
    void *Obj;
    void (*Update)(void *pObserver, void *val);
    void (*Delete)(void *pObserver);
};

//抽像通知者接口
struct _Subject
{
    void (*Attach)(void *pNotifier, void *pObserver);
    void (*Detach)(void *pNotifier, void *pObserver);
    void (*Notify)(void *pNotifier, void *val);
    void (*Delete)(void *pNotifier);
};

//抽象通知者
struct _Notifier
{
    Subject mSubject;
    List *Observers;
    void *Action;
};

Observer *ObserverCreate(char *Name, void *pSub, size_t Size);
Subject *SubjectCreate(size_t Size);
Notifier *Notifier_Create(size_t Size);