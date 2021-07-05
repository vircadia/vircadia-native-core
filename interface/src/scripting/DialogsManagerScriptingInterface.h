//
//  DialogsManagerScriptingInterface.h
//  interface/src/scripting
//
//  Created by Zander Otavka on 7/17/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_DialogsManagerScriptInterface_h
#define hifi_DialogsManagerScriptInterface_h

#include <QObject>

/*@jsdoc
 * The <code>DialogsMamnager</code> API provides facilities to work with some key dialogs.
 *
 * @namespace DialogsManager
 *
 * @hifi-interface
 * @hifi-client-entity
 * @hifi-avatar
 */

class DialogsManagerScriptingInterface : public QObject {
    Q_OBJECT
public:
    DialogsManagerScriptingInterface();
    static DialogsManagerScriptingInterface* getInstance();

    /*@jsdoc
     * <em>Currently performs no action.</em>
     * @function DialogsManager.showFeed
     */
    Q_INVOKABLE void showFeed();

public slots:
    /*@jsdoc
     * Shows the "Goto" dialog.
     * @function DialogsManager.showAddressBar
     */
    void showAddressBar();

    /*@jsdoc
     * Hides the "Goto" dialog.
     * @function DialogsManager.hideAddressBar
     */
    void hideAddressBar();

    /*@jsdoc
     * Shows the login dialog.
     * @function DialogsManager.showLoginDialog
     */
    void showLoginDialog();

signals:
    /*@jsdoc
     * Triggered when the "Goto" dialog is opened or closed.
     * <p><strong>Warning:</strong> Currently isn't always triggered.</p>
     * @function DialogsManager.addressBarShown
     * @param {boolean} visible - <code>true</code> if the Goto dialog has been opened, <code>false</code> if it has been 
     *     closed.
     * @returns {Signal}
     */
    void addressBarShown(bool visible);
};

#endif
