//
//  TestRailSelectorWindow.h
//
//  Created by Nissim Hadar on 26 Jul 2017.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_TestRailSelectorWindow_h
#define hifi_TestRailSelectorWindow_h

#include "ui_TestRailSelectorWindow.h"

class TestRailSelectorWindow : public QDialog, public Ui::TestRailSelectorWindow {
    Q_OBJECT

public:
    TestRailSelectorWindow(QWidget* parent = Q_NULLPTR);

    bool getUserCancelled();

    void setURL(const QString& user);
    QString getURL();

    void setUser(const QString& user);
    QString getUser();

    QString getPassword();

    void setProject(const int project);
    int getProject();

    bool userCancelled{ false };

private slots:
    void on_OKButton_clicked();
    void on_cancelButton_clicked();
};

#endif