// import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick 2.6
import QtQuick.Window 2.2
import QtQuick.Controls 1.5
import QtQuick.Dialogs 1.1
import Hifi 1.0 as Hifi
import QtQuick.Controls.Styles 1.4
import QtQuick.Layouts 1.3
import "content"

Rectangle {
    id: root
    width: parent ? parent.width : 100
    height: parent ? parent.height : 100
	color: "transparent"
    signal moved(vector2d position);
    signal resized(size size);
	
	signal sendToScript(var message);

    property var channel;
	
	TabView {
        id: frame
        anchors.fill: parent
        anchors.margins: 4
        Tab { title: "content/tabicons/colorpaletteBtn.png"
            ColorWheel{
				id: colorPicker
				anchors.verticalCenter: parent.verticalCenter
			}
			function sendToScript( message)  { root.sendToScript(message)}
        }
        Tab { title: "content/tabicons/linewidthBtn.png" 
			Slider {
			
				value: 0.25
				onValueChanged: {
					root.sendToScript(["width", value]);
				}
				
				style: SliderStyle {
					
					handle: Rectangle {
						anchors.centerIn: parent
						
						implicitWidth: 34
						implicitHeight: 34
						border.color: "black"
						border.width: 1
						color: "transparent"

						Rectangle {
							anchors.fill: parent; anchors.margins: 2
							border.color: "white"; border.width: 1
							color: "transparent"
						}
					}
					
				}
				
			}
		
		}
        Tab { title: "content/tabicons/brushesBtn.png" 
			GridLayout {
				id: grid
				columns: 3
				
				Layout.maximumWidth : 100
				Layout.maximumHeight : 100
				Layout.fillWidth : true
				Layout.fillHeight : true
				Button {
					//width : Layout.fillWidth
					//height : Layout.fillHeight
					Layout.fillWidth : true
					Layout.fillHeight : true
					text: ""
					//iconSource: "content/brushes/paintbrush1.png"
					onClicked: {
						root.sendToScript(["brush", img.source]);
					}
					
					Image {
						id: img
						source: "content/brushes/paintbrush1.png"
						anchors.fill: parent
						
					}
					
					style: ButtonStyle {
						background: Rectangle {
							//width : Layout.fillWidth
							//height : Layout.fillHeight
							border.width: control.activeFocus ? 2 : 1
							border.color: "#888"
							radius: 4
							gradient: Gradient {
								GradientStop { position: 0 ; color: control.pressed ? "#ccc" : "#eee" }
								GradientStop { position: 1 ; color: control.pressed ? "#aaa" : "#ccc" }
							}
						}
					}
				}
				Button {
					//width : Layout.fillWidth
					//height : Layout.fillHeight
					Layout.fillWidth : true
					Layout.fillHeight : true
					text: ""
					//iconSource: "content/brushes/paintbrush2.png"
					//onClicked: {
					//	root.sendToScript(["brush", iconSource]);
					//}
					
					//iconSource: "content/brushes/paintbrush1.png"
					onClicked: {
						root.sendToScript(["brush", img2.source]);
					}
					
					Image {
						id: img2
						source: "content/brushes/paintbrush2.png"
						anchors.fill: parent
						
					}
					
					style: ButtonStyle {
						background: Rectangle {
							//width : Layout.fillWidth
							//height : Layout.fillHeight
							border.width: control.activeFocus ? 2 : 1
							border.color: "#888"
							radius: 4
							gradient: Gradient {
								GradientStop { position: 0 ; color: control.pressed ? "#ccc" : "#eee" }
								GradientStop { position: 1 ; color: control.pressed ? "#aaa" : "#ccc" }
							}
						}
					}
				}
				Button {
					Layout.fillWidth : true
					Layout.fillHeight : true
					//width : Layout.fillWidth
					//height : Layout.fillHeight
					text: ""
					//iconSource: "content/brushes/paintbrush3.png"
					//onClicked: {
					//	root.sendToScript(["brush", iconSource]);
					//}
					
					//iconSource: "content/brushes/paintbrush1.png"
					onClicked: {
						root.sendToScript(["brush", img3.source]);
					}
					
					Image {
						id: img3
						source: "content/brushes/paintbrush3.png"
						anchors.fill: parent
						
					}
					
					style: ButtonStyle {
						background: Rectangle {
							//width : Layout.fillWidth
							//height : Layout.fillHeight
							border.width: control.activeFocus ? 2 : 1
							border.color: "#888"
							radius: 4
							gradient: Gradient {
								GradientStop { position: 0 ; color: control.pressed ? "#ccc" : "#eee" }
								GradientStop { position: 1 ; color: control.pressed ? "#aaa" : "#ccc" }
							}
						}
					}
				}
				Button {
					Layout.fillWidth : true
					Layout.fillHeight : true
					//width : Layout.fillWidth
					//height : Layout.fillHeight
					text: ""
					//iconSource: "content/brushes/paintbrush4.png"
					//onClicked: {
					//	root.sendToScript(["brush", iconSource]);
					//}
					
					//iconSource: "content/brushes/paintbrush1.png"
					onClicked: {
						root.sendToScript(["brush", img4.source]);
					}
					
					Image {
						id: img4
						source: "content/brushes/paintbrush4.png"
						anchors.fill: parent
						
					}
					
					style: ButtonStyle {
						background: Rectangle {
							//width : Layout.fillWidth
							//height : Layout.fillHeight
							border.width: control.activeFocus ? 2 : 1
							border.color: "#888"
							radius: 4
							gradient: Gradient {
								GradientStop { position: 0 ; color: control.pressed ? "#ccc" : "#eee" }
								GradientStop { position: 1 ; color: control.pressed ? "#aaa" : "#ccc" }
							}
						}
					}
				}
				Button {
					Layout.fillWidth : true
					Layout.fillHeight : true
					//width : Layout.fillWidth
					//height : Layout.fillHeight
					text: ""
					//iconSource: "content/brushes/paintbrush5.png"
					//onClicked: {
					//	root.sendToScript(["brush", iconSource]);
					//}
					
					//iconSource: "content/brushes/paintbrush1.png"
					onClicked: {
						root.sendToScript(["brush", img5.source]);
					}
					
					Image {
						id: img5
						source: "content/brushes/paintbrush5.png"
						anchors.fill: parent
						
					}
					
					style: ButtonStyle {
						background: Rectangle {
							//width : Layout.fillWidth
							//height : Layout.fillHeight
							border.width: control.activeFocus ? 2 : 1
							border.color: "#888"
							radius: 4
							gradient: Gradient {
								GradientStop { position: 0 ; color: control.pressed ? "#ccc" : "#eee" }
								GradientStop { position: 1 ; color: control.pressed ? "#aaa" : "#ccc" }
							}
						}
					}
				}
				Button {
					Layout.fillWidth : true
					Layout.fillHeight : true
					//width : Layout.fillWidth
					//height : Layout.fillHeight
					text: ""
					//iconSource: "content/brushes/paintbrush6.png"
					//onClicked: {
					//	root.sendToScript(["brush", iconSource]);
					//}
					
					//iconSource: "content/brushes/paintbrush1.png"
					onClicked: {
						root.sendToScript(["brush", img6.source]);
					}
					
					Image {
						id: img6
						source: "content/brushes/paintbrush6.png"
						anchors.fill: parent
						
					}
					
					style: ButtonStyle {
						background: Rectangle {
							//width : Layout.fillWidth
							//height : Layout.fillHeight
							border.width: control.activeFocus ? 2 : 1
							border.color: "#888"
							radius: 4
							gradient: Gradient {
								GradientStop { position: 0 ; color: control.pressed ? "#ccc" : "#eee" }
								GradientStop { position: 1 ; color: control.pressed ? "#aaa" : "#ccc" }
							}
						}
					}
				}
			}
		}
		
		Tab { title: "content/tabicons/eraser.png"
			
			
		
			Button {
				width : 200
				height : 50
				
				anchors.verticalCenter: parent.verticalCenter
				text: "Undo"	
				onClicked: {
					root.sendToScript(["undo"]);
				}
				
				style: ButtonStyle {
					background: Rectangle {
						//width : Layout.fillWidth
						//height : Layout.fillHeight
						border.width: control.activeFocus ? 2 : 1
						border.color: "#888"
						radius: 4
						gradient: Gradient {
							GradientStop { position: 0 ; color: control.pressed ? "#ccc" : "#eee" }
							GradientStop { position: 1 ; color: control.pressed ? "#aaa" : "#ccc" }
						}
					}
				}
			}
		}
		
		Tab { title: "content/tabicons/pointingfinger128px.png"
			
			
		
			Button {
				width : 200
				height : 50
				anchors.verticalCenter: parent.verticalCenter
				text: "Toggle Hand"	
				onClicked: {
					root.sendToScript(["hand"]);
				}
				
				style: ButtonStyle {
					background: Rectangle {
						
						border.width: control.activeFocus ? 2 : 1
						border.color: "#888"
						radius: 4
						gradient: Gradient {
							GradientStop { position: 0 ; color: control.pressed ? "#ccc" : "#eee" }
							GradientStop { position: 1 ; color: control.pressed ? "#aaa" : "#ccc" }
						}
					}
				}
			}
		}

        style: TabViewStyle {
            frameOverlap: 1
            tab: Rectangle {
                color: styleData.selected ? "steelblue" : "grey"
                border.color:  "grey"
                implicitWidth: 100
                implicitHeight: 100
                radius: 2
                Text {
                    id: text
                    anchors.centerIn: parent
                    text: ""
                    color: styleData.selected ? "white" : "black"
                }
                Image {
                    source: styleData.title
                    width: parent.width
                    height: parent.height
                }
            }
            frame: Rectangle { color: "grey" }
        }
    }
	
	
}





