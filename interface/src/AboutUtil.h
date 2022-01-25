//
//  AboutUtil.h
//  interface/src
//
//  Created by Vlad Stelmahovsky on 15/5/2018.
//  Copyright 2018 High Fidelity, Inc.
//  Copyright 2020 Vircadia Contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#ifndef hifi_AboutUtil_h
#define hifi_AboutUtil_h

#include <QObject>

/*@jsdoc
 * The <code>About</code> API provides information about the version of Interface that is currently running. It also has the
 * functionality to open a web page in an Interface browser window.
 *
 * @namespace About
 *
 * @hifi-interface
 * @hifi-client-entity
 * @hifi-avatar
 *
 * @property {string} platform - The name of the Interface platform running, e,g., <code>"Vircadia"</code> for the Vircadia.
 *     <em>Read-only.</em>
 * @property {string} buildDate - The build date of Interface that is currently running. <em>Read-only.</em>
 * @property {string} buildVersion - The build version of Interface that is currently running. <em>Read-only.</em>
 * @property {string} releaseName - The release codename of the version that Interface is currently running. <em>Read-only.</em>
 * @property {string} qtVersion - The Qt version used in Interface that is currently running. <em>Read-only.</em>
 *
 * @example <caption>Report information on the version of Interface currently running.</caption>
 * print("Interface platform: " + About.platform);
 * print("Interface build date: " + About.buildDate);
 * print("Interface version: " + About.buildVersion);
 * print("Interface release name: " + About.releaseName);
 * print("Qt version: " + About.qtVersion);
 */

 /*@jsdoc
 * The <code>HifiAbout</code> API provides information about the version of Interface that is currently running. It also
 * has the functionality to open a web page in an Interface browser window.
 *
 * @namespace HifiAbout
 *
 * @hifi-interface
 * @hifi-client-entity
 * @hifi-avatar
 *
 * @deprecated This API is deprecated and will be removed. Use the {@link About} API instead.
 *
 * @property {string} platform - The name of the Interface platform running, e,g., <code>"Vircadia"</code> for the Vircadia.
 *     <em>Read-only.</em>
 * @property {string} buildDate - The build date of Interface that is currently running. <em>Read-only.</em>
 * @property {string} buildVersion - The build version of Interface that is currently running. <em>Read-only.</em>
 * @property {string} qtVersion - The Qt version used in Interface that is currently running. <em>Read-only.</em>
 *
 * @borrows About.openUrl as openUrl
 */

class AboutUtil : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString platform READ getPlatformName CONSTANT)
    Q_PROPERTY(QString buildDate READ getBuildDate CONSTANT)
    Q_PROPERTY(QString buildVersion READ getBuildVersion CONSTANT)
    Q_PROPERTY(QString releaseName READ getReleaseName CONSTANT)
    Q_PROPERTY(QString qtVersion READ getQtVersion CONSTANT)
public:
    static AboutUtil* getInstance();
    ~AboutUtil() {}

    QString getPlatformName() const { return "Vircadia"; }
    QString getBuildDate() const;
    QString getBuildVersion() const;
    QString getReleaseName() const;
    QString getQtVersion() const;

public slots:

    /*@jsdoc
     * Display a web page in an Interface browser window or the tablet.
     * @function About.openUrl
     * @param {string} url - The URL of the web page you want to view in Interface.
     */
    void openUrl(const QString &url) const;
private:
    AboutUtil(QObject* parent = nullptr);
    QString _dateConverted;
};

#endif // hifi_AboutUtil_h
