import QtQuick 2.5
import QtQuick.Controls 1.4
import Qt.labs.settings 1.0

Rectangle {
    id: root
    width: parent ? parent.width : 100
    height: parent ? parent.height : 100
            
    signal sendToScript(var message);
    property var values: [];
    property alias destination: addressLine.text
    readonly property string nullDestination: "169.254.0.1"
    property bool running: false

    function statusReport() {
        console.log("PERF status connected: " + AddressManager.isConnected);
    }
    
    Timer {
        id: readyStateTimer
        interval: 500
        repeat: true
        running: false
        onTriggered: {
            if (!root.running) {
                stop();
                return;
            }

            if (AddressManager.isConnected) {
                console.log("PERF already connected, disconnecting");
                AddressManager.handleLookupString(root.nullDestination);
                return;
            }
            
            stop();
            console.log("PERF disconnected, moving to target " + root.destination);
            AddressManager.handleLookupString(root.destination);

            // If we've arrived, start running the test
            console.log("PERF starting timers and frame timing");
            FrameTimings.start();
            rotationTimer.start();
            stopTimer.start();
        }
    }


    function startTest() {
        console.log("PERF startTest()");
        if (!root.running) {
            root.running = true
            readyStateTimer.start();
        }
    }

    function stopTest() {
        console.log("PERF stopTest()");
        if (root.running) {
            root.running = false;
            stopTimer.stop();
            rotationTimer.stop();
            FrameTimings.finish();
            root.values = FrameTimings.getValues();
            AddressManager.handleLookupString(root.nullDestination);
            resultGraph.requestPaint();
            console.log("PERF Value Count: " + root.values.length);
            console.log("PERF Max: " + FrameTimings.max);
            console.log("PERF Min: " + FrameTimings.min);
            console.log("PERF Avg: " + FrameTimings.mean);
            console.log("PERF StdDev: " + FrameTimings.standardDeviation);
        }
    }

    function yaw(a) {
        var y = -Math.sin( a / 2.0 );
        var w = Math.cos( a / 2.0 );
        var l = Math.sqrt((y * y) + (w * w));
        return Qt.quaternion(w / l, 0, y / l, 0);
    }

    function rotate() {
        MyAvatar.setOrientationVar(yaw(Date.now() / 1000));
    }


    Timer {
        id: stopTimer
        interval: 30 * 1000
        repeat: false
        running: false
        onTriggered: stopTest();
    }

    Timer {
        id: rotationTimer
        interval: 100
        repeat: true
        running: false
        onTriggered: rotate();
    }

    Row {
        id: row
        anchors { left: parent.left; right: parent.right; top: parent.top; margins: 16 }
        spacing: 8
        Button {
            text: root.running ? "Stop" : "Run"
            onClicked: root.running ? stopTest() : startTest();
        }
        Button {
            text: "Disconnect"
            onClicked: AddressManager.handleLookupString(root.nullDestination);
        }
        Button {
            text: "Connect"
            onClicked: AddressManager.handleLookupString(root.destination);
        }
        Button {
            text: "Status"
            onClicked: statusReport();
        }
    }

    TextField {
        id: addressLine
        anchors { 
            left: parent.left; right: parent.right;
            top: row.bottom; margins: 16; 
        }
        text: "Playa"
        onTextChanged: console.log("PERF new target " + text);
    }

    Settings {
        category: "Qml.Performance.RenderTest"
        property alias destination: addressLine.text
    }


//    Rectangle {
//        anchors { left: parent.left; right: parent.right; top: row.bottom; topMargin: 8; bottom: parent.bottom; }
//        //anchors.fill: parent
//        color: "#7fff0000"
//    }

    // Return the maximum value from a set of values
    function vv(i, max) {
        var perValue = values.length / max;
        var start = Math.floor(perValue * i);
        var end = Math.min(values.length, Math.floor(start + perValue));
        var result = 0;
        for (var j = start; j <= end; ++j) {
            result = Math.max(result, values[j]);
        }
        return result;
    }

    Canvas {
        id: resultGraph
        anchors { left: parent.left; right: parent.right; top: addressLine.bottom; margins: 16; bottom: parent.bottom; }
        property real maxValue: 200;
        property real perFrame: 10000;
        property real k1: (5 / maxValue) * height;
        property real k2: (10 / maxValue) * height;
        property real k3: (100 / maxValue) * height;

        onPaint: {
            var ctx = getContext("2d");
            if (values.length === 0) {
                ctx.fillStyle = Qt.rgba(1, 0, 0, 1);
                ctx.fillRect(0, 0, width, height);
                return;
            }


            //ctx.setTransform(1, 0, 0, -1, 0, 0);
            ctx.fillStyle = Qt.rgba(0, 0, 0, 1);
            ctx.fillRect(0, 0, width, height);

            ctx.strokeStyle= "gray";
            ctx.lineWidth="1";
            ctx.beginPath();
            for (var i = 0; i < width; ++i) {
                var value = vv(i, width); //values[Math.min(i, values.length - 1)];
                value /= 10000;
                value /= maxValue;
                ctx.moveTo(i, height);
                ctx.lineTo(i, height - (height * value));
            }
            ctx.stroke();

            ctx.strokeStyle= "green";
            ctx.lineWidth="2";
            ctx.beginPath();
            var lineHeight = height - k1;
            ctx.moveTo(0, lineHeight);
            ctx.lineTo(width, lineHeight);
            ctx.stroke();

            ctx.strokeStyle= "yellow";
            ctx.lineWidth="2";
            ctx.beginPath();
            lineHeight = height - k2;
            ctx.moveTo(0, lineHeight);
            ctx.lineTo(width, lineHeight);
            ctx.stroke();

            ctx.strokeStyle= "red";
            ctx.lineWidth="2";
            ctx.beginPath();
            lineHeight = height - k3;
            ctx.moveTo(0, lineHeight);
            ctx.lineTo(width, lineHeight);
            ctx.stroke();

        }
    }
}

