//
//  UpdateDialog.cpp
//  hifi
//
//  Created by Leonardo Murillo on 6/3/15.
//
//

#include "UpdateDialog.h"
#include "DependencyManager.h"

HIFI_QML_DEF(UpdateDialog)

UpdateDialog::UpdateDialog(QQuickItem* parent) : OffscreenQmlDialog(parent) {
    
}

void UpdateDialog::hide() {
    ((QQuickItem*)parent())->setEnabled(false);
}
