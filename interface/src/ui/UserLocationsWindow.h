//
//  UserLocationsWindow.h
//  interface/src/ui
//
//  Created by Ryan Huffman on 06/24/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_UserLocationsWindow_h
#define hifi_UserLocationsWindow_h

#include "ui_userLocationsWindow.h"
#include "UserLocationsModel.h"

class UserLocationsWindow : public QDialog {
    Q_OBJECT
public:
    UserLocationsWindow(QWidget* parent = NULL);

protected slots:
    void updateEnabled();
    void goToModelIndex(const QModelIndex& index);
    void deleteSelection();
    void renameSelection();

private:
    Ui::UserLocationsWindow _ui;
    QSortFilterProxyModel _proxyModel;
    UserLocationsModel _userLocationsModel;
};

#endif // hifi_UserLocationsWindow_h
