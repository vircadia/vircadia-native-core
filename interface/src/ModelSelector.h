//
//  ModelSelector.h
//
//
//  Created by Clement on 3/10/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ModelSelector_h
#define hifi_ModelSelector_h

#include <QDialog>
#include <QFileInfo>

#include <SettingHandle.h>

#include "ui/ModelsBrowser.h"

class QComboBox;
class QPushButton;

class ModelSelector : public QDialog {
    Q_OBJECT

public:
    ModelSelector();

    QFileInfo getFileInfo() const;
    FSTReader::ModelType getModelType() const;

    public slots:
    virtual void accept() override;

    private slots:
    void browse();

private:
    QFileInfo _modelFile;
    QPushButton* _browseButton;
    QComboBox* _modelType;
};

#endif // hifi_ModelSelector_h
