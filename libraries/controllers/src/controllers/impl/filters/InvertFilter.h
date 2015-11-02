//
//  Created by Bradley Austin Davis 2015/10/25
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Controllers_Filters_InvertFilter_h
#define hifi_Controllers_Filters_InvertFilter_h

#include "ScaleFilter.h"

namespace controller {

class InvertFilter : public ScaleFilter {
    REGISTER_FILTER_CLASS(InvertFilter);
public:
    using ScaleFilter::parseParameters;
    InvertFilter() : ScaleFilter(-1.0f) {}

    virtual bool parseParameters(const QJsonArray& parameters) { return true; }

private:
};

}

#endif
