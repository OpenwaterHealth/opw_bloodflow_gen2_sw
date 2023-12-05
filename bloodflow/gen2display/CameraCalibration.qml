import QtQuick 2.12
import QtQuick.Controls 1.4
Item {
    id: cameraCalibrationScreen

    anchors.fill: parent
    Image{
        id: cameraCalibrationScreenLogo
        anchors{
            horizontalCenter: cameraCalibrationScreen.horizontalCenter
            top: cameraCalibrationScreen.top
            topMargin: cameraCalibrationScreen.height/20
        }
        width: cameraCalibrationScreen.width/4 ; height:cameraCalibrationScreen.height/10
        source: "qrc:/images/OpenwaterLogo.png"
    }

    Image{
        id: cameraCalibrationScreenHome

        signal clicked

        anchors{
            left: cameraCalibrationScreen.left
            top: cameraCalibrationScreen.top
            topMargin: cameraCalibrationScreen.height/40
            leftMargin: cameraCalibrationScreen.width/40
        }
        width: cameraCalibrationScreen.width/16 ; height:cameraCalibrationScreen.height/9
        source: "qrc:/images/home.svg"
        opacity: cameraCalibrationScreenHomeButton.pressed ? 0.25 : 1

        MouseArea {
            id: cameraCalibrationScreenHomeButton
            anchors.fill: parent
            onClicked:{
                rootWin.currentScreen = CommonDefinitions.ScreensState.MaintainanceHome
            }
        }
    }

    Rectangle{
        id: moduleForCalibrationSelector
        anchors{
            left: cameraCalibrationScreen.left
            leftMargin: cameraCalibrationScreen.width/40
            top: cameraCalibrationScreenLogo.bottom
        }
        width: cameraCalibrationScreen.width*7/15 ; height:cameraCalibrationScreen.height*3/4

        ModuleSelector {
            id: moduleSelectorForCalibration
            anchors.fill : parent
        }
        property alias rFissMod : moduleSelectorForCalibration.rFissMod
        property alias lFissMod : moduleSelectorForCalibration.lFissMod
        property alias rForeMod : moduleSelectorForCalibration.rForeMod
        property alias lForeMod : moduleSelectorForCalibration.lForeMod

        property alias rFissModEnable: moduleSelectorForCalibration.rFissModEnable
        property alias rForeModEnable: moduleSelectorForCalibration.rForeModEnable
        property alias lFissModEnable: moduleSelectorForCalibration.lFissModEnable
        property alias lForeModEnable: moduleSelectorForCalibration.lForeModEnable
    }

    Rectangle {
        id: calibrationInstructions

        anchors{
            left: cameraCalibrationScreen.horizontalCenter
            top: cameraCalibrationScreenLogo.bottom
            topMargin: cameraCalibrationScreenLogo.height/3
        }

        height: cameraCalibrationScreen.height/15; width: cameraCalibrationScreen.width/2

        Text {
            text: "Select a module to begin calibration"
            color: "darkMagenta"
            font.family: "Helvetica"
            font.bold: true
            font.pointSize: cameraCalibrationScreen.height/48
        }
    }
    Rectangle {
        id: calibrateButton

        anchors{
            horizontalCenter: calibrationInstructions.horizontalCenter
            top: calibrationInstructions.bottom
        }

        height: cameraCalibrationScreen.height/10; width: cameraCalibrationScreen.width/4
        antialiasing: true
        radius: 10
        color: calibrateButtonMouseArea.pressed ? "darkCyan" : "darkGreen"

        MouseArea {
            id: calibrateButtonMouseArea
            anchors.fill: parent
            onClicked:{
                var moduleSelected = moduleForCalibrationSelector.rFissMod * CommonDefinitions.ModuleSequence.RFissure +
                                     moduleForCalibrationSelector.rForeMod * CommonDefinitions.ModuleSequence.RForehead +
                                     moduleForCalibrationSelector.lFissMod * CommonDefinitions.ModuleSequence.LFissure +
                                     moduleForCalibrationSelector.lForeMod * CommonDefinitions.ModuleSequence.LForehead
                var jsonString = "{ \"cmd\": {\"calibrateScreen\": " + moduleSelected + " }}"
                var commString = qsTr(jsonString)
                console.log(commString)
                submitJSONString(commString)
            }
        }

        Text {
            text: "Calibrate Module"
            color: "white"
            anchors.centerIn: parent
            font.family: "Helvetica"
            font.bold: true
            font.pointSize: cameraCalibrationScreen.height/48
        }
    }

    Rectangle {
        id: calibrationTableHolder

        anchors{
            left: cameraCalibrationScreen.horizontalCenter
            top: calibrateButton.bottom
            topMargin: cameraCalibrationScreen.height/48
        }

        height: cameraCalibrationScreen.height*6/13; width: cameraCalibrationScreen.width/2-20

        TableView {
            anchors.fill: parent
            TableViewColumn {
                role: "module"
                title: "Module"
                width: 200
            }
            TableViewColumn {
                role: "camera"
                title: "Camera"
                width: 150
            }
            TableViewColumn {
                role: "contrast"
                title: "Contrast"
                width: 150
            }
            TableViewColumn {
                role: "mean"
                title: "Mean"
                width: 100
            }
            model: myModel
        }

        ListModel {
            id: myModel
            ListElement {
                module: "Left Temple"
                camera: "Near"
                mean:   "100"
                contrast: "0.323"
            }
            ListElement {
                module: "Left Temple"
                camera: "Far"
                mean:   "100"
                contrast: "0.323"
            }
            ListElement {
                module: "Left Forehead"
                camera: "Near"
                mean:   "100"
                contrast: "0.323"
            }
            ListElement {
                module: "Left Forehead"
                camera: "Far"
                mean:   "100"
                contrast: "0.323"
            }
            ListElement {
                module: "Right Forehead"
                camera: "Near"
                mean:   "100"
                contrast: "0.323"
            }
            ListElement {
                module: "Right Forehead"
                camera: "Far"
                mean:   "100"
                contrast: "0.323"
            }
            ListElement {
                module: "Right Temple"
                camera: "Near"
                mean:   "100"
                contrast: "0.323"
            }
            ListElement {
                module: "Right Temple"
                camera: "Far"
                mean:   "100"
                contrast: "0.323"
            }
        }
    }

    Rectangle {
        id: applyCalibration

        anchors{
            right: calibrationTableHolder.horizontalCenter
            rightMargin: cameraCalibrationScreen.width/40
            top: calibrationTableHolder.bottom
            topMargin: cameraCalibrationScreen.height/30
        }

        height: cameraCalibrationScreen.height/9; width: cameraCalibrationScreen.width/5
        antialiasing: true
        radius: 10
        color: applyCalsMouseArea.pressed ? "darkCyan" : "darkGreen"

        MouseArea {
            id: applyCalsMouseArea
            anchors.fill: parent
            onClicked:{
                //Save set change future calibration association to new set
            }
        }

        Text {
            text: "     Apply\nCalibration"
            color: "white"
            anchors.centerIn: parent
            font.family: "Helvetica"
            font.bold: true
            font.pointSize: cameraCalibrationScreen.height/48
        }
    }

    Rectangle {
        id: saveCalibration

        anchors{
            left: calibrationTableHolder.horizontalCenter
            leftMargin: cameraCalibrationScreen.width/40
            top: calibrationTableHolder.bottom
            topMargin: cameraCalibrationScreen.height/30
        }

        height: applyCalibration.height; width: applyCalibration.width
        antialiasing: true
        radius: 10
        color: saveCalsMouseArea.pressed ? "darkCyan" : "darkGreen"

        MouseArea {
            id: saveCalsMouseArea
            anchors.fill: parent
            onClicked:{
                //Save set change future calibration association to new set
            }
        }

        Text {
            text: "       Log\nCalibration"
            color: "white"
            anchors.centerIn: parent
            font.family: "Helvetica"
            font.bold: true
            font.pointSize: cameraCalibrationScreen.height/48
        }
    }
}
