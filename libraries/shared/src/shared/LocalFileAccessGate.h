//
//  Created by Bradley Austin Davis on 2019/09/05
//  Copyright 2013-2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_LocalFileAccessGate_h
#define hifi_LocalFileAccessGate_h

namespace hifi { namespace scripting {

void setLocalAccessSafeThread(bool safe = true);

bool isLocalAccessSafeThread();

}}  // namespace hifi::scripting

#endif
