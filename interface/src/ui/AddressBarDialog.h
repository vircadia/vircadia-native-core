//
//  AddressBarDialog.h
//  interface/src/ui
//
//  Created by Bradley Austin Davis on 2015/04/14
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_AddressBarDialog_h
#define hifi_AddressBarDialog_h

#include <OffscreenQmlDialog.h>

class AddressBarDialog : public OffscreenQmlDialog {
    Q_OBJECT
    HIFI_QML_DECL
    Q_PROPERTY(bool backEnabled READ backEnabled NOTIFY backEnabledChanged)
    Q_PROPERTY(bool forwardEnabled READ forwardEnabled NOTIFY forwardEnabledChanged)

public:
    AddressBarDialog(QQuickItem* parent = nullptr);
    bool backEnabled() { return _backEnabled; }
    bool forwardEnabled() { return _forwardEnabled; }

signals:
    void backEnabledChanged();
    void forwardEnabledChanged();

protected:
    void displayAddressOfflineMessage();
    void displayAddressNotFoundMessage();

    Q_INVOKABLE QString getHost() const;
    Q_INVOKABLE void loadAddress(const QString& address);
    Q_INVOKABLE void loadHome();
    Q_INVOKABLE void loadBack();
    Q_INVOKABLE void loadForward();
    Q_INVOKABLE void observeShownChanged(bool visible);

    bool _backEnabled;
    bool _forwardEnabled;
};

#endif
