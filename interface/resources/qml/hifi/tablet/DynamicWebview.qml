import QtQuick 2.0
import "../../controls" as Controls

Controls.WebView {
    id: root
    function fromScript(message) {
        root.url = message.url;
    }
}


