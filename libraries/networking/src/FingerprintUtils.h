//
//  FingerprintUtils.h
//  libraries/networking/src
//
//  Created by David Kelly on 2016-12-02.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_FingerprintUtils_h
#define hifi_FingerprintUtils_h

#include <QString>

#ifdef Q_OS_WIN
#include <comdef.h>
#include <Wbemidl.h>
#endif //Q_OS_WIN

class FingerprintUtils {
public:
    static QString getMachineFingerprint();

};

#endif // hifi_FingerprintUtils_h

