//
//  UserLocationsDialog.h
//  interface/src/ui
//
//  Created by Ryan Huffman on 06/24/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_UserLocationsDialog_h
#define hifi_UserLocationsDialog_h

#include "ui_userLocationsDialog.h"
#include "UserLocationsModel.h"

class UserLocationsDialog : public QDialog {
    Q_OBJECT
public:
    UserLocationsDialog(QWidget* parent = NULL);

protected slots:
    void updateEnabled();
    void goToModelIndex(const QModelIndex& index);
    void deleteSelection();
    void renameSelection();

private:
    Ui::UserLocationsDialog _ui;
    QSortFilterProxyModel _proxyModel;
    UserLocationsModel _userLocationsModel;
};

#endif // hifi_UserLocationsDialog_h
