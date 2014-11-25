//
//  HMDToolsDialog.h
//  interface/src/ui
//
//  Created by Brad Hefta-Gaub on 7/19/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_HMDToolsDialog_h
#define hifi_HMDToolsDialog_h

#include <QDialog>

class HMDToolsDialog : public QDialog {
    Q_OBJECT
public:
    // Sets up the UI
    HMDToolsDialog(QWidget* parent);
    ~HMDToolsDialog();
    
signals:
    void closed();

public slots:
    void reject();
    void enterModeClicked(bool checked);
    void leaveModeClicked(bool checked);
    void moveWindowAfterLeaveMode();

protected:
    // Emits a 'closed' signal when this dialog is closed.
    void closeEvent(QCloseEvent*);

private:
    bool _wasMoved;
    QRect _previousRect;
    QScreen* _previousScreen;
};

#endif // hifi_HMDToolsDialog_h
