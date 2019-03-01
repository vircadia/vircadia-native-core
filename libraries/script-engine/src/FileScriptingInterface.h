//
//  FileScriptingInterface.h
//  libraries/script-engine/src
//
//  Created by Elisa Lupin-Jimenez on 6/28/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_FileScriptingInterface_h
#define hifi_FileScriptingInterface_h

#include <QtCore/QObject>
#include <QFileInfo>
#include <QString>

/**jsdoc
 * @namespace File
 *
 * @hifi-interface
 * @hifi-client-entity
 * @hifi-avatar
 * @hifi-server-entity
 * @hifi-assignment-client
 */

class FileScriptingInterface : public QObject {
    Q_OBJECT

public:
    FileScriptingInterface(QObject* parent);

public slots:

    /**jsdoc
     * @function File.convertUrlToPath
     * @param {string} url
     * @returns {string}
     */
    QString convertUrlToPath(QUrl url);

    /**jsdoc
     * @function File.runUnzip
     * @param {string} path
     * @param {string} url
     * @param {boolean} autoAdd
     * @param {boolean} isZip
     * @param {boolean} isBlocks
     */
    void runUnzip(QString path, QUrl url, bool autoAdd, bool isZip, bool isBlocks);

    /**jsdoc
     * @function File.getTempDir
     * @returns {string}
     */
    QString getTempDir();

signals:

    /**jsdoc
     * @function File.unzipResult
     * @param {string} zipFile
     * @param {string} unzipFile
     * @param {boolean} autoAdd
     * @param {boolean} isZip
     * @param {boolean} isBlocks
     * @returns {Signal}
     */
    void unzipResult(QString zipFile, QStringList unzipFile, bool autoAdd, bool isZip, bool isBlocks);

private:
    bool isTempDir(QString tempDir);
    bool hasModel(QStringList fileList);
    QStringList unzipFile(QString path, QString tempDir);
    void recursiveFileScan(QFileInfo file, QString* dirName);
    void downloadZip(QString path, const QString link);

};

#endif // hifi_FileScriptingInterface_h