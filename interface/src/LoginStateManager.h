//
//  LoginStateManager.h
//  interface/src
//
//  Created by Wayne Chen on 11/5/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_LoginStateManager_h
#define hifi_LoginStateManager_h
#include <QtCore/QList>
#include <QtCore/QVariant>

#include <PointerEvent.h>

#include <shared/ReadWriteLockable.h>

class LoginStateManager : protected ReadWriteLockable {
public:
    LoginStateManager() = default;
    ~LoginStateManager() = default;

    void setUp();
    void tearDown();

    void update(const QString& dominantHand, const QUuid& loginObjectID);

    bool isSetUp() const { return (_leftLoginPointerID > PointerEvent::INVALID_POINTER_ID) && (_rightLoginPointerID > PointerEvent::INVALID_POINTER_ID); }

private:
    QString _dominantHand;
    QList<QVariant> _renderStates {};
    QList<QVariant> _defaultRenderStates {};
    unsigned int _leftLoginPointerID { PointerEvent::INVALID_POINTER_ID };
    unsigned int _rightLoginPointerID { PointerEvent::INVALID_POINTER_ID };
};

#endif // hifi_LoginStateManager_h
