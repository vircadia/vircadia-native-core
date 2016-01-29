//
//  ImageOverlay.cpp
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ImageOverlay.h"

#include <DependencyManager.h>
#include <GeometryCache.h>
#include <gpu/Context.h>
#include <RegisteredMetaTypes.h>


QString const ImageOverlay::TYPE = "image";
QUrl const ImageOverlay::URL(QString("hifi/overlays/ImageOverlay.qml"));

ImageOverlay::ImageOverlay() 
    : QmlOverlay(URL) { }

ImageOverlay::ImageOverlay(const ImageOverlay* imageOverlay) :
    QmlOverlay(URL, imageOverlay) { }

ImageOverlay* ImageOverlay::createClone() const {
    return new ImageOverlay(this);
}

