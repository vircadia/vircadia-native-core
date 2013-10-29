//
//  LodToolsDialog.h
//  interface
//
//  Created by Brad Hefta-Gaub on 7/19/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__LodToolsDialog__
#define __hifi__LodToolsDialog__

#include <QDialog>
#include <QLabel>

#include <VoxelSceneStats.h>

class LodToolsDialog : public QDialog {
    Q_OBJECT
public:
    // Sets up the UI
    LodToolsDialog(QWidget* parent);
    ~LodToolsDialog();
    
    float getVoxelSizeScale();

signals:
    void closed();

public slots:
    void reject();

protected:
    void paintEvent(QPaintEvent*);

    // Emits a 'closed' signal when this dialog is closed.
    void closeEvent(QCloseEvent*);

private:
};

#endif /* defined(__interface__LodToolsDialog__) */

