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
class QLabel;

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
    void reject() override;
    void screenCountChanged(int newCount);

protected:
    virtual void closeEvent(QCloseEvent*) override; // Emits a 'closed' signal when this dialog is closed.
    virtual void showEvent(QShowEvent* event) override;
    virtual void hideEvent(QHideEvent* event) override;

private:
    void centerCursorOnWidget(QWidget* widget);
    void enterHMDMode();
    void leaveHMDMode();
    void toggleHMDMode();
    void updateUi();

    QScreen* _previousScreen{ nullptr };
    QScreen* _hmdScreen{ nullptr };
    int _hmdScreenNumber{ -1 };
    QPushButton* _switchModeButton{ nullptr };
    QLabel* _debugDetails{ nullptr };

    QRect _previousDialogRect;
    QScreen* _previousDialogScreen{ nullptr };
    QString _hmdPluginName;
    QString _defaultPluginName;

    QHash<QWindow*, HMDWindowWatcher*> _windowWatchers;
    friend class HMDWindowWatcher;
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
