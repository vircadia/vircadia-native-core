//
//  RenderingClient.cpp
//  gvr-interface/src
//
//  Created by Stephen Birarda on 1/20/15.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtWidgets/QInputDialog>

#include "GVRInterface.h"
#include "GVRMainWindow.h"

#include "RenderingClient.h"

RenderingClient::RenderingClient(QObject *parent) :
    Client(parent)
{
    
}

void RenderingClient::showAddressBar() {
#ifdef Q_OS_ANDROID
    QString addressString = QInputDialog::getText(qApp->getMainWindow()->centralWidget(), "Go to Address", "Address");
#endif
}
