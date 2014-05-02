//
//  SnapshotShareDialog.cpp
//  hifi
//
//  Created by Stojce Slavkovski on 2/16/14.
//
//

const int NARROW_SNAPSHOT_DIALOG_SIZE = 500;
const int WIDE_SNAPSHOT_DIALOG_WIDTH = 650;

#include "SnapshotShareDialog.h"

SnapshotShareDialog::SnapshotShareDialog(QString fileName, QWidget* parent) : QDialog(parent), _fileName(fileName) {

    setAttribute(Qt::WA_DeleteOnClose);

    ui.setupUi(this);

    QPixmap snaphsotPixmap(fileName);
    float snapshotRatio = static_cast<float>(snaphsotPixmap.size().width()) / snaphsotPixmap.size().height();

    // narrow snapshot
    if (snapshotRatio > 1) {
        setFixedWidth(WIDE_SNAPSHOT_DIALOG_WIDTH);
        ui.snapshotWidget->setFixedWidth(WIDE_SNAPSHOT_DIALOG_WIDTH);
    }

    float labelRatio = static_cast<float>(ui.snapshotWidget->size().width()) / ui.snapshotWidget->size().height();

    // set the same aspect ratio of label as of snapshot
    if (snapshotRatio > labelRatio) {
        int oldHeight = ui.snapshotWidget->size().height();
        ui.snapshotWidget->setFixedHeight(ui.snapshotWidget->size().width() / snapshotRatio);

        // if height is less then original, resize the window as well
        if (ui.snapshotWidget->size().height() < NARROW_SNAPSHOT_DIALOG_SIZE) {
            setFixedHeight(size().height() - (oldHeight - ui.snapshotWidget->size().height()));
        }
    } else {
        ui.snapshotWidget->setFixedWidth(ui.snapshotWidget->size().height() * snapshotRatio);
    }

    ui.snapshotWidget->setPixmap(snaphsotPixmap);
    ui.snapshotWidget->adjustSize();
}

void SnapshotShareDialog::accept() {
    // post to Discourse forum
    
    close();
}
