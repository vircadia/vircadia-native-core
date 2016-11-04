//
//  Created by Bradley Austin Davis on 2016/01/27
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RectangleOverlay_h
#define hifi_RectangleOverlay_h

#include "QmlOverlay.h"

class RectangleOverlay : public QmlOverlay {
public:
    static QString const TYPE;
    virtual QString getType() const override { return TYPE; }
    static QUrl const URL;

    RectangleOverlay();
    RectangleOverlay(const RectangleOverlay* RectangleOverlay);

    virtual RectangleOverlay* createClone() const override;
};

#endif // hifi_RectangleOverlay_h
