//
//  ImageOverlay.h
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ImageOverlay_h
#define hifi_ImageOverlay_h

#include <QUrl>

#include "QmlOverlay.h"

class ImageOverlay : public QmlOverlay {
    Q_OBJECT
    
public:
    static QString const TYPE;
    virtual QString getType() const override { return TYPE; }
    static QUrl const URL;

    ImageOverlay();
    ImageOverlay(const ImageOverlay* imageOverlay);
    
    virtual ImageOverlay* createClone() const override;
};

 
#endif // hifi_ImageOverlay_h
