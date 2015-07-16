//
//  LogUtils.cpp
//  libraries/shared/src
//
//  Created by Ryan Huffman on 09/03/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtGlobal>

#include "LogUtils.h"

void LogUtils::init() {
#ifdef Q_OS_WIN
    // Windows applications buffer stdout/err hard when not run from a terminal,
    // making assignment clients run from the Stack Manager application not flush
    // log messages.
    // This will disable the buffering.  If this becomes a performance issue,
    // an alternative is to call fflush(...) periodically.
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);
#endif
}
