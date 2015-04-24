import Hifi 1.0 as Hifi
import QtQuick 2.4
import QtQuick.Controls 1.3
import QtQuick.Controls.Styles 1.3
import "controls"
import "styles"

Hifi.HifiMenu {
	id: root
	anchors.fill: parent
	objectName: "HifiMenu"
    enabled: false
    opacity: 0.0
    property int animationDuration: 200
    HifiPalette { id: hifiPalette }

    onEnabledChanged: {
    	if (enabled && columns.length == 0) {
    		pushColumn(rootMenu.items);
    	}
    	opacity = enabled ? 1.0 : 0.0
        if (enabled) {
            forceActiveFocus()
        } 
    }

    // The actual animator
    Behavior on opacity {
        NumberAnimation {
            duration: root.animationDuration
            easing.type: Easing.InOutBounce
        }
    }

    onOpacityChanged: {
        visible = (opacity != 0.0);
    }

    onVisibleChanged: {
    	if (!visible) reset();
    }


    property var menu: Menu {}
    property var models: []
	property var columns: []
    property var itemBuilder: Component {
    	Text {
    	    SystemPalette { id: sp; colorGroup: SystemPalette.Active }
    		id: thisText
    		x: 32
    		property var source
	    	property var root
	    	property var listViewIndex
	    	property var listView
    		text: typedText()
    		height: implicitHeight
    		width: implicitWidth
    		color: source.enabled ? "black" : "gray"
    			
    		onImplicitWidthChanged: {
    			if (listView) {
    				listView.minWidth = Math.max(listView.minWidth, implicitWidth + 64);
    				listView.recalculateSize();
    			}
    		}

    		FontAwesome {
    			visible: source.type == 1 && source.checkable
    			x: -32
        		text: (source.type == 1 && source.checked) ? "\uF05D" : "\uF10C"
    		}
    		
    		FontAwesome {
    			visible: source.type == 2
    			x: listView.width - 64
        		text: "\uF0DA"
    		}
    		 

    		function typedText() {
    			switch(source.type) {
    			case 2:
    				return source.title;
    			case 1: 
    				return source.text;
    			case 0:
    				return "-----"
    			}
    		}
    		
    		MouseArea {
    		    id: mouseArea
    		    acceptedButtons: Qt.LeftButton
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 0
                anchors.top: parent.top
                anchors.topMargin: 0
                width: listView.width
    		    onClicked: {
    				listView.currentIndex = listViewIndex
		    		parent.root.selectItem(parent.source);
    	    	}
    	    }
    	}
    }

    	
    property var menuBuilder: Component {
    	Border {
    		SystemPalette { id: sysPalette; colorGroup: SystemPalette.Active }
    		x: root.models.length * 60; 
            anchors.verticalCenter: parent.verticalCenter
			border.color: hifiPalette.hifiBlue
			color: sysPalette.window

			ListView {
				spacing: 6
				property int outerMargin: 8 
				property real minWidth: 0
    			anchors.fill: parent
    			anchors.margins: outerMargin
    			id: listView
    			height: root.height
    			currentIndex: -1

    			onCountChanged: {
    				recalculateSize()
    			}    			 
    			
    			function recalculateSize() {
    				var newHeight = 0
    				var newWidth = minWidth;
    				for (var i = 0; i < children.length; ++i) {
    					var item = children[i];
    					newHeight += item.height
    				}
    				parent.height = newHeight + outerMargin * 2;
    				parent.width = newWidth + outerMargin * 2
    			}
    		
    			highlight: Rectangle {
    	            width: listView.minWidth; height: 32
    	            color: sysPalette.highlight
    	            y: (listView.currentItem) ? listView.currentItem.y : 0;
    	            Behavior on y { 
    		  	        NumberAnimation {
    		  	            duration: 100
    		  	            easing.type: Easing.InOutQuint
    		  	        }
                	}
    	        }
   		

    			property int columnIndex: root.models.length - 1 
    			model: root.models[columnIndex]
    		    delegate: Loader {
    		    	id: loader
    		    	sourceComponent: root.itemBuilder
    		        Binding {
    		            target: loader.item
    		            property: "root"
    		            value: root
    		            when: loader.status == Loader.Ready
    		        }	    
    		        Binding {
    		            target: loader.item
    		            property: "source"
    		            value: modelData
    		            when: loader.status == Loader.Ready
    		        }	    
    		        Binding {
    		            target: loader.item
    		            property: "listViewIndex"
    		            value: index
    		            when: loader.status == Loader.Ready
    		        }	    
    		        Binding {
    		            target: loader.item
    		            property: "listView"
    		            value: listView
    		            when: loader.status == Loader.Ready
    		        }	    
    		    }

    		}
    			
    	}
    }
 

    function lastColumn() {
    	return columns[root.columns.length - 1];
    }
    
	function pushColumn(items) {
		models.push(items)
		if (columns.length) {
			var oldColumn = lastColumn();
			oldColumn.enabled = false;
			oldColumn.opacity = 0.5;
		}
		var newColumn = menuBuilder.createObject(root); 
		columns.push(newColumn);
		newColumn.forceActiveFocus();
	}

	function popColumn() {
		if (columns.length > 0) {
			var curColumn = columns.pop();
			console.log(curColumn);
			curColumn.visible = false;
			curColumn.destroy();
			models.pop();
		} 
		
		if (columns.length == 0) {
			enabled = false;
			return;
		} 

		curColumn = lastColumn();
		curColumn.enabled = true;
		curColumn.opacity = 1.0;
		curColumn.forceActiveFocus();
    }
    
	function selectItem(source) {
		switch (source.type) {
		case 2:
			pushColumn(source.items)
			break;
		case 1:
			source.trigger()
			enabled = false
			break;
		case 0: 
			break;
		}
	}
	
	function reset() {
		while (columns.length > 0) {
			popColumn();
		}
	}

	/*
    HifiMenu {
	    id: rootMenu
	    Menu {
			id: menu
	        Menu {
	            title: "File"
	        	MenuItem { 
	            	action: HifiAction {
	            		name: rootMenu.login
	            	}
	    		}
				MenuItem { 
					action: HifiAction {
						name: "Test"
						checkable: true
					}
				}
	    		MenuItem { 
	    			action: HifiAction {
	    				name: rootMenu.quit
	    			}
				}
	    	}
	        Menu {
	            title: "Edit"
	            MenuItem { 
	            	action: HifiAction {
	                    text: "Copy"
	                    shortcut: StandardKey.Copy
	                }
	    		}
		        MenuItem { 
		        	action: HifiAction {
		                text: "Cut"
		                shortcut: StandardKey.Cut
		            }
				}
		        MenuItem { 
		        	action: HifiAction {
		                text: "Paste"
		                shortcut: StandardKey.Paste
		            }
				}
		        MenuItem { 
		        	action: HifiAction {
		                text: "Undo"
		                shortcut: StandardKey.Undo
		            }
				}
		        MenuItem { 
		        	action: HifiAction {
		                text: "Redo"
		                shortcut: StandardKey.Redo
		            }
				}
		        MenuItem { 
		            action: HifiAction {
		        		name: rootMenu.attachments
		        	}
				}
		        MenuItem { 
		            action: HifiAction {
		        		name: rootMenu.animations
		        	}
				}
	        }

	        Menu {
	        	title: "Scripts"
		        MenuItem {
	                action: HifiAction {
	            		name: rootMenu.scriptEditor
	            	}
	        	}
		        MenuItem {
	                action: HifiAction {
	            		name: rootMenu.loadScript
	            	}
	        	}
		        MenuItem {
		            action: HifiAction {
		        		name: rootMenu.loadScriptURL
		        	}
		    	}
		        MenuItem {
		    		action: HifiAction {
		        		name: rootMenu.stopAllScripts
		        	}        
		    	}
		        MenuItem {
		    		action: HifiAction {
		        		name: rootMenu.reloadAllScripts
		        	}        
		    	}
		        MenuItem {
		    		action: HifiAction {
		        		name: rootMenu.runningScripts
		        	}        
		    	}
	        }
	        Menu {
	        	title: "Location"
		        MenuItem {
		    		action: HifiAction {
		        		name: rootMenu.addressBar
		        	}        
		    	}
		        MenuItem {
		    		action: HifiAction {
		        		name: rootMenu.copyAddress
		        	}        
		    	}
		        MenuItem {
		    		action: HifiAction {
		        		name: rootMenu.copyPath
		        	}        
		    	}
	        }
	    }
	}
    */
	
    Keys.onPressed: {
    	switch (event.key) {
    	case Qt.Key_Escape:
        	root.popColumn()
            event.accepted = true;
    	}
    }

	MouseArea {
		anchors.fill: parent
	    id: mouseArea
	    acceptedButtons: Qt.LeftButton | Qt.RightButton
	    onClicked: {
            if (mouse.button == Qt.RightButton) {
            	root.popColumn();
            } else {
            	root.enabled = false;
            }    		
	    }
	}
}
