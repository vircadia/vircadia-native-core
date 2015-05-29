//
//  DomainConnectionDialog.h
//  interface/src/ui
//
//  Created by Stephen Birarda on 05/26/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_DomainConnectionDialog_h
#define hifi_DomainConnectionDialog_h

#pragma once

#include <QtWidgets/QDialog>

class DomainConnectionDialog : public QDialog {
    Q_OBJECT
public:
    DomainConnectionDialog(QWidget* parent);
};

#endif // hifi_DomainConnectionDialog_h
