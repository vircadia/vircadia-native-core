//
//  TestRailRunSelectorWindow.h
//
//  Created by Nissim Hadar on 31 Jul 2017.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_TestRailRunSelectorWindow_h
#define hifi_TestRailRunSelectorWindow_h

#include "ui_TestRailRunSelectorWindow.h"

class TestRailRunSelectorWindow : public QDialog, public Ui::TestRailRunSelectorWindow {
    Q_OBJECT

public:
    TestRailRunSelectorWindow(QWidget* parent = Q_NULLPTR);

    void reset();

    bool getUserCancelled();

    void setURL(const QString& user);
    QString getURL();

    void setUser(const QString& user);
    QString getUser();

    QString getPassword();

    void setProjectID(const int project);
    int getProjectID();

    void setSuiteID(const int project);
    int getSuiteID();

    bool userCancelled{ false };

    void updateSectionsComboBoxData(QStringList data);
    int getSectionID();

private slots:
    void on_acceptButton_clicked();
    void on_OKButton_clicked();
    void on_cancelButton_clicked();
};

#endif