        Rectangle {
            id:permissionsContainer
            visible:false
            color: "#000000"
            width:  parent.width
            anchors.top: buttons.bottom
            height:40
            z:100
            gradient: Gradient {
                GradientStop { position: 0.0; color: "black" }
                GradientStop { position: 1.0; color: "grey" }
            }

            RalewayLight {
                    id: permissionsInfo
                    anchors.right:permissionsRow.left
                    anchors.rightMargin: 32
                    anchors.topMargin:8
                    anchors.top:parent.top
                    text: "This site wants to use your microphone/camera"
                    size: 18
                    color: hifi.colors.white
            }
         
            Row {
                id: permissionsRow
                spacing: 4
                anchors.top:parent.top
                anchors.topMargin: 8
                anchors.right: parent.right
                visible: true
                z:101
                
                Button {
                    id:allow
                    text: "Allow"
                    color: hifi.buttons.blue
                    colorScheme: root.colorScheme
                    width: 120
                    enabled: true
                    onClicked: root.allowPermissions(); 
                    z:101
                }

                Button {
                    id:block
                    text: "Block"
                    color: hifi.buttons.red
                    colorScheme: root.colorScheme
                    width: 120
                    enabled: true
                    onClicked: root.hidePermissionsBar();
                    z:101
                }
            }
        }

    function hidePermissionsBar(){
      permissionsContainer.visible=false;
    }

    function allowPermissions(){
        webview.grantFeaturePermission(permissionsBar.securityOrigin, permissionsBar.feature, true);
        hidePermissionsBar();
    }

    