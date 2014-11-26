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

    QString getDebugDetails() const;
    
signals:
    void closed();

public slots:
    void reject();
    void enterModeClicked(bool checked);
    void leaveModeClicked(bool checked);
    void activateWindowAfterEnterMode();
    void moveWindowAfterLeaveMode();
    void applicationWindowScreenChanged(QScreen* screen);
    void dialogWindowScreenChanged(QScreen* screen);
    void dialogWindowGeometryChanged(int arg);

protected:
    virtual void closeEvent(QCloseEvent*); // Emits a 'closed' signal when this dialog is closed.
    virtual void showEvent(QShowEvent* event);
    virtual void hideEvent(QHideEvent* event);

private:
    void centerCursorOnWidget(QWidget* widget);

    bool _wasMoved;
    QRect _previousRect;
    QScreen* _previousScreen;
    QScreen* _hmdScreen;
    QLabel* _debugDetails;

    QRect _previousDialogRect;
    QScreen* _previousDialogScreen;
};

#endif // hifi_HMDToolsDialog_h
