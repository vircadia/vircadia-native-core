//
//  CachesSizeDialog.h
//
//
//  Created by Clement on 1/12/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_CachesSizeDialog_h
#define hifi_CachesSizeDialog_h

#include <QDialog>

class QDoubleSpinBox;

class CachesSizeDialog : public QDialog {
    Q_OBJECT
public:
    // Sets up the UI
    CachesSizeDialog(QWidget* parent);

signals:
    void closed();

public slots:
    void reject() override;
    void confirmClicked(bool checked);
    void resetClicked(bool checked);

protected:
    // Emits a 'closed' signal when this dialog is closed.
    void closeEvent(QCloseEvent* event) override;

private:
    QDoubleSpinBox* _animations = nullptr;
    QDoubleSpinBox* _geometries = nullptr;
    QDoubleSpinBox* _scripts = nullptr;
    QDoubleSpinBox* _sounds = nullptr;
    QDoubleSpinBox* _textures = nullptr;
};

#endif // hifi_CachesSizeDialog_h
