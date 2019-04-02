//
//  ImageComparer.h
//
//  Created by Nissim Hadar on 18 Nov 2017.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_ImageComparer_h
#define hifi_ImageComparer_h

#include "common.h"

#include <QtCore/QString>
#include <QImage>

class ImageComparer {
public:
    void compareImages(const QImage& resultImage, const QImage& expectedImage);

    double getSSIMValue();
    double getWorstTileValue();

    SSIMResults getSSIMResults();

private:
    SSIMResults _ssimResults;
};

#endif // hifi_ImageComparer_h
