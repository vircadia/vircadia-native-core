pragma Singleton
import QtQuick 2.6

Item {
    id: singleton
    readonly property string main: "main"
    readonly property string project: "project"
    readonly property string createProject: "createProject"
    readonly property string projectUpload: "projectUpload"
}
