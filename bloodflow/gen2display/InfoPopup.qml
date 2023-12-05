import QtQuick 2.12

// Info dialog
// This is a general purpose dialog to display non-critical information 
//  to the user. It can be configured to have a 'continue' (OK) button, 
//  dismissing the dialog, or a non-dimissable dialog with a 'request 
//  shutdown' button being the only way out. The non-dismissable dialog
//  is for when a process is underway (e.g., firmware update) and it's
//  not otherwise interruptible. Non-dimissable dialogs can only be
//  cleared by a command coming from the device.

// A packet's 'msg' field is displayed in the main text area of the popup.
//  Content from the 'params' field is displayed in a small font below
//  the main text area.

// When dialog is displayed, the content of the rest of the screen is dimmed.

Item {
    Rectangle{
        // Dim the background and prevent mouse clicks from going to the
        //  background.
        id: undelyingScreenBlocker
        anchors.fill: parent
        color: "white"
        opacity: 0.80
        MouseArea {
            anchors.fill: parent
            onClicked:{
                // do nothing
            }
        }
    }

    Rectangle{
        id: infoMessageCanvas
        color: "lightgray"
        anchors{
            horizontalCenter: parent.horizontalCenter
            top: parent.top
            topMargin: 5*parent.height/16
        }
        height: parent.height/2
        width: parent.width/2
    }

    Rectangle{
        id: infoMessageCanvas2
        color: "white"
        anchors{
            horizontalCenter: parent.horizontalCenter
            top: parent.top
            topMargin: 5*parent.height/16 + parent.height/2
        }
        height: parent.height/4
        width: parent.width/2
    }

    Image{
        id: infoImage
        anchors{
            bottom: infoMessageCanvas.top
            left: infoMessageCanvas.left
            bottomMargin: infoImage.height/8
        }
        height: infoMessageCanvas.height/4
        width: infoMessageCanvas.height/4
        source: "qrc:/images/notification.png"
    }

    Text {
        id: infoTextDisplay
        anchors.centerIn: infoMessageCanvas
        text: rootWin.infoPopupString
        anchors.fill: infoMessageCanvas
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        antialiasing: true
        font.family: "Helvetica"
        font.pointSize: parent.height/32
        color: "Black"
    }

    Rectangle {
        id: okInfoButton

        anchors{
            right: infoMessageCanvas.right
            top: infoMessageCanvas.bottom
            topMargin: infoMessageCanvas.height/9
        }

        height: infoMessageCanvas.height/5 
        width: 2*infoMessageCanvas.width/5
        antialiasing: true
        radius: 10
        // if shutdown is the only option then paint button reddish,
        //  otherwise use greenish
        color: rootWin.popupShutdownOnly ?
            (okInfoMouseArea.pressed ? "darkOrange" : "darkRed") :
            (okInfoMouseArea.pressed ? "darkCyan" : "darkGreen")
        enabled: rootWin.appIsAlive & !rootWin.responsePending
        opacity: enabled ? 1.0 : 0.25

        MouseArea {
            id: okInfoMouseArea
            anchors.fill: parent
            onClicked:{
                if ((rootWin.popupShutdownOnly) || 
                        (rootWin.requestShutdown)) {
                    sendUserCommandToTDA4(consts.cmd_shutdown)
                } else if (rootWin.purgeRequested == true) {
                    sendUserCommandToTDA4(consts.cmd_cleanup)
                    rootWin.purgeRequested = false
                }
                rootWin.infoPopupDisplay = false
            }
        }

        Text {
            text: rootWin.popupShutdownOnly ? "Shutdown" : "OK"
            color: "white"
            anchors.centerIn: parent
            font.family: "Helvetica"
            font.bold: true
            font.pointSize: undelyingScreenBlocker.height/32
        }
    }

    Text {
        id: infoParamText
        anchors{
            horizontalCenter: infoMessageCanvas.horizontalCenter
            top: infoMessageCanvas.bottom 
            //top: infoMessageCanvas.bottom - infoMessageCanvas.height/6
        }
        text: rootWin.infoPopupStringParam
        antialiasing: true
        font.family: "Helvetica"
        font.pointSize: parent.height / 64
        color: "Gray"
    }


    Rectangle {
        id: cancelInfoButton

        anchors{
            left: infoMessageCanvas.left
            top: infoMessageCanvas.bottom
            topMargin: infoMessageCanvas.height/9
        }

        height: infoMessageCanvas.height/5 
        width: 2*infoMessageCanvas.width/5
        antialiasing: true
        radius: 10
        visible: true
        enabled: rootWin.infoShowCancel
        opacity: enabled ? 1.0 : 0.25
        // if shutdown is the only option then paint button reddish,
        //  otherwise use greenish
        color: rootWin.popupShutdownOnly ?
            (cancelInfoMouseArea.pressed ? "darkOrange" : "darkRed") :
            (cancelInfoMouseArea.pressed ? "darkCyan" : "darkGreen")

        MouseArea {
            id: cancelInfoMouseArea
            anchors.fill: parent
            onClicked:{
                rootWin.infoPopupDisplay = false
                // clear any requested-action state
                rootWin.purgeRequested = false
                rootWin.requestShutdown = false
            }
        }

        Text {
            text: "Cancel"
            color: "white"
            anchors.centerIn: parent
            font.family: "Helvetica"
            font.bold: true
            font.pointSize: undelyingScreenBlocker.height/32
        }
    }
}
