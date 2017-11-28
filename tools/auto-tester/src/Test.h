//
//  Test.h
//  zone/ambientLightInheritence
//
//  Created by Nissim Hadar on 2 Nov 2017.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_test_h
#define hifi_test_h

#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtCore/QRegularExpression>

#include "ImageComparer.h"
#include "ui/MismatchWindow.h"

class Test {
public: 
    Test();

    void evaluateTests();
    void createRecursiveScript();
    void createTest();

    void createListOfAllJPEGimagesInDirectory();

    bool isInSnapshotFilenameFormat(QString filename);
    bool isInExpectedImageFilenameFormat(QString filename);

    void importTest(QTextStream& textStream, const QString& testPathname, int testNumber);

private:
    QMessageBox messageBox;

    QString pathToImageDirectory;
    QDir imageDirectory;
    QStringList sortedImageFilenames;

    QRegularExpression snapshotFilenameFormat;
    QRegularExpression expectedImageFilenameFormat;

    MismatchWindow mismatchWindow;

    ImageComparer imageComparer;
};

#endif // hifi_test_h
