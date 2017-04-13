//
//  DomainBaker.h
//  tools/oven/src
//
//  Created by Stephen Birarda on 4/12/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_DomainBaker_h
#define hifi_DomainBaker_h

#include <QtCore/QJsonArray>
#include <QtCore/QObject>
#include <QtCore/QUrl>

#include <FBXBaker.h>

class DomainBaker : public QObject {
    Q_OBJECT
public:
    DomainBaker(const QUrl& localEntitiesFileURL, const QString& domainName,
                const QString& baseOutputPath, const QUrl& destinationPath);

public:
    void bake();

signals:
    void finished();
    void allModelsFinished();

private slots:
    void handleFinishedBaker();

private:
    void setupOutputFolder();
    void loadLocalFile();
    void enumerateEntities();
    void writeNewEntitiesFile();

    QUrl _localEntitiesFileURL;
    QString _domainName;
    QString _baseOutputPath;
    QString _uniqueOutputPath;
    QString _contentOutputPath;
    QUrl _destinationPath;

    QJsonArray _entities;

    QHash<QUrl, QSharedPointer<FBXBaker>> _bakers;
    QMultiHash<QUrl, QJsonValueRef> _entitiesNeedingRewrite;
};

#endif // hifi_DomainBaker_h
