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
#include "BakeWidget.h"

class QCheckBox;
class QLineEdit;

class DomainBakeWidget : public BakeWidget {
    Q_OBJECT

public:
    DomainBakeWidget(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());
    
private slots:
    void chooseFileButtonClicked();
    void chooseOutputDirButtonClicked();
    void bakeButtonClicked();

    void outputDirectoryChanged(const QString& newDirectory);

    void handleBakerProgress(int baked, int total);
    void handleFinishedBaker();

private:
    void setupUI();

    QLineEdit* _domainNameLineEdit;
    QLineEdit* _entitiesFileLineEdit;
    QLineEdit* _outputDirLineEdit;
    QLineEdit* _destinationPathLineEdit;
    QCheckBox* _rebakeOriginalsCheckBox;

    Setting::Handle<QString> _domainNameSetting;
    Setting::Handle<QString> _exportDirectory;
    Setting::Handle<QString> _browseStartDirectory;
    Setting::Handle<QString> _destinationPathSetting;
};

#endif // hifi_ModelBakeWidget_h
