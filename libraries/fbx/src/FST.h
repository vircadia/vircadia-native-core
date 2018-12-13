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

class FST : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString name READ getName WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QString modelPath READ getModelPath WRITE setModelPath NOTIFY modelPathChanged)
    Q_PROPERTY(QUuid marketplaceID READ getMarketplaceID)
public:
    FST(QString fstPath, QVariantHash data);

    QString absoluteModelPath() const;

    QString getName() const { return _name; }
    void setName(const QString& name);

    QString getModelPath() const { return _modelPath; }
    void setModelPath(const QString& modelPath);

    QUuid getMarketplaceID() const { return _marketplaceID; }

signals:
    void nameChanged(const QString& name);
    void modelPathChanged(const QString& modelPath);

private:
    QString _fstPath;

    QString _name{};
    QString _modelPath{};
    QUuid _marketplaceID{};

    QVariantHash _other{};
};

#endif  // hifi_FST_h
