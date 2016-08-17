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
    Q_PROPERTY(bool useFeed READ useFeed WRITE setUseFeed NOTIFY useFeedChanged)

public:
    AddressBarDialog(QQuickItem* parent = nullptr);
    bool backEnabled() { return _backEnabled; }
    bool forwardEnabled() { return _forwardEnabled; }
    bool useFeed() { return _useFeed; }
    void setUseFeed(bool useFeed) { if (_useFeed != useFeed) { _useFeed = useFeed; emit useFeedChanged(); } }

signals:
    void backEnabledChanged();
    void forwardEnabledChanged();
    void useFeedChanged();

protected:
    void displayAddressOfflineMessage();
    void displayAddressNotFoundMessage();

    Q_INVOKABLE void loadAddress(const QString& address, bool fromSuggestions = false);
    Q_INVOKABLE void loadHome();
    Q_INVOKABLE void loadBack();
    Q_INVOKABLE void loadForward();
    Q_INVOKABLE void observeShownChanged(bool visible);

    bool _backEnabled;
    bool _forwardEnabled;
    bool _useFeed { false };
};

#endif
