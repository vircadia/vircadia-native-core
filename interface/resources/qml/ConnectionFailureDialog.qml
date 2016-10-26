import QtQuick.Dialogs 1.2 as OriginalDialogs

import "dialogs"

MessageDialog {
    id: root
    objectName: "ConnectionFailureDialog"

    title: "No Connection"
    text: "Unable to connect to this domain. Click the 'GO TO' button on the toolbar to visit another domain."
    buttons: OriginalDialogs.StandardButton.Ok
    icon: OriginalDialogs.StandardIcon.Warning
    defaultButton: OriginalDialogs.StandardButton.NoButton;
}
