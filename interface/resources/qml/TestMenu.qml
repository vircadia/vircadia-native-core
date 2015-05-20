import QtQuick 2.4
import QtQuick.Controls 1.3
import Hifi 1.0

// Currently for testing a pure QML replacement menu
Item {
    Item {
        objectName: "AllActions"
        Action {
            id: aboutApp
            objectName: "HifiAction_" + MenuConstants.AboutApp
            text: qsTr("About Interface")
        }

        //
        // File Menu
        //
        Action {
            id: login
            objectName: "HifiAction_" + MenuConstants.Login
            text: qsTr("Login")
        }
        Action {
            id: quit
            objectName: "HifiAction_" + MenuConstants.Quit
            text: qsTr("Quit")
            //shortcut: StandardKey.Quit
            shortcut: "Ctrl+Q"
        }


        //
        // Edit menu
        //
        Action {
            id: undo
            text: "Undo"
            shortcut: StandardKey.Undo
        }

        Action {
            id: redo
            text: "Redo"
            shortcut: StandardKey.Redo
        }

        Action {
            id: animations
            objectName: "HifiAction_" + MenuConstants.Animations
            text: qsTr("Animations...")
        }
        Action {
            id: attachments
            text: qsTr("Attachments...")
        }
        Action {
            id: explode
            text: qsTr("Explode on quit")
            checkable: true
            checked: true
        }
        Action {
            id: freeze
            text: qsTr("Freeze on quit")
            checkable: true
            checked: false
        }
        ExclusiveGroup {
            Action {
                id: visibleToEveryone
                objectName: "HifiAction_" + MenuConstants.VisibleToEveryone
                text: qsTr("Everyone")
                checkable: true
                checked: true
            }
            Action {
                id: visibleToFriends
                objectName: "HifiAction_" + MenuConstants.VisibleToFriends
                text: qsTr("Friends")
                checkable: true
            }
            Action {
                id: visibleToNoOne
                objectName: "HifiAction_" + MenuConstants.VisibleToNoOne
                text: qsTr("No one")
                checkable: true
            }
        }
    }

    Menu {
        objectName: "rootMenu";
        Menu {
            title: "File"
            MenuItem { action: login }
            MenuItem { action: explode }
            MenuItem { action: freeze }
            MenuItem { action: quit }
        }
        Menu {
            title: "Tools"
            Menu {
                title: "I Am Visible To"
                MenuItem { action: visibleToEveryone }
                MenuItem { action: visibleToFriends }
                MenuItem { action: visibleToNoOne }
            }
            MenuItem { action: animations }
        }
        Menu {
            title: "Long menu name top menu"
            MenuItem { action: aboutApp }
        }
        Menu {
            title: "Help"
            MenuItem { action: aboutApp }
        }
    } 
}
