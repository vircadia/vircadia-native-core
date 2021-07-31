//
//  GooglePolyScriptingInterface.h
//  interface/src/scripting
//
//  Created by Elisa Lupin-Jimenez on 12/3/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_GooglePolyScriptingInterface_h
#define hifi_GooglePolyScriptingInterface_h

#include <QObject>
#include <DependencyManager.h>

/*@jsdoc 
 * The GooglePoly API allows you to interact with Google Poly models direct from inside High Fidelity.
 * @namespace GooglePoly
 *
 * @hifi-interface
 * @hifi-client-entity
 * @hifi-avatar
 */

class GooglePolyScriptingInterface : public QObject, public Dependency {
    Q_OBJECT

public:
    GooglePolyScriptingInterface();

public slots:

    /*@jsdoc
     * @function GooglePoly.setAPIKey
     * @param {string} key
     */
    void setAPIKey(const QString& key);

    /*@jsdoc
     * @function GooglePoly.getAssetList
     * @param {string} keyword
     * @param {string} category
     * @param {string} format
     * @returns {string}
     */
    QString getAssetList(const QString& keyword, const QString& category, const QString& format);

    /*@jsdoc
     * @function GooglePoly.getFBX
     * @param {string} keyword
     * @param {string} category
     * @returns {string}
     */
    QString getFBX(const QString& keyword, const QString& category);

    /*@jsdoc
     * @function GooglePoly.getOBJ
     * @param {string} keyword
     * @param {string} category
     * @returns {string}
     */
    QString getOBJ(const QString& keyword, const QString& category);

    /*@jsdoc
     * @function GooglePoly.getBlocks
     * @param {string} keyword
     * @param {string} category
     * @returns {string}
     */
    QString getBlocks(const QString& keyword, const QString& category);

    /*@jsdoc
     * @function GooglePoly.getGLTF
     * @param {string} keyword
     * @param {string} category
     * @returns {string}
     */
    QString getGLTF(const QString& keyword, const QString& category);

    /*@jsdoc
     * @function GooglePoly.getGLTF2
     * @param {string} keyword
     * @param {string} category
     * @returns {string}
     */
    QString getGLTF2(const QString& keyword, const QString& category);

    /*@jsdoc
     * @function GooglePoly.getTilt
     * @param {string} keyword
     * @param {string} category
     * @returns {string}
     */
    QString getTilt(const QString& keyword, const QString& category);

    /*@jsdoc
     * @function GooglePoly.getModelInfo
     * @param {string} input
     * @returns {string}
     */
    QString getModelInfo(const QString& input);

private:
    QString _authCode;

    QUrl formatURLQuery(const QString& keyword, const QString& category, const QString& format);
    QString getModelURL(const QUrl& url);
    QByteArray getHTTPRequest(const QUrl& url);
    QVariant parseJSON(const QUrl& url, int fileType);
    int getRandIntInRange(int length);

};

#endif // hifi_GooglePolyScriptingInterface_h
