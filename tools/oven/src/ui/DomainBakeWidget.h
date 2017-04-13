//
//  DomainBakeWidget.h
//  tools/oven/src/ui
//
//  Created by Stephen Birarda on 4/12/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_DomainBakeWidget_h
#define hifi_DomainBakeWidget_h

#include <QtWidgets/QWidget>

#include <SettingHandle.h>

#include "../DomainBaker.h"

class QLineEdit;

class DomainBakeWidget : public QWidget {
    Q_OBJECT

public:
    DomainBakeWidget(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());

private slots:
    void chooseFileButtonClicked();
    void chooseOutputDirButtonClicked();
    void bakeButtonClicked();
    void cancelButtonClicked();

    void outputDirectoryChanged(const QString& newDirectory);

private:
    void setupUI();

    std::unique_ptr<DomainBaker> _baker;

    QLineEdit* _domainNameLineEdit;
    QLineEdit* _entitiesFileLineEdit;
    QLineEdit* _outputDirLineEdit;

    Setting::Handle<QString> _exportDirectory;
    Setting::Handle<QString> _browseStartDirectory;
};

#endif // hifi_ModelBakeWidget_h
