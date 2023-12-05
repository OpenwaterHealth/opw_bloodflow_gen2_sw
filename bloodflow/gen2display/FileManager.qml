import QtQuick 2.12
import QtQuick.Controls 2.5
//import QtQuick.TreeView
//import "CustomTreeView.qml"
// "File Management" screen
// Allows the user to see scans and delete them

//import ".." 1.0
//import "../components" 1.0

Item {

    id: fileManagerScreen

    Component.onCompleted: {
        rootWin.screenName = "File Manager"
        fileManager.update("")
    }

    Image{
        id: maintScreenHome

        signal clicked

        anchors{
            left: fileManagerScreen.left
            top: fileManagerScreen.top
            topMargin: fileManagerScreen.height/40
            leftMargin: fileManagerScreen.width/40
        }
        width: fileManagerScreen.width/16 ; height:fileManagerScreen.height/9
        source: "qrc:/images/home.svg"
        opacity: fileManagerScreenHomeButton.pressed ? 0.25 : 1

        MouseArea {
            id: fileManagerScreenHomeButton
            anchors.fill: parent
            onClicked:{
                rootWin.currentScreen = CommonDefinitions.ScreensState.Home
            }
        }
    }

    Image{
        id: powerOffButton

        signal clicked

        anchors{
            right: fileManagerScreen.right
            top: fileManagerScreen.top
            topMargin: fileManagerScreen.height/40
            rightMargin: fileManagerScreen.width/40
        }
        width: fileManagerScreen.width/16 ; height:fileManagerScreen.height/9
        source: "qrc:/images/powerButton.png"
        opacity: powerOffButton.pressed ? 0.25 : 1

        MouseArea {
            id: powerOffButtonArea
            anchors.fill: parent
            onClicked:{
                showInfoDialog("Requesting shutdown.", true, false)
                console.log("Shutdown request from home screen")
                rootWin.requestShutdown = true
            }
        }
    }
    ListView {
        id: sampleListView
        anchors {
            top : powerOffButton.bottom
            topMargin:  20
            left: fileManagerScreen.left
            leftMargin: fileManagerScreen.width/10
        }
        width: screenWidth/2
        height: screenHeight *2/3
        model: fileSystemModel


        // Create the list view delegate
        delegate: Rectangle {
            width: screenWidth/2
            height: 40
            color: 'lightgray'
            CheckBox {
                id: checkBox
                x: parent.x
                anchors.verticalCenter: parent.verticalCenter

                onCheckStateChanged: {
                    if(checkBox.checkState == 2){//checked
                        console.log("checked!")
                        fileManager.addIndex(model.index);
                    }
                    else{
                        fileManager.removeIndex(model.index);
                    }
                }
            }

            Text {
                text: display
                anchors.verticalCenter: parent.verticalCenter
                x: checkBox.x + 50
                width: parent.width / 3 // Adjust the width as needed
//                horizontalAlignment: Text.AlignHCenter
                font.pointSize: 16
            }

/*
            Rectangle {
                id: treeNode
                implicitWidth: treeNodeLabel.x + treeNodeLabel.width
                implicitHeight: Math.max(indicator.height, treeNodeLabel.height, checkBox.height)
                height: 40
                color: d.bgColor(column, row)

                Image {
                    id: indicator
                    x: 50
                    width: implicitWidth
                    anchors.verticalCenter: parent.verticalCenter
                    source: {

                            "qrc:/images/file.png"
                    }
                }
                CheckBox {
                    id: checkBox
                    anchors {
                        top: parent.top
                    }
                    onCheckStateChanged: {
                        if(checkBox.checkState == 2){//checked
                            console.log("checked!")
                            fileManager.addIndex(model.index);
                        }
                        else{
                            fileManager.removeIndex(model.index);
                        }
                    }
                 }

                TapHandler {
                    onTapped: {
                        checkBox.toggle()
                    }
                }
                Text {
                    id: treeNodeLabel
                    x: indicator.x + Math.max(styleHints.indent, indicator.width * 1.5 + checkBox.width)
                    anchors.verticalCenter: parent.verticalCenter
                    clip: true
                    color: d.fgColor(column, row)
                    //font: styleHints.font
                    text: model.display
                    font.pointSize: 16
                }
            }
        */
        }
    }

    // Back, Delete and Transfer Buttons
    Rectangle {
        id: deleteButton

        signal clicked
        anchors {
            right: fileManagerScreen.right
            top: powerOffButton.bottom
            topMargin:  20
            rightMargin: fileManagerScreen.width/20

        }

        height: fileManagerScreen.height/8; width: fileManagerScreen.width/4
        radius: 10
        color: deleteButtonTouchArea.pressed ? "darkCyan" : "darkGreen"
        MouseArea {
            id: deleteButtonTouchArea
            anchors.fill: parent
            onClicked:{
                console.log("deleting")
                fileManager.deleteList();
            }
        }
        Text {
            text: "Delete"
            color: "White"
            anchors.centerIn: parent
            font.family: "Helvetica"
            font.bold: true
            font.pointSize: 15
        }
    }

    Rectangle {
        id: transferButton

        signal clicked
        anchors {
            right: fileManagerScreen.right
            rightMargin: fileManagerScreen.width/20
            top: deleteButton.bottom
            topMargin: fileManagerScreen.height/20
        }

        height: fileManagerScreen.height/8; width: fileManagerScreen.width/4
        radius: 10
        color: transferButtonTouchArea.pressed ? "darkCyan" : "darkGreen"
        MouseArea {
            id: transferButtonTouchArea
            anchors.fill: parent

            onClicked:{
                fileManager.transferList();
            }
        }
        Text {
            text: "Transfer"
            color: "White"
            anchors.centerIn: parent
            font.family: "Helvetica"
            font.bold: true
            font.pointSize: 15
        }
    }


    // Flash Drive Status View (shows connection status and external storage free) + Internal Storage ViewSection
    Text {
        id: systemFreeStorage
        text: "Internal Storage Free: " + rootWin.internalStorageFree
        font.family: "Helvetica"
        font.pointSize: fileManagerScreen.height/48
        color: "black"
        anchors.bottom: fileManagerScreen.bottom
        anchors.right: fileManagerScreen.right
        anchors.bottomMargin: fileManagerScreen.height/20
        anchors.rightMargin: fileManagerScreen.width/6
        MouseArea {
            id: systemFreeStorageButton
            anchors.fill: parent
            onClicked: {
                sendUserCommandToTDA4(consts.cmd_get_free_internal_storage);
            }
        }
    }

    Text {
        id: flashDriveStatus
        text: "Flash Drive Connected (" + rootWin.flashDriveFreeStorage + " free)"
        font.family: "Helvetica"
        color: "black"
        anchors {
            left: fileManagerScreen.left
            leftMargin: fileManagerScreen.width/10
            bottom: fileManagerScreen.bottom
            bottomMargin: fileManagerScreen.height/20
        }
        font.bold: false
        font.pointSize: fileManagerScreen.height/48
        visible: rootWin.flashDriveConnected
    }

}
