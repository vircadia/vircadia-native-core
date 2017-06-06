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

class BakerCLI : public QObject {
    Q_OBJECT   

public:
    void bakeFile(const QString inputFilename, const QString outputFilename);

private slots:
    void handleFinishedBaker();

};

#endif // hifi_BakerCLI_h