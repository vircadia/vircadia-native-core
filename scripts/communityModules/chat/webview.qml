import QtQuick 2.7
import QtQuick.Controls 2.2
import QtWebChannel 1.0
import controls 1.0
import hifi.toolbars 1.0
import QtGraphicalEffects 1.0
import controlsUit 1.0 as HifiControls
import stylesUit 1.0


Rectangle {
    width: parent.width
    height: parent.height

    signal sendToScript(var message);
    color: "#00000000"
    property alias thing: thing

    function sendMessage(text){
        sendToScript(text);
    }

    function fromScript(message) {
        console.log("fromScript "+message);
        var data = {failed:true};
        try{
            data = JSON.parse(message);
        } catch(e){
            //
        }
        if(!data.failed){
            if(data.cmd){
                JSConsole.executeCommand(data.msg);
            }
            console.log(data.visible);
            if(data.visible){
                thing.visible = true;
                textArea.focus = true;
            } else if(!data.visible){
                thing.visible = false;
                textArea.focus = false;
            }
        }
    }

    WebView {
        id: overlayWindow
        url: Qt.resolvedUrl("FloofChat2.html?appUUID="+QUrlQuery::queryItemValue()
        enabled: true
        blurOnCtrlShift: false
    }
}