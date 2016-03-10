import Hifi 1.0
import QtQuick 2.3
import QtQuick.Controls 1.2

Item {
    id: root
    clip: true

    property int dumpDepth: 0;


    function dumpObject(object) {
        var keys = Object.keys(object);
        var tabsString = "";
        for (var j = 0; j < dumpDepth; ++j) {
            tabsString = tabsString + "\t";
        }

        for (var i = 0; i < keys.length; ++i) {
            var key = keys[i];
            var value = object[key];
            console.log(tabsString + "OVERLAY Key " + key + " (" + typeof(value) + "): " + value);
            if (typeof(value) === "object") {
                ++dumpDepth;
                dumpObject(value)
                --dumpDepth;
            }
        }
    }
}
