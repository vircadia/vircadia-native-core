//
//  BackupHandler.h
//  assignment-client
//
//  Created by Clement Brisset on 2/5/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_BackupHandler_h
#define hifi_BackupHandler_h

#include <memory>

#include <QDebug>

#include <quazip5/quazip.h>

class BackupHandler {
public:
    template <typename T>
    BackupHandler(T x) : _self(std::make_shared<Model<T>>(std::move(x))) {}

    void loadBackup(const QuaZip& zip) {
        _self->loadBackup(zip);
    }
    void createBackup(QuaZip& zip) const {
        _self->createBackup(zip);
    }
    void recoverBackup(const QuaZip& zip) const {
        _self->recoverBackup(zip);
    }
    void deleteBackup(const QuaZip& zip) {
        _self->deleteBackup(zip);
    }
    void consolidateBackup(QuaZip& zip) const {
        _self->consolidateBackup(zip);
    }

private:
    struct Concept {
        virtual ~Concept() = default;

        virtual void loadBackup(const QuaZip& zip) = 0;
        virtual void createBackup(QuaZip& zip) const = 0;
        virtual void recoverBackup(const QuaZip& zip) const = 0;
        virtual void deleteBackup(const QuaZip& zip) = 0;
        virtual void consolidateBackup(QuaZip& zip) const = 0;
    };

    template <typename T>
    struct Model : Concept {
        Model(T x) : data(std::move(x)) {}

        void loadBackup(const QuaZip& zip) {
            data.loadBackup(zip);
        }
        void createBackup(QuaZip& zip) const {
            data.createBackup(zip);
        }
        void recoverBackup(const QuaZip& zip) const {
            data.recoverBackup(zip);
        }
        void deleteBackup(const QuaZip& zip) {
            data.deleteBackup(zip);
        }
        void consolidateBackup(QuaZip& zip) const {
            data.consolidateBackup(zip);
        }

        T data;
    };

    std::shared_ptr<Concept> _self;
};

#include <quazip5/quazipfile.h>
class EntitiesBackupHandler {
public:
    EntitiesBackupHandler(QString entitiesFilePath) : _entitiesFilePath(entitiesFilePath) {}

    void loadBackup(const QuaZip& zip) {}

    void createBackup(QuaZip& zip) const {
        qDebug() << "Creating a backup from handler";

        QFile entitiesFile { _entitiesFilePath };

        if (entitiesFile.open(QIODevice::ReadOnly)) {
            QuaZipFile zipFile { &zip };
            zipFile.open(QIODevice::WriteOnly, QuaZipNewInfo("models.json.gz", _entitiesFilePath));
            zipFile.write(entitiesFile.readAll());
            zipFile.close();
            if (zipFile.getZipError() != UNZ_OK) {
                qDebug() << "testCreate(): outFile.close(): " << zipFile.getZipError();
            }
        }
    }

    void recoverBackup(const QuaZip& zip) const {}
    void deleteBackup(const QuaZip& zip) {}
    void consolidateBackup(QuaZip& zip) const {}

private:
    QString _entitiesFilePath;
};

#endif /* hifi_BackupHandler_h */
