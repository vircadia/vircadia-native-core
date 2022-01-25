//
//  LocationBookmarks.h
//  interface/src
//
//  Created by Triplelexx on 23/03/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_LocationBookmarks_h
#define hifi_LocationBookmarks_h

#include <DependencyManager.h>

#include "Bookmarks.h"

/*@jsdoc
 * The <code>LocationBookmarks</code> API provides facilities for working with location bookmarks. A location bookmark 
 * associates a name with a metaverse address.
 *
 * @namespace LocationBookmarks
 *
 * @hifi-client-entity
 * @hifi-interface
 * @hifi-avatar
 */

class LocationBookmarks : public Bookmarks, public  Dependency  {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:
    LocationBookmarks();

    void setupMenus(Menu* menubar, MenuWrapper* menu) override;
    static const QString HOME_BOOKMARK;

    /*@jsdoc
     * Gets the metaverse address associated with a bookmark.
     * @function LocationBookmarks.getAddress
     * @param {string} bookmarkName - Name of the bookmark to get the metaverse address for (case sensitive).
     * @returns {string} The metaverse address for the bookmark. If the bookmark does not exist, <code>""</code> is returned.
     * @example <caption>Report the "Home" bookmark's metaverse address.</caption>
     * print("Home bookmark's address: " + LocationBookmarks.getAddress("Home"));
     */
    Q_INVOKABLE QString getAddress(const QString& bookmarkName);

public slots:

    /*@jsdoc
     * Prompts the user to bookmark their current location. The user can specify the name of the bookmark in the dialog that is 
     * opened.
     * @function LocationBookmarks.addBookmark
     */
    void addBookmark();

    /*@jsdoc
     * Sets the metaverse address associated with the "Home" bookmark.
     * @function LocationBookmarks.setHomeLocationToAddress
     * @param {string} address - The metaverse address to set the "Home" bookmark to.
     */
    void setHomeLocationToAddress(const QVariant& address);

    /*@jsdoc
     * Gets the metaverse address associated with the "Home" bookmark.
     * @function LocationBookmarks.getHomeLocationAddress
     * @returns {string} The metaverse address for the "Home" bookmark.
     */
    QString getHomeLocationAddress();

protected:
    void addBookmarkToMenu(Menu* menubar, const QString& name, const QVariant& address) override;

private:
    const QString LOCATIONBOOKMARKS_FILENAME = "bookmarks.json";

private slots:
    void setHomeLocation();
    void teleportToBookmark();
};

#endif // hifi_LocationBookmarks_h
