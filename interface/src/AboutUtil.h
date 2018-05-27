//
//  AboutUtil.h
//  interface/src
//
//  Created by Vlad Stelmahovsky on 15/5/2018.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#ifndef hifi_AboutUtil_h
#define hifi_AboutUtil_h

#include <QObject>

/**jsdoc
 * @namespace HifiAbout
 *
 * @hifi-interface
 * @hifi-client-entity
 * 
 * @property {string} buildDate
 * @property {string} buildVersion
 * @property {string} qtVersion
 */

class AboutUtil : public QObject {

    Q_OBJECT

    Q_PROPERTY(QString buildDate READ buildDate CONSTANT)
    Q_PROPERTY(QString buildVersion READ buildVersion CONSTANT)
    Q_PROPERTY(QString qtVersion READ qtVersion CONSTANT)

    AboutUtil(QObject* parent = nullptr);
public:
    static AboutUtil* getInstance();
    ~AboutUtil() {}

    QString buildDate() const;
    QString buildVersion() const;
    QString qtVersion() const;

public slots:

    /**jsdoc
     * @function HifiAbout.openUrl
     * @param {string} url
     */
    void openUrl(const QString &url) const;
private:

    QString m_DateConverted;
};

#endif // hifi_AboutUtil_h
