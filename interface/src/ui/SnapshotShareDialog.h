//
//  SnapshotShareDialog.h
//  hifi
//
//  Created by Stojce Slavkovski on 2/16/14.
//
//

#ifndef __hifi__snapshotShareDialog__
#define __hifi__snapshotShareDialog__

#include "ui_shareSnapshot.h"

class SnapshotShareDialog : public QDialog {
    Q_OBJECT

public:
    SnapshotShareDialog(QString fileName, QWidget* parent = 0);

private:
    QString _fileName;
    Ui_SnapshotShareDialog ui;
};

#endif /* defined(__hifi__snapshotShareDialog__) */
