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

/// @addtogroup ScriptEngine
/// @{

#ifndef hifi_FileScriptingInterface_h
#define hifi_FileScriptingInterface_h

#include <QtCore/QObject>
#include <QFileInfo>
#include <QString>

/*@jsdoc
 * The <code>File</code> API provides some facilities for working with the file system.
 *
 * @namespace File
 *
 * @hifi-interface
 * @hifi-client-entity
 * @hifi-avatar
 * @hifi-server-entity
 * @hifi-assignment-client
 */
/// Provides the <code><a href="https://apidocs.vircadia.dev/File.html">File</a></code> scripting API
class FileScriptingInterface : public QObject {
    Q_OBJECT

public:
    FileScriptingInterface(QObject* parent);

public slots:

    /*@jsdoc
     * Extracts a filename from a URL, where the filename is specified in the query part of the URL as <code>filename=</code>.
     * @function File.convertUrlToPath
     * @param {string} url - The URL to extract the filename from.
     * @returns {string} The filename specified in the URL; an empty string if no filename is specified.
     * @example <caption>Extract a filename from a URL.</caption>
     * var url = "http://domain.tld/path/page.html?filename=file.ext";
     * print("File name: " + File.convertUrlToPath(url));  // file.ext
     */
    QString convertUrlToPath(QUrl url);

    /*@jsdoc
     * Unzips a file in the local file system to a new, unique temporary directory.
     * @function File.runUnzip
     * @param {string} path - The path of the zip file in the local file system. May have a leading <code>"file:///"</code>. 
     *     Need not have a <code>".zip"</code> extension if it is in a temporary directory (as created by 
     *     {@link File.getTempDir|getTempDir}).
     * @param {string} url - <em>Not used.</em>
     * @param {boolean} autoAdd - <em>Not used by user scripts.</em> The value is simply passed through to the 
     *     {@link File.unzipResult|unzipResult} signal.
     * @param {boolean} isZip - Set to <code>true</code> if <code>path</code> has a <code>".zip"</code> extension, 
     *     <code>false</code> if it doesn't (but should still be treated as a zip file).
     * @param {boolean} isBlocks - <em>Not used by user scripts.</em> The value is simply passed through to the 
     *     {@link File.unzipResult|unzipResult} signal.
     * @example <caption>Select and unzip a file.</caption>
     * File.unzipResult.connect(function (zipFile, unzipFiles, autoAdd, isZip, isBlocks) {
     *     print("File.unzipResult()");
     *     print("- zipFile: " + zipFile);
     *     print("- unzipFiles(" + unzipFiles.length + "): " + unzipFiles);
     *     print("- autoAdd: " + autoAdd);
     *     print("- isZip: " + isZip);
     *     print("- isBlocks: " + isBlocks);
     * });
     * 
     * var zipFile = Window.browse("Select a Zip File", "", "*.zip");
     * if (zipFile) {
     *     File.runUnzip(zipFile, "", false, true, false);
     * } else {
     *     print("Zip file not selected.");
     * }
     */
    void runUnzip(QString path, QUrl url, bool autoAdd, bool isZip, bool isBlocks);

    /*@jsdoc
     * Creates a new, unique directory for temporary use.
     * @function File.getTempDir
     * @returns {string} The path of the newly created temporary directory.
     * @example <caption>Create a temporary directory.</caption>
     * print("New temporary directory: " + File.getTempDir());
     */
    QString getTempDir();

signals:

    /*@jsdoc
     * Triggered when {@link File.runUnzip|runUnzip} completes.
     * @function File.unzipResult
     * @param {string} zipFile - The file that was unzipped.
     * @param {string[]} unzipFiles - The paths of the unzipped files in a newly created temporary directory. Includes entries 
     *     for any subdirectories created. An empty array if the <code>zipFile</code> could not be unzipped.
     * @param {boolean} autoAdd - The value that {@link File.runUnzip|runUnzip} was called with.
     * @param {boolean} isZip - <code>true</code> if {@link File.runUnzip|runUnzip} was called with <code>isZip == true</code>,  
     *     unless there is no FBX or OBJ file in the unzipped file(s) in which case the value is <code>false</code>.
     * @param {boolean} isBlocks - The value that {@link File.runUnzip|runUnzip} was called with.
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

/// @}
