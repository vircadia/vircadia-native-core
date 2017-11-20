//
//  ITKImageComparer.h
//
//  Created by Nissim Hadar on 18 Nov 2017.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_ITKImageComparer_h
#define hifi_ITKImageComparer_h

#include "ImageComparer.h"

#include <itkImage.h>

class ITKImageComparer : public ImageComparer {
public:
    float compareImages(QString file1, QString file2) const final;
};

#endif // hifi_ITKImageComparer_h
