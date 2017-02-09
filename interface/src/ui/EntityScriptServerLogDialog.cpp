//
//  EntityScriptServerLogDialog.cpp
//  interface/src/ui
//
//  Created by Clement Brisset on 1/31/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "EntityScriptServerLogDialog.h"

#include <EntityScriptServerLogClient.h>

EntityScriptServerLogDialog::EntityScriptServerLogDialog(QWidget* parent) : BaseLogDialog(parent) {
    setWindowTitle("Entity Script Server Log");


    auto client = DependencyManager::get<EntityScriptServerLogClient>();
    QObject::connect(client.data(), &EntityScriptServerLogClient::receivedNewLogLines,
                     this, &EntityScriptServerLogDialog::appendLogLine);
}
