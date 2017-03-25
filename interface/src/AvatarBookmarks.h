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

#include "Bookmarks.h"

class AvatarBookmarks: public Bookmarks {
    Q_OBJECT

public:
    AvatarBookmarks();

    void setupMenus(Menu* menubar, MenuWrapper* menu) override;

public slots:
    void addBookmark();

protected:
    void addBookmarkToMenu(Menu* menubar, const QString& name, const QString& address) override;

private slots:
    void changeToBookmarkedAvatar();

private:
    const QString AvatarBookmarks_FILENAME = "avatarbookmarks.json";
};

#endif // hifi_AvatarBookmarks_h
