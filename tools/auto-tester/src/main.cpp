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

#include <iostream>

AutoTester* autoTester;

int main(int argc, char *argv[]) {
    // If no parameters then run in interactive mode
    // Parameter --testFolder <folder containing the test images>
    // Parameter --branch <branch on GitHub>
    //      default is "master"
    //
    QString testFolder;
    QString branch;
    if (argc > 2) {
        for (int i = 1; i < argc - 1; ++i)
        if (QString(argv[i]) == "--testFolder") {
            ++i;
            if (i < argc) {
                testFolder = QString(argv[i]);
            } else {
                std::cout << "Missing parameter after --testFolder" << endl;
                exit(-1);
            }
        } else if (QString(argv[i]) == "--branch") {
            ++i;
            if (i < argc) {
                branch = QString(argv[i]);
            } else {
                std::cout << "Missing parameter after --branch" << endl;
                exit(-1);
            }
        } else {
            std::cout << "Unknown parameter" << endl;
            exit(-1);
        }
    }

    QApplication application(argc, argv);

    autoTester = new AutoTester();
    autoTester->setup();

    if (!testFolder.isNull()) {
        autoTester->runFromCommandLine(testFolder, branch);
    } else {
        autoTester->show();
    }

    return application.exec();
}
