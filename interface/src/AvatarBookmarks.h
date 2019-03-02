//
//  AvatarBookmarks.h
//  interface/src
//
//  Created by Triplelexx on 23/03/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AvatarBookmarks_h
#define hifi_AvatarBookmarks_h

#include <DependencyManager.h>
#include "Bookmarks.h"

/**jsdoc 
 * This API helps manage adding and deleting avatar bookmarks.
 * @namespace AvatarBookmarks
 *
 * @hifi-interface
 * @hifi-client-entity
 * @hifi-avatar
 *
 */

class AvatarBookmarks: public Bookmarks, public  Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:
    AvatarBookmarks();
    void setupMenus(Menu* menubar, MenuWrapper* menu) override {};
    Q_INVOKABLE QVariantMap getBookmark(const QString& bookmarkName);

public slots:
    /**jsdoc 
     * Add the current Avatar to your avatar bookmarks.
     * @function AvatarBookmarks.addBookMark
     */
    void addBookmark(const QString& bookmarkName);
    void saveBookmark(const QString& bookmarkName);
    void loadBookmark(const QString& bookmarkName);
    void removeBookmark(const QString& bookmarkName);
    void updateAvatarEntities(const QVariantList& avatarEntities);
    QVariantMap getBookmarks() { return _bookmarks; }

signals:
    /**jsdoc
     * This function gets triggered after avatar loaded from bookmark
     * @function AvatarBookmarks.bookmarkLoaded
     * @param {string} bookmarkName
     * @returns {Signal}
     */
    void bookmarkLoaded(const QString& bookmarkName);

    /**jsdoc
     * This function gets triggered after avatar bookmark deleted
     * @function AvatarBookmarks.bookmarkDeleted
     * @param {string} bookmarkName
     * @returns {Signal}
     */
    void bookmarkDeleted(const QString& bookmarkName);

    /**jsdoc
     * This function gets triggered after avatar bookmark added
     * @function AvatarBookmarks.bookmarkAdded
     * @param {string} bookmarkName
     * @returns {Signal}
     */
    void bookmarkAdded(const QString& bookmarkName);

protected:
    void addBookmarkToMenu(Menu* menubar, const QString& name, const QVariant& bookmark) override {};
    void readFromFile() override;
    QVariantMap getAvatarDataToBookmark();

private:
    const QString AVATARBOOKMARKS_FILENAME = "avatarbookmarks.json";
    const QString ENTRY_AVATAR_URL = "avatarUrl";
    const QString ENTRY_AVATAR_ATTACHMENTS = "attachments";
    const QString ENTRY_AVATAR_ENTITIES = "avatarEntites";
    const QString ENTRY_AVATAR_SCALE = "avatarScale";
    const QString ENTRY_VERSION = "version";

    const int AVATAR_BOOKMARK_VERSION = 3;
};

#endif // hifi_AvatarBookmarks_h
