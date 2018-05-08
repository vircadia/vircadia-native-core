import QtQuick 2.1;
import QtQuick.Window 2.1;

MouseArea {
    id: base;
    opacity: 0.65;
    // anchors.fill: parent;
    width: 400;
    height: 300;

    drag.target: list;
    onWheel: { }

    onClicked: { object = null }
    property var object: null
    onObjectChanged: {
        visible = (object != null)
    }

    property var properties: []
    onPropertiesChanged: {
        console.debug('properties: ', JSON.stringify(properties, 4, 0))
    }

    function getPropertiesList(obj) {
        var props = [];
        var propertiesObject = obj;
        if(properties.length !== 0) {
            propertiesObject = {};
            for(var i = 0; i < properties.length; ++i) {
                propertiesObject[properties[i]] = properties[i];
            }
        }

        for(var prop in propertiesObject) {

            var info = {'name' : prop};
            var value = obj[prop];
            var typeOfValue = typeof(value);

            if(typeof(value) === 'string') {
                info['type'] = 'string'
            } else if(typeof(value) === 'number') {
                if(Number.isInteger(value))
                    info['type'] = 'int'
                else
                    info['type'] = 'float'
            } else if(typeof(value) === 'boolean') {
                info['type'] = 'boolean'
            } else if(typeof(value) === 'function') {
                continue;
            }

            /*
            if(prop !== 'parent' && prop !== 'data' && prop !== 'children')
                console.debug('typeof(value): ', typeof(value), JSON.stringify(value, null, 4));
            */

            info['subName'] = ''
            props.push(info);
        }

        return props;
    }

    Rectangle {
        color: "lightgray";
        anchors.fill: list;
        anchors.margins: -50;
    }
    ListView {
        id: list;
        x: 50;
        y: 50;
        width: 400;
        height: 300;
        spacing: 5;
        model: object !== null ? getPropertiesList(object) : [];
        header: Text {
            text: object !== null ? object.toString () : '';
            font.bold: true;
            font.pixelSize: 20;
        }
        delegate: Row {
            spacing: 20;

            Column {
                width: 180;

                Text {
                    text: (modelData ["subName"] !== "" ? (modelData ["name"] + "." + modelData ["subName"]) : modelData ["name"]);
                    font.pixelSize: 16;
                }
            }
            Column {
                width: 200;

                Text {
                    text: {
                        return modelData ["type"]
                    }
                    font.pixelSize: 10;
                }
                TextInput {
                    id: input;
                    text: display;
                    width: parent.width;
                    font.pixelSize: 16;
                    font.underline: (text !== display);
                    Keys.onReturnPressed: { save (); }
                    Keys.onEnterPressed:  { save (); }
                    Keys.onEscapePressed: { cancel (); }

                    property string display : "";

                    function save () {
                        var tmp;
                        switch (modelData ["type"]) {
                        case 'boolean':
                            tmp = (text === "true" || text === "1");
                            break;
                        case 'float':
                            tmp = parseFloat (text);
                            break;
                        case 'int':
                            tmp = parseInt (text);
                            break;
                        case 'string':
                            tmp = text;
                            break;

                        default:
                            break;
                        }
                        if (modelData ["subName"] !== "") {
                            object [modelData ["name"]][modelData ["subName"]] = tmp;
                        }
                        else {
                            object [modelData ["name"]] = tmp;
                        }
                        text = display;
                    }

                    function cancel () {
                        text = display;
                    }

                    Binding on text    { value: input.display; }
                    Binding on display {
                        value: {
                            var ret = (modelData ["subName"] !== ""
                                       ? object [modelData ["name"]][modelData ["subName"]]
                                       : object [modelData ["name"]]);
                            return ret.toString ();
                        }
                    }
                    Rectangle {
                        z: -1;
                        color: "white";
                        anchors.fill: parent;
                    }
                }
            }
        }
    }
}
