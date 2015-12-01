//
//  Created by Bradley Austin Davis on 2015/11/14
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_RecorderDialog_h
#define hifi_RecorderDialog_h

#include <OffscreenQmlDialog.h>

class RecorderDialog : public OffscreenQmlDialog {
    Q_OBJECT
    HIFI_QML_DECL

public:
    RecorderDialog(QQuickItem* parent = nullptr);

signals:

protected:
    void hide();
};

#endif
