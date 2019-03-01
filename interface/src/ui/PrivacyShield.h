//
//  PrivacyShield.h
//  interface/src/ui
//
//  Created by Wayne Chen on 2/27/19.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#include <QtCore/QObject>
#include <QtCore/QUuid>
#include <QtGlobal>

#include <DependencyManager.h>
#include <Sound.h>

class PrivacyShield : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:
    PrivacyShield();
    void createPrivacyShield();
    void destroyPrivacyShield();

    bool isVisible() const { return _visible; }
    void update(float deltaTime);

protected slots:
    void enteredIgnoreRadius();
    void onPrivacyShieldToggled(bool enabled, bool doNotLog = false);

private:
    void showPrivacyShield();
    void hidePrivacyShield();

    SharedSoundPointer _bubbleActivateSound;
    QUuid _localPrivacyShieldID;
    quint64 _privacyShieldTimestamp;
    quint64 _lastPrivacyShieldSoundTimestamp;
    bool _visible { false };
    bool _updateConnected { false };
};
