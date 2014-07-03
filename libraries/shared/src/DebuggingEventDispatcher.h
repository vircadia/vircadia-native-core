//
//  DebuggingEventDispatcher.h
//  shared
//
//  Created by Brad Hefta-Gaub on 4/1/14
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//
//

#ifndef __shared__DebuggingEventDispatcher__
#define __shared__DebuggingEventDispatcher__

#include <QAbstractEventDispatcher>
#include <QSocketNotifier>
#include <QWinEventNotifier>
//#include <TimerInfo>

class DebuggingEventDispatcher : public QAbstractEventDispatcher {
    Q_OBJECT
public:
    DebuggingEventDispatcher(QThread* thread);
    virtual void flush();
    virtual bool hasPendingEvents();
    void installNativeEventFilter(QAbstractNativeEventFilter* filterObj);
    virtual void interrupt();
    virtual bool processEvents(QEventLoop::ProcessEventsFlags flags);
    //virtual bool registerEventNotifier(QWinEventNotifier* notifier);
    virtual void registerSocketNotifier(QSocketNotifier* notifier);
    int	registerTimer(int interval, Qt::TimerType timerType, QObject* object);
    virtual void registerTimer(int timerId, int interval, Qt::TimerType timerType, QObject* object);
    virtual QList<TimerInfo> registeredTimers(QObject* object) const;
    virtual int	remainingTime(int timerId);
    void removeNativeEventFilter(QAbstractNativeEventFilter* filter);
    //virtual void unregisterEventNotifier(QWinEventNotifier* notifier);
    virtual void unregisterSocketNotifier(QSocketNotifier* notifier);
    virtual bool unregisterTimer(int timerId);
    virtual bool unregisterTimers(QObject* object);
    virtual void wakeUp();
    
signals:
    void aboutToBlock();
    void awake();
private:
    QAbstractEventDispatcher*  _realEventDispatcher;
};

#endif // __shared__DebuggingEventDispatcher__
