//
//  SkyboxBakeWidget.h
//  tools/oven/src/ui
//
//  Created by Stephen Birarda on 4/17/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_SkyboxBakeWidget_h
#define hifi_SkyboxBakeWidget_h

#include <QtWidgets/QWidget>

#include <SettingHandle.h>

#include <TextureBaker.h>

class QLineEdit;

class SkyboxBakeWidget : public QWidget {
    Q_OBJECT

public:
    SkyboxBakeWidget(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());

private slots:
    void chooseFileButtonClicked();
    void chooseOutputDirButtonClicked();
    void bakeButtonClicked();
    void cancelButtonClicked();

    void outputDirectoryChanged(const QString& newDirectory);

    void handleFinishedBaker();

private:
    void setupUI();

    using BakerRowPair = std::pair<std::unique_ptr<TextureBaker>, int>;
    using BakerRowPairList = std::list<BakerRowPair>;
    BakerRowPairList _bakers;

    QLineEdit* _selectionLineEdit;
    QLineEdit* _outputDirLineEdit;

    Setting::Handle<QString> _exportDirectory;
    Setting::Handle<QString> _selectionStartDirectory;
};

#endif // hifi_SkyboxBakeWidget_h
