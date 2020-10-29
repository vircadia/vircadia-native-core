//
//  MetaverseAPI.h
//  libraries/networking/src
//
//  Created by Kalila L. on 2019-12-16.
//  Copyright 2019 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef athena_MetaverseAPI_h
#define athena_MetaverseAPI_h

#include <QtCore/QProcessEnvironment>
#include <QtCore/QUrl>

namespace MetaverseAPI {
    QUrl getCurrentMetaverseServerURL();
    QString getCurrentMetaverseServerURLPath(bool appendForwardSlash = false);
}

#endif  // athena_MetaverseAPI_h
