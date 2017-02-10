//
//  EntityScriptServerLogDialog.h
//  interface/src/ui
//
//  Created by Clement Brisset on 1/31/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_EntityScriptServerLogDialog_h
#define hifi_EntityScriptServerLogDialog_h

#include "BaseLogDialog.h"

class EntityScriptServerLogDialog : public BaseLogDialog {
    Q_OBJECT

public:
    EntityScriptServerLogDialog(QWidget* parent = nullptr);

protected:
    QString getCurrentLog() override { return QString(); };
};

#endif // hifi_EntityScriptServerLogDialog_h
