//
//  SSLCommon.h
//  libraries/networking/src
//
//  Created by Nshan G. on 2021-12-08.
//  Copyright 2021 Vircadia contributors.
//  Copyright 2021 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef VIRCADIA_LIBRARIES_NETWORKING_SRC_SSLCOMMON_H
#define VIRCADIA_LIBRARIES_NETWORKING_SRC_SSLCOMMON_H

#include <QString>

struct CertificatePaths {
    QString cert;
    QString key;
    QString trustedAuthorities;
};

#endif /* end of include guard */
