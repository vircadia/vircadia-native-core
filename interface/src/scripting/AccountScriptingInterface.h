//
//  AccountScriptingInterface.h
//  interface/src/scripting
//
//  Created by Stojce Slavkovski on 6/07/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AccountScriptingInterface_h
#define hifi_AccountScriptingInterface_h

#include <QObject>

class AccountScriptingInterface : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString username READ getUsername NOTIFY usernameChanged)

    /**jsdoc
     * @namespace Account
     * @property username {String} username if user is logged in, otherwise it returns "Unknown user"
     */

signals:

    /**jsdoc
     * Triggered when username has changed.
     * @function Account.usernameChanged
     * @return {Signal}
     */
    void usernameChanged();

public slots:
    static AccountScriptingInterface* getInstance();

    /**jsdoc
     * Returns the username for the currently logged in High Fidelity metaverse account.
     * @function Account.getUsername
     * @return {string} username if user is logged in, otherwise it returns "Unknown user"
     */
    QString getUsername();

    /**jsdoc
     * Determine if the user is logged into the High Fidleity metaverse.
     * @function Account.isLoggedIn
     * @return {bool} true when user is logged into the High Fidelity metaverse.
     */
    bool isLoggedIn();
    bool checkAndSignalForAccessToken();
};

#endif // hifi_AccountScriptingInterface_h
