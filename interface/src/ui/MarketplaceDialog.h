//
//  MarketplaceDialog.h
//
//  Created by Bradley Austin Davis on 2015/04/14
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_MarketplaceDialog_h
#define hifi_MarketplaceDialog_h

#include <OffscreenQmlDialog.h>

class MarketplaceDialog : public OffscreenQmlDialog
{
    Q_OBJECT
    HIFI_QML_DECL

public:
    MarketplaceDialog(QQuickItem* parent = nullptr);

    Q_INVOKABLE bool navigationRequested(const QString& url);

};

#endif
