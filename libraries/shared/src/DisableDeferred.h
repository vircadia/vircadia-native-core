//
//  Created by Sam Gondelman on 3/7/19.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_DisableDeferred_h
#define hifi_DisableDeferred_h

#include <QString>
#include <QProcess>

#if defined(USE_GLES)
static bool DISABLE_DEFERRED = true;
#else
static const QString RENDER_FORWARD{ "HIFI_RENDER_FORWARD" };
static bool DISABLE_DEFERRED = QProcessEnvironment::systemEnvironment().contains(RENDER_FORWARD);
#endif


#endif // hifi_DisableDeferred_h

