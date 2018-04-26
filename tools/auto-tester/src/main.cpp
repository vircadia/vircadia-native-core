//
//  main.cpp
//
//  Created by Nissim Hadar on 2 Nov 2017.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include <QtWidgets/QApplication>
#include "ui/AutoTester.h"

AutoTester* autoTester;

int main(int argc, char *argv[]) {
    QApplication application(argc, argv);

    autoTester = new AutoTester();
    autoTester->show();

    return application.exec();
}
