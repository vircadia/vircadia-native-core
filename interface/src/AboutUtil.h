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
 * @hifi-avatar
 *
 * @property {string} buildDate
 * @property {string} buildVersion
 * @property {string} qtVersion
 */

class AboutUtil : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString buildDate READ getBuildDate CONSTANT)
    Q_PROPERTY(QString buildVersion READ getBuildVersion CONSTANT)
    Q_PROPERTY(QString qtVersion READ getQtVersion CONSTANT)
public:
    static AboutUtil* getInstance();
    ~AboutUtil() {}

    QString getBuildDate() const;
    QString getBuildVersion() const;
    QString getQtVersion() const;

public slots:

    /**jsdoc
     * @function HifiAbout.openUrl
     * @param {string} url
     */
    void openUrl(const QString &url) const;
private:
    AboutUtil(QObject* parent = nullptr);
    QString _dateConverted;
};

#endif // hifi_AboutUtil_h
