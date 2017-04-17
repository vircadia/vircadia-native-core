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
#include <QtCore/QThread>

#include <Baker.h>
#include <FBXBaker.h>

class DomainBaker : public Baker {
    Q_OBJECT
public:
    // This is a real bummer, but the FBX SDK is not thread safe - even with separate FBXManager objects.
    // This means that we need to put all of the FBX importing/exporting from the same process on the same thread.
    // That means you must pass a usable running QThread when constructing a domain baker.
    DomainBaker(const QUrl& localEntitiesFileURL, const QString& domainName,
                const QString& baseOutputPath, const QUrl& destinationPath);

signals:
    void allModelsFinished();
    void bakeProgress(int modelsBaked, int modelsTotal);

private slots:
    virtual void bake() override;
    void handleFinishedBaker();

private:
    void setupOutputFolder();
    void loadLocalFile();
    void enumerateEntities();
    void checkIfRewritingComplete();
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

    int _totalNumberOfEntities { 0 };
};

#endif // hifi_DomainBaker_h
