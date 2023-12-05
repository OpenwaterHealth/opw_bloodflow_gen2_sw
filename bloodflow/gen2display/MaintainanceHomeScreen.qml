import QtQuick 2.12

Item {
    id: maintHomeScreen
    anchors.fill: parent

    Component.onCompleted: {
        rootWin.screenName = "Maintenance"
        sendUserCommandToTDA4(consts.cmd_get_time);
        sendUserCommandToTDA4(consts.cmd_get_temp);
    }

    Image{
        id: logo
        anchors{
            horizontalCenter: maintHomeScreen.horizontalCenter
            top: maintHomeScreen.top
            topMargin: maintHomeScreen.height/20
        }
        width: maintHomeScreen.width/4 ; height:maintHomeScreen.height/10
        source: "qrc:/images/OpenwaterLogo.png"
    }

    Image{
        id: maintScreenHome

        signal clicked

        anchors{
            left: maintHomeScreen.left
            top: maintHomeScreen.top
            topMargin: maintHomeScreen.height/40
            leftMargin: maintHomeScreen.width/40
        }
        width: maintHomeScreen.width/16 ; height:maintHomeScreen.height/9
        source: "qrc:/images/home.svg"
        opacity: maintScreenHomeButton.pressed ? 0.25 : 1

        MouseArea {
            id: maintScreenHomeButton
            anchors.fill: parent
            onClicked:{
                rootWin.currentScreen = CommonDefinitions.ScreensState.Home
            }
        }
    }

    Text {
        id: systemDateTime
        text: "Click to Update Time"
        font.family: "Helvetica"
        font.pointSize: maintHomeScreen.height/48
        color: "black"
        anchors.top: maintHomeScreen.top
        anchors.right: maintHomeScreen.right
        anchors.topMargin: maintHomeScreen.height/20
        anchors.rightMargin: maintHomeScreen.width/20
        MouseArea {
            id: systemDateTimeButton
            anchors.fill: parent
            onClicked: {
                // Update the text to display the new date and time

                sendUserCommandToTDA4(consts.cmd_get_time);
            }
        }
    }

    Text {
        id: systemTemp
        text: "Click to Update Internal Temperature"
        font.family: "Helvetica"
        font.pointSize: maintHomeScreen.height/48
        color: "black"
        anchors.top: systemDateTime.bottom
        anchors.right: systemDateTime.right
        anchors.topMargin: maintHomeScreen.height/20
        anchors.rightMargin: maintHomeScreen.width/60 - 20
        MouseArea {
            id: systemTempButton
            anchors.fill: parent
            onClicked: {
                sendUserCommandToTDA4(consts.cmd_get_temp);
            }
        }
    }

    Rectangle {
        id: bkupButton

        signal clicked

        anchors{
            left: maintHomeScreen.left
            leftMargin: 3*maintHomeScreen.width/16
            top: maintHomeScreen.top
            topMargin: maintHomeScreen.height/4
        }

        height: maintHomeScreen.height/8; width: maintHomeScreen.width/4
        antialiasing: true
        radius: 10
        color: backupMouseArea.pressed ? "darkCyan" : "darkGreen"
        enabled: rootWin.appIsAlive & !rootWin.responsePending

        MouseArea {
            id: backupMouseArea
            anchors.fill: parent
            onClicked:{
                sendUserCommandToTDA4(consts.cmd_backup)
            }
        }

        Text {
            text: "Backup\nto USB"
            color: "White"
            anchors.centerIn: parent
            font.family: "Helvetica"
            font.bold: true
            font.pointSize: maintHomeScreen.height/48
        }
    }

    Rectangle {
        id: bkupLogButton

        signal clicked

        anchors{
            left: bkupButton.left
            top: bkupButton.top
            topMargin: maintHomeScreen.height/5
        }

        height: maintHomeScreen.height/8; width: maintHomeScreen.width/4
        antialiasing: true
        radius: 10
        color: backupLogMouseArea.pressed ? "darkCyan" : "darkGreen"
        enabled: rootWin.appIsAlive & !rootWin.responsePending

        MouseArea {
            id: backupLogMouseArea
            anchors.fill: parent
            onClicked:{
                sendUserCommandToTDA4(consts.cmd_backup_logs)
            }
        }

        Text {
            text: "Backup\nlogs to USB"
            color: "White"
            anchors.centerIn: parent
            font.family: "Helvetica"
            font.bold: true
            font.pointSize: maintHomeScreen.height/48
        }
    }

    Rectangle {
        id: purgeButton

        signal clicked

        anchors{
            left: bkupLogButton.left
            top: bkupLogButton.top
            topMargin: maintHomeScreen.height/5
        }

        height: maintHomeScreen.height/8; width: maintHomeScreen.width/4
        antialiasing: true
        radius: 10
        color: purgeMouseArea.pressed ? "darkCyan" : "darkGreen"
        enabled: rootWin.appIsAlive & !rootWin.responsePending

        MouseArea {
            id: purgeMouseArea
            anchors.fill: parent
            onClicked:{
                rootWin.purgeRequested = true
                showInfoDialog("Are you sure you want to delete files " +
                        "that were previously backed-up to USB?", true, false)
            }
        }

        Text {
            text: "Delete previously\nbacked-up records"
            color: "White"
            anchors.centerIn: parent
            font.family: "Helvetica"
            font.bold: true
            font.pointSize: maintHomeScreen.height/48
        }
    }

    Rectangle {
        id: fwUpdateButton

        signal clicked

        anchors{
            left: maintHomeScreen.left
            leftMargin: 9*maintHomeScreen.width/16
            top: maintHomeScreen.top
            topMargin: maintHomeScreen.height/4
        }

        height: bkupButton.height; width: bkupButton.width
        antialiasing: true
        radius: 10
        color: fwUpdateMouseArea.pressed ? "darkCyan" : "darkGreen"
        enabled: rootWin.appIsAlive & !rootWin.responsePending

        MouseArea {
            id: fwUpdateMouseArea
            anchors.fill: parent
            onClicked:{
                sendUserCommandToTDA4(consts.cmd_update)
            }
        }

        Text {
            text: "Update Main\nFirmware"
            color: "white"
            anchors.centerIn: parent
            font.family: "Helvetica"
            font.bold: true
            font.pointSize: maintHomeScreen.height/48
        }
    }

    Rectangle {
        id: touchscreenUpdateButton

        signal clicked

        anchors{
            left: fwUpdateButton.left
            top: fwUpdateButton.top
            topMargin: maintHomeScreen.height/5
        }

        height: bkupButton.height; width: bkupButton.width
        antialiasing: true
        radius: 10
        color: touchscreenUpdateMouseArea.pressed ? "darkCyan" : "darkGreen"
        enabled: rootWin.appIsAlive & !rootWin.responsePending

        MouseArea {
            id: touchscreenUpdateMouseArea
            anchors.fill: parent
            onClicked:{
                if (!fwUpdate.available()) {
                    showErrorDialog("No update available", false)
                }
                else if (!fwUpdate.copy()) {
                    showErrorDialog("Unable to copy update", false)
                }
                else {
                    sendUserCommandToTDA4(consts.cmd_shutdown,
                            "Screen update complete")
                    console.log("Shutdown request from maintainance " +
                            "screen after UI update")
                }
            }
        }

        Text {
            text: "     Update\nTouchscreen"
            color: "white"
            anchors.centerIn: parent
            font.family: "Helvetica"
            font.bold: true
            font.pointSize: maintHomeScreen.height/48
        }
    }


    Text {
        text: "Flash Drive Connected"
        color: "black"
        anchors {
            left: parent.left
            top: dataManagerButton.bottom
//            topMargin: fileManagerScreen.height/6
            leftMargin: maintHomeScreen.width/20
        }        font.family: "Helvetica"
        font.bold: true
        font.pointSize: maintHomeScreen.height/48
        visible: rootWin.flashDriveConnected
    }


    function setSystemTime(timeString) {
        systemDateTime.text = timeString;
    }
    function setSystemTemp(tempString) {
        systemTemp.text = tempString;
    }
}
