//
//  FST.h
//
//  Created by Ryan Huffman on 12/11/15.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_FST_h
#define hifi_FST_h

#include <QVariantHash>
#include <QUuid>
#include "FSTReader.h"

namespace hfm {
    class Model;
};

class FST : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString name READ getName WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QString modelPath READ getModelPath WRITE setModelPath NOTIFY modelPathChanged)
    Q_PROPERTY(QUuid marketplaceID READ getMarketplaceID)
    Q_PROPERTY(bool hasMarketplaceID READ getHasMarketplaceID NOTIFY marketplaceIDChanged)
public:
    FST(QString fstPath, QMultiHash<QString, QVariant> data);

    static FST* createFSTFromModel(const QString& fstPath, const QString& modelFilePath, const hfm::Model& hfmModel);

    QString absoluteModelPath() const;

    QString getName() const { return _name; }
    void setName(const QString& name);

    QString getModelPath() const { return _modelPath; }
    void setModelPath(const QString& modelPath);

    Q_INVOKABLE bool getHasMarketplaceID() const { return !_marketplaceID.isNull(); }
    QUuid getMarketplaceID() const { return _marketplaceID; }
    void setMarketplaceID(QUuid marketplaceID);

    QStringList getScriptPaths() const { return _scriptPaths; }
    void setScriptPaths(QStringList scriptPaths) { _scriptPaths = scriptPaths; }

    QString getPath() const { return _fstPath; }

    QMultiHash<QString, QVariant> getMapping() const;

    bool write();

signals:
    void nameChanged(const QString& name);
    void modelPathChanged(const QString& modelPath);
    void marketplaceIDChanged();

private:
    QString _fstPath;

    QString _name{};
    QString _modelPath{};
    QUuid _marketplaceID{};

    QStringList _scriptPaths{};

    QVariantHash _other{};
};

#endif  // hifi_FST_h
