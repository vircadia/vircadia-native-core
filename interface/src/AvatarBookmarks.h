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
 *
 */

class AvatarBookmarks: public Bookmarks, public  Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:
    AvatarBookmarks();
    void setupMenus(Menu* menubar, MenuWrapper* menu) override;


public slots:
    /**jsdoc 
     * Add the current Avatar to your avatar bookmarks.
     * @function AvatarBookmarks.addBookMark
     */
    void addBookmark();

protected:
    void addBookmarkToMenu(Menu* menubar, const QString& name, const QVariant& bookmark) override;
    void readFromFile() override;

private:
    const QString AVATARBOOKMARKS_FILENAME = "avatarbookmarks.json";
    const QString ENTRY_AVATAR_URL = "avatarUrl";
    const QString ENTRY_AVATAR_ATTACHMENTS = "attachments";
    const QString ENTRY_AVATAR_ENTITIES = "avatarEntites";
    const QString ENTRY_AVATAR_SCALE = "avatarScale";
    const QString ENTRY_VERSION = "version";

    const int AVATAR_BOOKMARK_VERSION = 3;

private slots:
    void changeToBookmarkedAvatar();
};

#endif // hifi_AvatarBookmarks_h
