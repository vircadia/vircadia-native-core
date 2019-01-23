//
//  TestRunner.h
//
//  Created by Nissim Hadar on 23 Jan 2019.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_testRunner_h
#define hifi_testRunner_h

#include <QLabel>

class TestRunner {
public:
    void setWorkingFolder(QLabel* workingFolderLabel);

private:
    QString _workingFolder;
};
#endif
