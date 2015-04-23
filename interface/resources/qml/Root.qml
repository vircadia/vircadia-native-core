import Hifi 1.0
import QtQuick 2.3

// This is our primary 'window' object to which all dialogs and controls will 
// be childed. 
Root {
    id: root
    anchors.fill: parent
    Item {
        Menu {
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
}

