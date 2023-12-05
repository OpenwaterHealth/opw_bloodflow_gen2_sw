import QtQuick 2.12

// Error dialog
// This is a general purpose error dialog. Errors can be 'hard' or 'soft',
//  with the former providing a dialog with a 'shutdown' button, and the
//  latter providing a 'continue' button. The main difference between
//  an info dialog and a soft error is the icon displayed.

// A packet's 'msg' field is displayed in the main text area of the popup.
//  Content from the 'params' field is displayed in a small font below
//  the main text area.


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
        id: errorMessageCanvas
        color: "lightgray"
        anchors{
            horizontalCenter: parent.horizontalCenter
            top: parent.top
            topMargin: 5*parent.height/16
        }
        height: parent.height/2
        width: parent.width/2
    }

    Image{
        id: errorImage
        anchors{
            bottom: errorMessageCanvas.top
            left: errorMessageCanvas.left
            bottomMargin: errorImage.height/8
        }
        height: errorMessageCanvas.height/4
        width: errorMessageCanvas.height/4
        source: "qrc:/images/error.png"
    }

    Text {
        id: errorTextDisplay
        anchors.centerIn: errorMessageCanvas
        text: rootWin.errorPopupString
        anchors.fill: errorMessageCanvas
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        antialiasing: true
        font.family: "Helvetica"
        font.pointSize: parent.height/32
        color: "Black"
    }

    Text {
        id: errorParamText
        anchors{
            horizontalCenter: errorMessageCanvas.horizontalCenter
            top: errorMessageCanvas.bottom
        }
        text: rootWin.errorPopupStringParam
        antialiasing: true
        font.family: "Helvetica"
        font.pointSize: parent.height / 64
        color: "Gray"
    }


    Rectangle {
        id: dismissErrorButton

        anchors{
            horizontalCenter: errorMessageCanvas.horizontalCenter
            top: errorMessageCanvas.bottom
            topMargin: errorMessageCanvas.height/9
        }

        height: errorMessageCanvas.height/5 
        width: errorMessageCanvas.width/2
        antialiasing: true
        radius: 10
        // if shutdown is the only option then paint button reddish,
        //  otherwise use greenish
        color: rootWin.popupShutdownOnly ?
            (dismissErrorMouseArea.pressed ? "darkOrange" : "darkRed") :
            (dismissErrorMouseArea.pressed ? "darkCyan" : "darkGreen")
        enabled: rootWin.appIsAlive & !rootWin.responsePending
        opacity: enabled ? 1.0 : 0.25

        MouseArea {
            id: dismissErrorMouseArea
            anchors.fill: parent
            onClicked:{
                if (rootWin.popupShutdownOnly) {
                    rootWin.errorPopupDisplay = false
                    sendUserCommandToTDA4(consts.cmd_shutdown)
                } else {
                    rootWin.errorPopupDisplay = false
                }
            }
        }

        Text {
            text: rootWin.popupShutdownOnly ? "Shutdown" : "Dismiss"
            color: "white"
            anchors.centerIn: parent
            font.family: "Helvetica"
            font.bold: true
            font.pointSize: undelyingScreenBlocker.height/32
        }
    }

}
