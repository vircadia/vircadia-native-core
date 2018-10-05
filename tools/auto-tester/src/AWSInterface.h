//
//  AWSInterface.h
//
//  Created by Nissim Hadar on 3 Oct 2018.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AWSInterface_h
#define hifi_AWSInterface_h

#include <QObject>
#include <QTextStream>

class AWSInterface : public QObject {
    Q_OBJECT
public:
    explicit AWSInterface(QObject* parent = 0);

    void createWebPageFromResults(const QString& testResults, const QString& tempDirectory);

    void createHTMLFile(const QString& testResults, const QString& tempDirectory);

    void startHTMLpage(QTextStream& stream);
    void writeHead(QTextStream& stream);
    void writeBody(const QString& testResults, QTextStream& stream);
    void finishHTMLpage(QTextStream& stream);

    void writeTitle(const QString& testResults, QTextStream& stream);
    void writeTable(QTextStream& stream);
    void openTable(QTextStream& stream);
    void closeTable(QTextStream& stream);

    void createEntry(int index, const QString& testFailure, QTextStream& stream);

private:
    QString _tempDirectory;
    QString _resultsFolder;

    const QString FAILURE_FOLDER{ "failures" };
};

#endif  // hifi_AWSInterface_h