//
//  TestSuiteCreator.h
//
//  Created by Nissim Hadar on 6 Jul 2018.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_test_suite_creator_h
#define hifi_test_suite_creator_h

#include <QDirIterator>
#include <QtXml/QDomDocument>
#include <QString>

class TestSuiteCreator {
public:
    void createTestSuite(const QString& testDirectory, const QString& user, const QString& branch);

    QDomElement processDirectory(const QString& directory, const QString& user, const QString& branch, const QDomElement& element);
    QDomElement processTest(const QString& fullDirectory, const QString& test, const QString& user, const QString& branch, const QDomElement& element);

private:
    QDomDocument document;
};

#endif // hifi_test_suite_creator_h