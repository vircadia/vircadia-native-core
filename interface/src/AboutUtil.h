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
 * The <code>HifiAbout</code> API provides information about the version of Interface that is currently running. It also
 * has the functionality to open a web page in an Interface browser window.
 *
 * @namespace HifiAbout
 *
 * @hifi-interface
 * @hifi-client-entity
 * @hifi-avatar
 *
 * @property {string} buildDate - The build date of Interface that is currently running. <em>Read-only.</em>
 * @property {string} buildVersion - The build version of Interface that is currently running. <em>Read-only.</em>
 * @property {string} qtVersion - The Qt version used in Interface that is currently running. <em>Read-only.</em>
 *
 * @example <caption>Report build information for the version of Interface currently running.</caption>
 * print("HiFi build date: " + HifiAbout.buildDate);  // Returns the build date of the version of Interface currently running on your machine.
 * print("HiFi version: " + HifiAbout.buildVersion);  // Returns the build version of Interface currently running on your machine.
 * print("Qt version: " + HifiAbout.qtVersion);  // Returns the Qt version details of the version of Interface currently running on your machine.
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
     * Display a web page in an Interface browser window.
     * @function HifiAbout.openUrl
     * @param {string} url - The URL of the web page you want to view in Interface.
     */
    void openUrl(const QString &url) const;
private:
    AboutUtil(QObject* parent = nullptr);
    QString _dateConverted;
};

#endif // hifi_AboutUtil_h
