//
//  PropGroup.qml
//
//  Created by Sam Gateau on 3/2/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.7

// PropGroup is mostly reusing the look and feel of the PropFolderPanel
// It is populated by calling "updatePropItems"
// or adding manually new Items to the "propItemsPanel"
PropFolderPanel {    
    Global { id: global }
    id: root
    
    property var rootObject: {}

    property alias propItemsPanel: root.panelFrameContent
    
    // Prop Group is designed to author an array of ProItems, they are defined with an array of the tuplets describing each individual item:
    // [ ..., PropItemInfo, ...]
    // PropItemInfo {
    //    type: "PropXXXX", object: JSobject, property: "propName"      
    // }
    //
    function updatePropItems(propItemsContainer, propItemsModel) {
        root.hasContent = false
        for (var i = 0; i < propItemsModel.length; i++) {
            var proItem = propItemsModel[i];
            // valid object
            if (proItem['object'] !== undefined && proItem['object'] !== null ) {
                // valid property
                if (proItem['property'] !== undefined && proItem.object[proItem.property] !== undefined) {
                    // check type
                    if (proItem['type'] === undefined) {
                        proItem['type'] = typeof(proItem.object[proItem.property])
                    }
                    switch(proItem.type) {
                        case 'string':
                        case 'PropString': {                      
                            var component = Qt.createComponent("PropString.qml");
                            component.createObject(propItemsContainer, {
                                "label": proItem.property,
                                "object": proItem.object,
                                "property": proItem.property
                            })
                        } break;
                        case 'boolean':
                        case 'PropBool': {
                            var component = Qt.createComponent("PropBool.qml");
                            component.createObject(propItemsContainer, {
                                "label": proItem.property,
                                "object": proItem.object,
                                "property": proItem.property
                            })
                        } break;
                        case 'number':
                        case 'PropScalar': {
                            var component = Qt.createComponent("PropScalar.qml");
                            component.createObject(propItemsContainer, {
                                "label": proItem.property,
                                "object": proItem.object,
                                "property": proItem.property,
                                "min": (proItem["min"] !== undefined ? proItem.min : 0.0),                   
                                "max": (proItem["max"] !== undefined ? proItem.max : 1.0),                                       
                                "integer": (proItem["integral"] !== undefined ? proItem.integral : false),
                                "readOnly": (proItem["readOnly"] !== undefined ?  proItem["readOnly"] : true),
                            })
                        } break;
                        case 'PropEnum': {
                            var component = Qt.createComponent("PropEnum.qml");
                            component.createObject(propItemsContainer, {
                                "label": proItem.property,
                                "object": proItem.object,
                                "property": proItem.property,
                                "enums": (proItem["enums"] !== undefined ? proItem.enums : ["Undefined Enums !!!"]), 
                            })
                        } break;
                        case 'object': {
                            console.log('Item is an object, create PropGroup: ' + JSON.stringify(proItem.object[proItem.property]));
                            var itemRootObject = proItem.object[proItem.property];
                            var itemLabel = proItem.property;
                            var itemDepth = root.indentDepth + 1;
                            if (Array.isArray(itemRootObject)) {
                                itemLabel = proItem.property + "[] / " + itemRootObject.length
                                if (itemRootObject.length == 0) {
                                    var component = Qt.createComponent("PropItem.qml");
                                    component.createObject(propItemsContainer, {
                                        "label": itemLabel
                                    })
                                } else {
                                    var component = Qt.createComponent("PropGroup.qml");
                                    component.createObject(propItemsContainer, {
                                        "label": itemLabel,
                                        "rootObject":itemRootObject,
                                        "indentDepth": itemDepth,
                                        "isUnfold": true,
                                    })
                                }
                            } else {
                                var component = Qt.createComponent("PropGroup.qml");
                                component.createObject(propItemsContainer, {
                                    "label": itemLabel,
                                    "rootObject":itemRootObject,
                                    "indentDepth": itemDepth,
                                    "isUnfold": true,
                                })
                            }
                        } break;
                        case 'printLabel': {
                            var component = Qt.createComponent("PropItem.qml");
                            component.createObject(propItemsContainer, {
                                "label": proItem.property
                             })
                        } break;
                    }
                    root.hasContent = true
                } else {
                    console.log('Invalid property: ' + JSON.stringify(proItem));
                }
            } else if (proItem['type'] === 'printLabel') {
                var component = Qt.createComponent("PropItem.qml");
                component.createObject(propItemsContainer, {
                    "label": proItem.label
                })     
            } else {
                console.log('Invalid object: ' + JSON.stringify(proItem));
            }
        }
    }

    function populateFromObjectProps(object) {
        var propsModel = []

        if (object !== undefined) {
            var props = Object.keys(object);
            for (var p in props) {
                var o = {};
                o["object"] = object
                o["property"] = props[p];
                // o["readOnly"] = true;
            
                var thePropThing = object[props[p]];
                if ((thePropThing !== undefined) && (thePropThing !== null)) {
                    var theType = typeof(thePropThing)
                    switch(theType) {
                        case 'object': {
                            o["type"] = "object";
                            propsModel.push(o)  
                        } break;
                        default: {
                            o["type"] = "string";
                            propsModel.push(o)
                        } break;
                    }
                
                } else {
                    o["type"] = "string";
                    propsModel.push(o)
                }
            }
        }

        root.updatePropItems(root.propItemsPanel, propsModel);
    }

    Component.onCompleted: {
        if (root.rootObject !== null) {
            populateFromObjectProps(root.rootObject)
        }
    }
}
