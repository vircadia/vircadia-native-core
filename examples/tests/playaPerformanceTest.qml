import QtQuick 2.5
import QtQuick.Controls 1.4

Rectangle {
    id: root
    width: parent ? parent.width : 100
    height: parent ? parent.height : 100
            
    signal sendToScript(var message);
    property var values: [];
    property var host: AddressManager.hostname


    Component.onCompleted: {
        Window.domainChanged.connect(function(newDomain){
            if (newDomain !== root.host) {
                root.host = AddressManager.hostname;
            }
        });
    }

    onHostChanged: {
        if (root.running) {
            if (host !== "Dreaming" && host !== "Playa") {
                return;
            }

            console.log("PERF new domain " + host)
            if (host === "Dreaming") {
                AddressManager.handleLookupString("Playa");
                return;
            }

            if (host === "Playa") {
                console.log("PERF starting timers and frame timing");
                // If we've arrived, start running the test
                FrameTimings.start();
                rotationTimer.start();
                stopTimer.start();
            }
        }
    }

    function startTest() {
        console.log("PERF startTest()");
        root.running = true
        console.log("PERF current host: " + AddressManager.hostname)
        // If we're already in playa, we need to go somewhere else...
        if ("Playa" === AddressManager.hostname) {
            console.log("PERF Navigating to dreaming")
            AddressManager.handleLookupString("Dreaming/0,0,0");
        } else {
            console.log("PERF Navigating to playa")
            AddressManager.handleLookupString("Playa");
        }
    }

    function stopTest() {
        console.log("PERF stopTest()");
        root.running = false;
        stopTimer.stop();
        rotationTimer.stop();
        FrameTimings.finish();
        root.values = FrameTimings.getValues();
        AddressManager.handleLookupString("Dreaming/0,0,0");
        resultGraph.requestPaint();
        console.log("PERF Value Count: " + root.values.length);
        console.log("PERF Max: " + FrameTimings.max);
        console.log("PERF Min: " + FrameTimings.min);
        console.log("PERF Avg: " + FrameTimings.mean);
        console.log("PERF StdDev: " + FrameTimings.standardDeviation);
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

    property bool running: false

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
        anchors { left: parent.left; right: parent.right; }
        spacing: 8
        Button {
            text: root.running ? "Stop" : "Run"
            onClicked: root.running ? stopTest() : startTest();
        }
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
        anchors { left: parent.left; right: parent.right; top: row.bottom; margins: 16; bottom: parent.bottom; }
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


