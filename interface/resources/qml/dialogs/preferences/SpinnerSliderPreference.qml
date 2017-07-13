//
//  SpinnerSliderPreference.qml
//
//  Created by Cain Kilgore on 11th July 2017
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5

import "../../dialogs"
import "../../controls-uit"

Preference {
    id: root
    property alias slider: slider
    height: control.height + hifi.dimensions.controlInterlineHeight

    Component.onCompleted: {
        slider.value = preference.value;
    }

    function save() {
        preference.value = slider.value;
        preference.save();
    }

	function sliderToAvatarScale(sliderValue) {
		return MyAvatar.getDomainMinScale() + (MyAvatar.getDomainMaxScale()-MyAvatar.getDomainMinScale())*value;
	}

    Item {
        id: control
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        height: Math.max(labelText.height, slider.height)

        Label {
            id: labelText
            text: root.label + ":"
            colorScheme: hifi.colorSchemes.dark
            anchors {
                left: parent.left
                right: slider.left
                rightMargin: hifi.dimensions.labelPadding
                verticalCenter: parent.verticalCenter
            }
            horizontalAlignment: Text.AlignRight
            wrapMode: Text.Wrap
        }

        Slider {
            id: slider
            value: preference.value
            width: 100
            minimumValue: MyAvatar.getDomainMinScale()
            maximumValue: MyAvatar.getDomainMaxScale()
            stepSize: preference.step
            onValueChanged: {
                spinner.value = value
            }
            anchors {
                right: spinner.left
				rightMargin: 10
            }
        }
        SpinBox {
            id: spinner
            decimals: preference.decimals
            value: preference.value
            minimumValue: MyAvatar.getDomainMinScale()
            maximumValue: MyAvatar.getDomainMaxScale()
            width: 100
            onValueChanged: {
                slider.value = value;
            }
            anchors {
                right: button.left
				rightMargin: 10
            }
            colorScheme: hifi.colorSchemes.dark
        }
		Button {
			id: button
			onClicked: {
				if(spinner.maximumValue >= 1) {
					spinner.value = 1
					slider.value = 1
				} else {
					spinner.value = spinner.maximumValue
					slider.value = spinner.maximumValue
				}
			}
			width: 50
            font.family: hiFiGlyphs.name
			text: "a"
			anchors {
				right: parent.right
			}
		}
    }
}