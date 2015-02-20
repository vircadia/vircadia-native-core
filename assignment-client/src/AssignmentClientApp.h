//
//  AssignmentClientapp.h
//  assignment-client/src
//
//  Created by Seth Alves on 2/19/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QApplication>

class AssignmentClientApp : public QApplication {
    Q_OBJECT
public:
    AssignmentClientApp(int argc, char* argv[]);
    ~AssignmentClientApp();
};
