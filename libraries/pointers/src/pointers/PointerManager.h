//
//  Created by Bradley Austin Davis on 2017/10/16
//  Copyright 2013-2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_pointers_PointerManager_h
#define hifi_pointers_PointerManager_h

#include <DependencyManager.h>
#include <PointerEvent.h>

class PointerManager : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY
public:
    PointerManager();

signals:
    void triggerBegin(const QUuid& id, const PointerEvent& pointerEvent);
    void triggerContinue(const QUuid& id, const PointerEvent& pointerEvent);
    void triggerEnd(const QUuid& id, const PointerEvent& pointerEvent);

    void hoverEnter(const QUuid& id, const PointerEvent& pointerEvent);
    void hoverOver(const QUuid& id, const PointerEvent& pointerEvent);
    void hoverLeave(const QUuid& id, const PointerEvent& pointerEvent);
};

#endif // hifi_pointers_PointerManager_h
