//
//  SnapshotShareDialog.cpp
//  interface/src/ui
//
//  Created by Stojce Slavkovski on 2/16/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#if 0


#include <OffscreenUi.h>

const int NARROW_SNAPSHOT_DIALOG_SIZE = 500;
const int WIDE_SNAPSHOT_DIALOG_WIDTH = 650;
const int SUCCESS_LABEL_HEIGHT = 140;

const QString SHARE_BUTTON_STYLE = "border-width:0;border-radius:9px;border-radius:9px;font-family:Arial;font-size:18px;"
    "font-weight:100;color:#FFFFFF;width: 120px;height: 50px;";
const QString SHARE_BUTTON_ENABLED_STYLE = "background-color: #333;";
const QString SHARE_BUTTON_DISABLED_STYLE = "background-color: #999;";

Q_DECLARE_METATYPE(QNetworkAccessManager::Operation)

SnapshotShareDialog::SnapshotShareDialog(QString fileName, QWidget* parent) :
    QDialog(parent),
    _fileName(fileName)
{


    _ui.snapshotWidget->setPixmap(snaphsotPixmap);
    _ui.snapshotWidget->adjustSize();
}

void SnapshotShareDialog::accept() {
    // prevent multiple clicks on share button
    _ui.shareButton->setEnabled(false);
    // gray out share button
    _ui.shareButton->setStyleSheet(SHARE_BUTTON_STYLE + SHARE_BUTTON_DISABLED_STYLE);
    uploadSnapshot();
}


#endif
