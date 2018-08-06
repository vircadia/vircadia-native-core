//
//  Created by Bradley Austin Davis on 2018/05/13
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

class QByteArray;
class QString;

#include <functional>

void installTestMessageHandler();

bool downloadFile(const QString& url, const std::function<void(const QByteArray&)> handler);