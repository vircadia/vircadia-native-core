//
//  LoginPointerManager.h
//  interface/src
//
//  Created by Wayne Chen on 11/5/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_LoginPointerManager_h
#define hifi_LoginPointerManager_h
#include <QtCore/QList>
#include <QtCore/QVariant>

#include <PointerEvent.h>

#include <shared/ReadWriteLockable.h>

class LoginPointerManager : protected ReadWriteLockable {
public:
    LoginPointerManager() {}
    ~LoginPointerManager();

    void init();

    void update();

    const unsigned int leftLoginPointerID() const {
        return resultWithReadLock<unsigned int>([&] { return _leftLoginPointerID; });
    }

    const unsigned int rightLoginPointerID() const {
        return resultWithReadLock<unsigned int>([&] { return _rightLoginPointerID; });
    }

private:
    QList<QVariant> _renderStates {};
    QList<QVariant> _defaultRenderStates {};
    unsigned int _leftLoginPointerID { PointerEvent::INVALID_POINTER_ID };
    unsigned int _rightLoginPointerID { PointerEvent::INVALID_POINTER_ID };
};
#endif // hifi_LoginPointerManager_h
