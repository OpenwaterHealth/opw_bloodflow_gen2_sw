import QtQuick 2.12

Item {
    id: homeScreen
    anchors.fill: parent

    Component.onCompleted: {
        rootWin.screenName = "Home"
        Qt.inputMethod.hide();

    }

    Image{
        id: logo
        anchors{
            horizontalCenter: homeScreen.horizontalCenter
            top: homeScreen.top
            topMargin: homeScreen.height/9
        }
        width: homeScreen.width/2 ; height:homeScreen.height/5
        source: "qrc:/images/OpenwaterLogo.png"
    }

    Text {
        id: shutdownTxt
        anchors{
            right: homeScreen.right
            top: homeScreen.top
            rightMargin: powerOffButton.width * 3 / 2
        }
        text: "To prevent damage tap this shutdown button\n" + 
                "before using the main power switch"
        antialiasing: true
        font.family: "Helvetica"
        font.pointSize: homeScreen.height / 58
        color: "Gray"
    }

    Image{
        id: powerOffButton

        signal clicked

        anchors{
            right: homeScreen.right
            top: homeScreen.top
            topMargin: homeScreen.height/40
            rightMargin: homeScreen.width/40
        }
        width: homeScreen.width/16 ; height:homeScreen.height/9
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

    Text {
        id: patientIDTxt
        anchors{
            left: homeScreen.left
            leftMargin: homeScreen.width/4
            top: logo.bottom
            topMargin: logo.height/3
        }
        text: "Patient ID"
        antialiasing: true
        font.family: "Helvetica"
        font.pointSize: homeScreen.height/32
        color: "Gray"
    }

    Text {
        id: estopTxt
        anchors{
            left: homeScreen.left
            leftMargin: homeScreen.width/4
            top: logo.bottom
            topMargin: 2*logo.height/3
        }
        text: "Emergency stop triggered"
        antialiasing: true
        font.family: "Helvetica"
        font.bold: true
        font.pointSize: homeScreen.height/24
        color: "red"
        visible: rootWin.estop
    }

    Rectangle {
        id: patientIDBox

        property alias patientIDLocal: patientIDEntered.text

        anchors{
            left: patientIDTxt.right
            leftMargin: homeScreen.width/40
            top: patientIDTxt.top
        }
        height: patientIDTxt.height; width: homeScreen.width/5
        color: "lightGray"
        radius: 4
        border{
            color: "darkGreen"
            width: patientIDBox.height/20
        }

        TextInput{
            id: patientIDEntered

            //inputMethodHints: Qt.ImhDigitsOnly
            focus: Qt.inputMethod.visible;
            onActiveFocusChanged: {
                if (!activeFocus) {
                    Qt.inputMethod.hide
                }
            }

            anchors{
                leftMargin: patientIDBox.width/20
                fill: parent
            }
            color: focus ? "black" : "gray"
            font.pixelSize: parent.height - 10
        }
    }

    Rectangle {
        id: startScanButton

        signal clicked

        anchors{
            left: patientIDBox.right
            leftMargin: homeScreen.width/40
            top: patientIDBox.top
        }

        height: patientIDTxt.height; width: patientIDTxt.width
        antialiasing: true
        radius: 10
        color: rootWin.estop ?
                scanMouseArea.pressed ? "gray" : "gray" :
                scanMouseArea.pressed ? "darkCyan" : "darkGreen"
        enabled: !rootWin.estop

        MouseArea {
            id: scanMouseArea
            anchors.fill: parent
            onClicked:{
                rootWin.patientID = patientIDBox.patientIDLocal != "" ? 
                        patientIDBox.patientIDLocal : rootWin.patientID
                sendUserCommandToTDA4(consts.cmd_open_patient, 
                        rootWin.patientID)
                Qt.inputMethod.hide()
            }
        }

        Text {
            text: "Start"
            color: "white"
            anchors.centerIn: parent
            font.family: "Helvetica"
            font.bold: true
            font.pointSize: homeScreen.height/32
        }
    }

    Rectangle {
        id: maintainanceButton

        visible: !Qt.inputMethod.visible
        signal clicked

        anchors{
            right: homeScreen.right
            bottom: homeScreen.bottom
            rightMargin: homeScreen.width/40
            bottomMargin: homeScreen.height/24
        }

        height: patientIDTxt.height*2; width: patientIDTxt.width*2
        antialiasing: true
        radius: 10
        color: maintainanceMouseArea.pressed ? "" : "white"

        MouseArea {
            id: maintainanceMouseArea
            anchors.fill: parent
            onClicked:{
                rootWin.currentScreen = CommonDefinitions.ScreensState.PasswordScreen
            }
        }

        Text {
            text: "Maintenance\n       Mode"
            color: maintainanceMouseArea.pressed ? "white" : "black"
            anchors.centerIn: parent
            font.family: "Helvetica"
            font.bold: true
            font.pointSize: homeScreen.height/32
        }
    }

    Rectangle {
        id: dataManagerButton

        signal clicked
        visible: !Qt.inputMethod.visible

        anchors{
            left: homeScreen.left
            bottom: homeScreen.bottom
            leftMargin: homeScreen.width/40
            bottomMargin: homeScreen.height/24
        }

        height: patientIDTxt.height*2; width: patientIDTxt.width*2

        radius: 10
        color: dataManagerTouchArea.pressed ? "darkCyan" : "white"
        enabled: rootWin.appIsAlive & !rootWin.responsePending
        MouseArea {
            id: dataManagerTouchArea
            anchors.fill: parent
            onClicked:{
                console.log("clicked")
                rootWin.currentScreen = CommonDefinitions.ScreensState.FileManager
            }
        }
        Text {
            text: "   File\nManager"
            color: dataManagerTouchArea.pressed ? "white" : "black"
            anchors.centerIn: parent
            font.family: "Helvetica"
            font.bold: true
            font.pointSize: homeScreen.height/32
        }
    }
}
