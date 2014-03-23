//
//  SnapshotShareDialog.cpp
//  hifi
//
//  Created by Stojce Slavkovski on 2/16/14.
//
//

#include "SnapshotShareDialog.h"

SnapshotShareDialog::SnapshotShareDialog(QString fileName, QWidget* parent) : QDialog(parent), _fileName(fileName) {

    setAttribute(Qt::WA_DeleteOnClose);

    ui.setupUi(this);

    QPixmap snaphsotPixmap(fileName);
    float snapshotRatio = static_cast<float>(snaphsotPixmap.size().width()) / snaphsotPixmap.size().height();
    float labelRatio = static_cast<float>(ui.snapshotWidget->size().width()) / ui.snapshotWidget->size().height();

    // set the same aspect ratio of label as of snapshot
    if (snapshotRatio > labelRatio) {
        ui.snapshotWidget->setFixedHeight(ui.snapshotWidget->size().width() / snapshotRatio);
    } else {
        ui.snapshotWidget->setFixedWidth(ui.snapshotWidget->size().height() * snapshotRatio);
    }

    ui.snapshotWidget->setPixmap(snaphsotPixmap);
    ui.snapshotWidget->adjustSize();
}
