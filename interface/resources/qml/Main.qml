import QtQuick 2.4
import QtQuick.Controls 1.3

ApplicationWindow {
	id: root
	width: 800
	height: 600
    visible: true

    menuBar: MenuBar {
        Menu { 
        	title: "File"
    		MenuItem {
        		text: "Quit"
        	} 
        }
    }

}

