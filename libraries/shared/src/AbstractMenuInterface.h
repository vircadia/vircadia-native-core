//
//  AbstractMenuInterface.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/16/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//

#ifndef __hifi__AbstractMenuInterface__
#define __hifi__AbstractMenuInterface__

class QMenu;
class QString;
class QObject;
class QKeySequence;
class QAction;

// these are actually class scoped enums, but we don't want to depend on the class for this abstract interface
const int NO_ROLE = 0;
typedef int QACTION_MENUROLE; 
typedef int QKEYSEQUENCE;

class AbstractMenuInterface {
public:
    virtual QMenu* getActiveScriptsMenu() = 0;
    virtual QAction* addActionToQMenuAndActionHash(QMenu* destinationMenu,
                                           const QString actionName,
                                           const QKEYSEQUENCE& shortcut = 0,
                                           const QObject* receiver = NULL,
                                           const char* member = NULL,
                                           QACTION_MENUROLE role = NO_ROLE) = 0;
    virtual void removeAction(QMenu* menu, const QString& actionName) = 0;
};

#endif /* defined(__hifi__AbstractMenuInterface__) */