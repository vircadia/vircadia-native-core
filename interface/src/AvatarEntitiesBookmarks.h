//
//  AvatarEntitiesBookmarks.h
//  interface/src
//
//  Created by Dante Ruiz on 15/01/18.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AvatarEntitiesBookmarks_h
#define hifi_AvatarEntitiesBookmarks_h

#include <DependencyManager.h>
#include "Bookmarks.h"

class AvatarEntitiesBookmarks: public Bookmarks, public Dependency {
  Q_OBJECT
  SINGLETON_DEPENDENCY

public:
    AvatarEntitiesBookmarks();
    void setupMenus(Menu* menubar, MenuWrapper* menu) override;

public slots:
    void addBookmark();

protected:
    void addBookmarkToMenu(Menu* menubar, const QString& name, const QVariant& bookmark) override;
    void readFromFile() override;

private:
    const QString AVATAR_ENTITIES_BOOKMARKS_FILENAME = "AvatarEntitiesbookmarks.json";
    const QString AVATAR_ENTITIES_ATTACHMENTS = "AvatarEntitiesName";
    const QString ENTITY_PROPERTIES = "AvatarEntitiesData";
    const QString ENTRY_VERSION = "version";

    const int ATTACHMENT_BOOKMARK_VERSION = 1;

private slots:
  void applyBookmarkedAvatarEntities();
};

#endif
