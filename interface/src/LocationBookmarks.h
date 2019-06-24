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

/**jsdoc
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

    /**jsdoc
     * @function LocationBookmarks.getAddress
     * @param {string} bookmarkName Name of the bookmark to get the address for.
     * @returns {string} The url for the specified bookmark. If the bookmark does not exist, the empty string will be returned.
     */
    Q_INVOKABLE QString getAddress(const QString& bookmarkName);

public slots:

    /**jsdoc
     * @function LocationBookmarks.addBookmark
     */
    void addBookmark();

    /**jsdoc
     * @function LocationBookmarks.setHomeLocationToAddress
     * @param {string} address
     */
    void setHomeLocationToAddress(const QVariant& address);

    /**jsdoc
     * @function LocationBookmarks.getHomeLocationAddress
     * @returns {string} The url for the home location bookmark
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
