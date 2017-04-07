//
//  ModelBakeWidget.h
//  tools/oven/src/ui
//
//  Created by Stephen Birarda on 4/6/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ModelBakeWidget_h
#define hifi_ModelBakeWidget_h

#include <QtWidgets/QWidget>

#include <SettingHandle.h>

#include <FBXBaker.h>

class QLineEdit;

class ModelBakeWidget : public QWidget {
    Q_OBJECT

public:
    ModelBakeWidget(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());

private slots:
    void chooseFileButtonClicked();
    void chooseOutputDirButtonClicked();
    void bakeButtonClicked();

    void outputDirectoryChanged(const QString& newDirectory);

private:
    void setupUI();

    std::list<std::unique_ptr<FBXBaker>> _bakers;

    QLineEdit* _modelLineEdit;
    QLineEdit* _outputDirLineEdit;

    QUrl modelToBakeURL;

    Setting::Handle<QString> _exportDirectory;
    Setting::Handle<QString> _modelStartDirectory;
};

#endif // hifi_ModelBakeWidget_h
