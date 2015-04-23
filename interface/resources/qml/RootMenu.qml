import QtQuick 2.4
import QtQuick.Controls 1.3

Item {
	Menu {
		id: rootMenuFooBar
		objectName: "rootMenu"
	    Menu {
	        title: "File"
			MenuItem { 
	        	text: "Test"
				checkable: true
			}
	    	MenuItem {
	        	text: "Quit"
			}
		}
	    Menu {
	        title: "Edit"
	        MenuItem { 
	            text: "Copy"
			}
	        MenuItem { 
	            text: "Cut"
			}
	        MenuItem { 
	            text: "Paste"
			}
	        MenuItem { 
	            text: "Undo"
			}
	    }
	}
}
