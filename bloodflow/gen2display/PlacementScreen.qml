import QtQuick 2.12
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4

Item {
    id: placementScreen
    anchors.fill: parent
    property bool showPlacementGuide : false

    Component.onCompleted: {
        rootWin.screenName = "Placement"
        rootWin.patientNote = ""
        rootWin.headsetTested = false
        rootWin.rFissMod = consts.cam_module_unknown_color
        rootWin.rForeMod = consts.cam_module_unknown_color
        rootWin.lFissMod = consts.cam_module_unknown_color
        rootWin.lForeMod = consts.cam_module_unknown_color
    }

    Image{
        id: logo
        anchors{
            horizontalCenter: placementScreen.horizontalCenter
            top: placementScreen.top
            topMargin: placementScreen.height/20
        }
        width: placementScreen.width/4 ; height:placementScreen.height/10
        source: "qrc:/images/OpenwaterLogo.png"
    }

    Image{
        id: placementScreenHome

        signal clicked

        anchors{
            left: placementScreen.left
            top: placementScreen.top
            topMargin: placementScreen.height/40
            leftMargin: placementScreen.width/40
        }
        width: placementScreen.width/16 ; height:placementScreen.height/9
        source: "qrc:/images/home.svg"
        opacity: placementScreenHomeMouseArea.pressed ? 0.25 : 1

        MouseArea {
            id: placementScreenHomeMouseArea
            anchors.fill: parent
            onClicked:{
                sendUserCommandToTDA4(consts.cmd_close_patient)
            }
        }
    }

    Rectangle {
        id: patientIDDisp

        anchors{
            right: placementScreen.right
            top:  placementScreen.top
            topMargin: placementScreen.height/12
            rightMargin: placementScreen.width/12
        }

        height: placementScreen.height/20; width: placementScreen.width/5
        color: "white"

        Text {
            text: "Patient ID: " + rootWin.patientID
            color: "darkMagenta"
            anchors.centerIn: parent
            font.family: "Helvetica"
            font.bold: true
            font.pointSize: placementScreen.height/36
        }
    }

    Rectangle {
        id: placementGuideButton

        anchors{
            right: placementScreen.right
            bottom: placementScreen.verticalCenter
            rightMargin: placementScreen.width/8
            bottomMargin: placementScreen.height/12
        }

        height: placementScreen.height/6; width: placementScreen.width/6
        antialiasing: true
        radius: 10
        color: placementGuideMouseArea.pressed ? "darkCyan" : "darkGreen"

        MouseArea {
            id: placementGuideMouseArea
            anchors.fill: parent
            onClicked:{
                showPlacementGuide = true
            }
        }

        Text {
            text: "    Open\nPlacement\n    Guide"
            color: "white"
            anchors.centerIn: parent
            font.family: "Helvetica"
            font.bold: true
            font.pointSize: placementScreen.height/48
        }
    }

    Rectangle {
        id: testHeadsetButton

        signal clicked

        anchors{
            left: placementGuideButton.left
            top: placementScreen.verticalCenter
        }

        height: placementScreen.height/9; width: placementScreen.width/6
        antialiasing: true
        radius: 10
        color: testHeadsetMouseArea.pressed ? "darkCyan" : "darkGreen"

        MouseArea {
            id: testHeadsetMouseArea
            anchors.fill: parent
            enabled: !showPlacementGuide
            onClicked:{
                sendUserCommandToTDA4(consts.cmd_scan_patient, "test")
                startScanMouseArea.enabled = true
                startLongScanMouseArea.enabled = true
            }
        }

        Text {
            text: "   Test\nHeadset"
            color: "white"
            anchors.centerIn: parent
            font.family: "Helvetica"
            font.bold: true
            font.pointSize: placementScreen.height/48
        }
    }

    Rectangle {
        id: startScanButton

        signal clicked

        anchors{
            left: placementGuideButton.left
            bottom: placementScreen.bottom
            bottomMargin: placementScreen.height/5
        }

        height: placementScreen.height/9; width: placementScreen.width/6
        antialiasing: true
        radius: 10
        color: startScanMouseArea.pressed || !rootWin.headsetTested ? "darkCyan" : "darkGreen"
        enabled: rootWin.headsetTested

        Text {
            text: "Begin\n Scan"
            color: "white"
            anchors.centerIn: parent
            font.family: "Helvetica"
            font.bold: true
            font.pointSize: placementScreen.height/48
        }
        MouseArea {
            id: startScanMouseArea
            anchors.fill: startScanButton
            enabled: false
            onClicked:{
                //rootWin.currentScreen = CommonDefinitions.ScreensState.ScanComplete
                rootWin.patientNote = ""
                sendUserCommandToTDA4(consts.cmd_scan_patient, "full")
            }
        }
    }

    Rectangle {
        id: startLongScanButton

        signal clicked

        anchors{
            left: parent.left
            leftMargin: 25
            bottom: placementScreen.bottom
            bottomMargin: 75
        }

        height: placementScreen.height/9; width: placementScreen.width/6
        antialiasing: true
        radius: 10
        color: startLongScanMouseArea.pressed || !rootWin.headsetTested ? "darkCyan" : "darkGreen"
        enabled: rootWin.headsetTested

        Text {
            text: " Begin Long \n      Scan"
            color: "white"
            anchors.centerIn: parent
            font.family: "Helvetica"
            font.bold: true
            font.pointSize: placementScreen.height/48
        }
        MouseArea {
            id: startLongScanMouseArea
            anchors.fill: startLongScanButton
            enabled: false
            onClicked:{
                //rootWin.currentScreen = CommonDefinitions.ScreensState.ScanComplete
                rootWin.patientNote = ""
                sendUserCommandToTDA4(consts.cmd_scan_patient, "long")
            }
        }
    }


    Rectangle {
        id: head
        anchors{
            left: placementScreen.left
            verticalCenter: placementScreen.verticalCenter
            topMargin: placementScreen.height/4
            leftMargin: placementScreen.width/3
        }
        width: placementScreen.width/3 ; height:placementScreen.height/2
        ModuleStatus {
            id: moduleStatusItem
            rFissModEnable: false
            lFissModEnable: false
            rForeModEnable: false
            lForeModEnable: false

            anchors.fill : parent
        }
    }

    Image{
        id: placementGuideDisplay
        anchors.fill : parent
        source: "qrc:/images/PlacementGuide.png"
        visible: showPlacementGuide
    }

    Rectangle {
        id: backFromGuide

        anchors{
            right: placementScreen.right
            rightMargin: placementScreen.width/32
            bottom: placementScreen.bottom
            bottomMargin: placementScreen.height/10
        }
        visible: showPlacementGuide

        height: placementScreen.height/9; width: placementScreen.width/6
        antialiasing: true
        radius: 10
        color: exitGuideMouseArea.pressed ? "darkCyan" : "darkGreen"

        Text {
            text: " Exit\nGuide"
            color: "white"
            anchors.centerIn: parent
            font.family: "Helvetica"
            font.bold: true
            font.pointSize: placementScreen.height/48
        }

        MouseArea {
            id: exitGuideMouseArea
            anchors.fill: backFromGuide
            enabled: showPlacementGuide
            onClicked:{
                showPlacementGuide = false
            }
        }
    }

    ////////////////////////////////////////////////////////////////////
    // Camera module indicator color legend

    RadioButton {
        id: moduleUnknownLegend
        Text {
            text: "Unknown"
            color: "black"
            anchors.left: moduleUnknownLegend.left
            anchors.leftMargin: placementScreen.width/20
            font.family: "Helvetica"
            font.bold: true
            font.pointSize: placementScreen.height/36
        }
        visible: !showPlacementGuide
        anchors{
            left: placementScreen.left
            leftMargin: placementScreen.width/64
            top:  placementScreen.top
            topMargin: 2*placementScreen.height/8
        }
        style: RadioButtonStyle {
            indicator: Rectangle {
                height: placementScreen.height/20; width: height
                antialiasing: true
                radius: height*0.5
                color: consts.cam_module_unknown_color
                enabled: false
            }
        }
    }

    RadioButton {
        id: moduleReadyLegend
        Text {
            text: "Ready"
            color: "black"
            anchors.left: moduleReadyLegend.left
            anchors.leftMargin: placementScreen.width/20
            font.family: "Helvetica"
            font.bold: true
            font.pointSize: placementScreen.height/36
        }
        visible: !showPlacementGuide
        anchors{
            left: moduleUnknownLegend.left
            top: moduleUnknownLegend.top
            topMargin: placementScreen.width/20
        }
        style: RadioButtonStyle {
            indicator: Rectangle {
                height: placementScreen.height/20; width: height
                antialiasing: true
                radius: height*0.5
                color: consts.cam_module_ready_color
                enabled: false
            }
        }
    }

    RadioButton {
        id: moduleAmbientLegend
        Text {
            text: "Ambient light"
            color: "black"
            anchors.left: moduleAmbientLegend.left
            anchors.leftMargin: placementScreen.width/20
            font.family: "Helvetica"
            font.bold: true
            font.pointSize: placementScreen.height/36
        }
        visible: !showPlacementGuide
        anchors{
            left: moduleReadyLegend.left
            top: moduleReadyLegend.top
            topMargin: placementScreen.width/20
        }
        style: RadioButtonStyle {
            indicator: Rectangle {
                height: placementScreen.height/20; width: height
                antialiasing: true
                radius: height*0.5
                color: consts.cam_module_ambient_color
                enabled: false
            }
        }
    }

    RadioButton {
        id: moduleLaserLegend
        Text {
            text: "Laser contact"
            color: "black"
            anchors.left: moduleLaserLegend.left
            anchors.leftMargin: placementScreen.width/20
            font.family: "Helvetica"
            font.bold: true
            font.pointSize: placementScreen.height/36
        }
        visible: !showPlacementGuide
        anchors{
            left: moduleAmbientLegend.left
            top: moduleAmbientLegend.top
            topMargin: placementScreen.width/20
        }
        style: RadioButtonStyle {
            indicator: Rectangle {
                height: placementScreen.height/20; width: height
                antialiasing: true
                radius: height*0.5
                color: consts.cam_module_laser_color
                enabled: false
            }
        }
    }

    RadioButton {
        id: moduleAmbientLaserLegend
        Text {
            text: "Ambient light +\nlaser contact"
            color: "black"
            anchors.left: moduleAmbientLaserLegend.left
            anchors.leftMargin: placementScreen.width/20
            font.family: "Helvetica"
            font.bold: true
            font.pointSize: placementScreen.height/36
        }
        visible: !showPlacementGuide
        anchors{
            left: moduleLaserLegend.left
            top: moduleLaserLegend.top
            topMargin: placementScreen.width/20
        }
        style: RadioButtonStyle {
            indicator: Rectangle {
                height: placementScreen.height/20; width: height
                antialiasing: true
                radius: height*0.5
                color: consts.cam_module_ambient_laser_color
                enabled: false
            }
        }
    }

}
