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

#include "Bookmarks.h"

class LocationBookmarks: public Bookmarks {
    Q_OBJECT

public:
    LocationBookmarks();

    void setupMenus(Menu* menubar, MenuWrapper* menu) override;
    static const QString HOME_BOOKMARK;

public slots:
    void addBookmark();

protected:
    void addBookmarkToMenu(Menu* menubar, const QString& name, const QString& address) override;

private slots:
    void setHomeLocation();
    void teleportToBookmark();
    
private:
    const QString LocationBookmarks_FILENAME = "bookmarks.json";
};

#endif // hifi_LocationBookmarks_h
