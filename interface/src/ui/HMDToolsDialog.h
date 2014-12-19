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

class HMDWindowWatcher;

class HMDToolsDialog : public QDialog {
    Q_OBJECT
public:
    // Sets up the UI
    HMDToolsDialog(QWidget* parent);
    ~HMDToolsDialog();

    QString getDebugDetails() const;
    QScreen* getHMDScreen() const { return _hmdScreen; }
    QScreen* getLastApplicationScreen() const { return _previousScreen; }
    bool hasHMDScreen() const { return _hmdScreenNumber >= -1; }
    void watchWindow(QWindow* window);
    
signals:
    void closed();

public slots:
    void reject();
    void switchModeClicked(bool checked);
    void activateWindowAfterEnterMode();
    void moveWindowAfterLeaveMode();
    void applicationWindowScreenChanged(QScreen* screen);
    void aboutToQuit(); 
    void screenCountChanged(int newCount);
    
protected:
    virtual void closeEvent(QCloseEvent*); // Emits a 'closed' signal when this dialog is closed.
    virtual void showEvent(QShowEvent* event);
    virtual void hideEvent(QHideEvent* event);

private:
    void centerCursorOnWidget(QWidget* widget);
    void enterHDMMode();
    void leaveHDMMode();

    bool _wasMoved;
    QRect _previousRect;
    QScreen* _previousScreen;
    QScreen* _hmdScreen;
    int _hmdScreenNumber;
    QPushButton* _switchModeButton;
    QLabel* _debugDetails;

    QRect _previousDialogRect;
    QScreen* _previousDialogScreen;
    bool _inHDMMode;
    
    QHash<QWindow*, HMDWindowWatcher*> _windowWatchers;
};


class HMDWindowWatcher : public QObject {
    Q_OBJECT
public:
    // Sets up the UI
    HMDWindowWatcher(QWindow* window, HMDToolsDialog* hmdTools);
    ~HMDWindowWatcher();

public slots:
    void windowScreenChanged(QScreen* screen);
    void windowGeometryChanged(int arg);
    
private:
    QWindow* _window;
    HMDToolsDialog* _hmdTools;
    QRect _previousRect;
    QScreen* _previousScreen;
};

#endif // hifi_HMDToolsDialog_h
