import QtQuick 2.12

Item {
    id: scanCompleteScreen
    anchors.fill: parent

    Component.onCompleted: {
        rootWin.screenName = "Scan complete"
    }

    Image{
        id: scanCompleteLogo
        anchors{
            horizontalCenter: scanCompleteScreen.horizontalCenter
            top: scanCompleteScreen.top
            topMargin: scanCompleteScreen.height/20
        }
        width: scanCompleteScreen.width/4 ; height:scanCompleteScreen.height/10
        source: "qrc:/images/OpenwaterLogo.png"
    }

    Rectangle {
        id: patientIDDisp

        anchors{
            right: scanCompleteScreen.right
            top:  scanCompleteScreen.top
            topMargin: scanCompleteScreen.height/12
            rightMargin: scanCompleteScreen.width/12
        }

        height: scanCompleteScreen.height/20; width: scanCompleteScreen.width/5
        color: "white"

        Text {
            text: "Patient ID: " + rootWin.patientID
            color: "darkMagenta"
            anchors.centerIn: parent
            font.family: "Helvetica"
            font.bold: true
            font.pointSize: scanCompleteScreen.height/36
        }
    }

    Text {
        id: notesTxt
        anchors{
            left: scanCompleteScreen.horizontalCenter
            leftMargin: scanCompleteScreen.width/20
            top: scanCompleteScreen.top
            topMargin: scanCompleteScreen.height/5
        }
        text: "Notes:"
        antialiasing: true
        font.family: "Helvetica"
        font.pointSize: scanCompleteScreen.height/48
        color: "Gray"
    }

    Rectangle {
        id: notesInputBox

        property alias notesEnteredLocal: notesEntered.text
    
        Component.onCompleted: {
            notesEntered.text = rootWin.patientNote
        }

        anchors{
            left: notesTxt.left
            top: notesTxt.bottom
        }
        height: notesTxt.height*5; width: scanCompleteScreen.width/3
        color: "lightGray"
        radius: 4
        border{
            color: "darkGreen"
            width: notesTxt.height/20
        }

        TextInput{
            id: notesEntered
            anchors{
                leftMargin: parent.width/20
                fill: parent
            }
            color: focus ? "black" : "gray"
            font.pixelSize: parent.height/5 - 10
            wrapMode: TextInput.WordWrap
        }
    }

    Rectangle {
        id: saveNoteButton

        signal clicked

        anchors{
            right: notesInputBox.right
            top: notesInputBox.bottom
            topMargin: notesEntered.height/20
        }

        height: scanCompleteScreen.height/12; width: scanCompleteScreen.width/10
        antialiasing: true
        radius: 10
        color: saveNoteButtonMouseArea.pressed ? "darkCyan" : "darkGreen"

        MouseArea {
            id: saveNoteButtonMouseArea
            anchors.fill: parent
            onClicked:{
                sendUserCommandToTDA4(consts.cmd_save_note,
                        notesInputBox.notesEnteredLocal)
                rootWin.patientNote = notesInputBox.notesEnteredLocal
                Qt.inputMethod.hide()
            }
        }

        Text {
            text: "Save"
            color: "white"
            anchors.centerIn: parent
            font.family: "Helvetica"
            font.bold: true
            font.pointSize: scanCompleteScreen.height/48
        }
    }

    Row {
        id: buttonRow
        anchors.bottom: parent.bottom
        anchors.leftMargin: 64
        anchors.bottomMargin: 120
        width: parent.width - 128;
        spacing: (parent.width - 128 - (4*216))/3
        anchors{
            left: parent.left
        }

        Rectangle {
            id: scanCompleteHomeButton

            signal clicked

            width: 216
            height: 100
            border.color: "black"
            radius: 0.25 * height
            color: "darkGreen"

            property string text: 'Home'
            MouseArea {
                id: scanCompleteHomeButtonMouseArea
                anchors.fill: parent
                onPressed: parent.opacity = 0.5
                onReleased:parent.opacity = 1.0
                onCanceled: parent.opacity = 1.0
                onClicked:{
                    sendUserCommandToTDA4(consts.cmd_close_patient)
                }
            }

            Image{
                id: homeImage
                anchors{
                    left: parent.left
                    verticalCenter: parent.verticalCenter
                    topMargin: parent.height/5
                    leftMargin: parent.width/20
                }
                width: height; height:parent.height-parent.height/12
                source: "qrc:/images/home.svg"
            }

            Text {
                text: scanCompleteHomeButton.text
                color: "white"
                font.pointSize: 14
                anchors.centerIn: parent
                leftPadding: homeImage.width
                font.family: "Helvetica"
                font.bold: true
            }

        }

        Rectangle {
            id: repeatHeadsetTestButton

            signal clicked

            width: 216
            height: 100
            border.color: "black"
            radius: 0.25 * height
            color: "darkGreen"

            property string text: 'Repeat\nHeadset Test'

            Text {
                text: repeatHeadsetTestButton.text
                horizontalAlignment: Text.AlignHCenter
                color: "white"
                font.pointSize: 14
                anchors.centerIn: parent
                font.family: "Helvetica"
                font.bold: true
            }

            MouseArea {
                id: repeatHeadsetTestMouseArea
                anchors.fill: parent
                onPressed: parent.opacity = 0.5
                onReleased:parent.opacity = 1.0
                onCanceled: parent.opacity = 1.0
                onClicked:{
                    rootWin.currentScreen = CommonDefinitions.ScreensState.Placement
                }
            }
        }

        Rectangle {
            id: repeatScanButton

            width: 216
            height: 100
            border.color: "black"
            radius: 0.25 * height
            color: "darkGreen"

            property string text: 'Repeat\nScan'

            Text {
                text: repeatScanButton.text
                horizontalAlignment: Text.AlignHCenter
                color: "white"
                font.pointSize: 14
                anchors.centerIn: parent
                font.family: "Helvetica"
                font.bold: true
            }

            MouseArea {
                id: repeatScanMouseArea
                anchors.fill: parent
                onPressed: parent.opacity = 0.5
                onReleased:parent.opacity = 1.0
                onCanceled: parent.opacity = 1.0
                onClicked:{
                    rootWin.patientNote = ""
                    notesInputBox.notesEnteredLocal = ""
                    sendUserCommandToTDA4(consts.cmd_scan_patient, "full")
                }
            }
        }

        Rectangle {
            id: btnGraph

            width: 216
            height: 100
            border.color: "black"
            radius: 0.25 * height
            color: "darkGreen"

            property string text: 'View Data'

            Text {
                text: btnGraph.text
                horizontalAlignment: Text.AlignHCenter
                color: "white"
                font.pointSize: 14
                anchors.centerIn: parent
                font.family: "Helvetica"
                font.bold: true
            }

            MouseArea {
                anchors.fill: parent
                onPressed: parent.opacity = 0.5
                onReleased:parent.opacity = 1.0
                onCanceled: parent.opacity = 1.0
                onClicked:{
                    rootWin.patientNote = notesInputBox.notesEnteredLocal
                    rootWin.currentScreen = 
                            CommonDefinitions.ScreensState.PatientDataView
                }
            }
        }

    }


    Rectangle{
        id: scanCompletionStatus

        property bool scanPassedBool: true
        property string scanStatusString: scanPassedBool ? "Scan successfully completed!" : "Scan failed :("

        anchors{
            left: parent.left
            top: notesInputBox.top
            leftMargin: parent.width/10
        }
        width: parent.width/3 ; height:parent.height/3

        Rectangle{
            id: completionStatusMarkBox
            anchors{
                left: parent.left
                top: parent.top
            }
            width: parent.width/4 ; height:width

            Image{
                anchors.fill: parent
                source: "qrc:/images/check.png"
                opacity: scanCompletionStatus.scanPassedBool ? 1 : 0
            }

            Image{
                anchors.fill: parent
                source: "qrc:/images/cross.svg"
                opacity: scanCompletionStatus.scanPassedBool ? 0 : 1
            }
        }

        Rectangle{
            anchors{
                left: completionStatusMarkBox.right
                leftMargin: parent.width/20
                top: completionStatusMarkBox.top
            }
            width: parent.width-completionStatusMarkBox.width
            height: parent.height

            Text {
                text: scanCompletionStatus.scanStatusString
                color: "black"
                anchors.fill: parent
                wrapMode: Text.WordWrap
                font.family: "Helvetica"
                font.bold: true
                font.pointSize: scanCompleteScreen.height/40
            }
        }
    }

}

/*##^##
Designer {
    D{i:0;autoSize:true;height:480;width:640}D{i:12}D{i:13}D{i:14}
}
##^##*/
