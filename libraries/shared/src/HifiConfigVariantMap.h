//
//  HifiConfigVariantMap.h
//  hifi
//
//  Created by Stephen Birarda on 2014-04-08.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__HifiConfigVariantMap__
#define __hifi__HifiConfigVariantMap__

#include <QtCore/QStringList>

class HifiConfigVariantMap {
public:
    static QVariantMap mergeCLParametersWithJSONConfig(const QStringList& argumentList);
};

#endif /* defined(__hifi__HifiConfigVariantMap__) */
