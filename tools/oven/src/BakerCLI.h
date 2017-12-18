//
//  BakerCLI.h
//  tools/oven/src
//
//  Created by Robbie Uvanni on 6/6/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_BakerCLI_h
#define hifi_BakerCLI_h

#include <QtCore/QObject>
#include <QDir>

#include <memory>

#include "Baker.h"
#include "Oven.h"

static const int OVEN_STATUS_CODE_SUCCESS { 0 };
static const int OVEN_STATUS_CODE_FAIL { 1 };
static const int OVEN_STATUS_CODE_ABORT { 2 };

static const QString OVEN_ERROR_FILENAME = "errors.txt";

class BakerCLI : public QObject {
    Q_OBJECT   

public:
    BakerCLI(Oven* parent);
    void bakeFile(QUrl inputUrl, const QString& outputPath, const QString& type = QString::null);

private slots:
    void handleFinishedBaker();  

private:
    QDir _outputPath;
    std::unique_ptr<Baker> _baker;
};

#endif // hifi_BakerCLI_h
