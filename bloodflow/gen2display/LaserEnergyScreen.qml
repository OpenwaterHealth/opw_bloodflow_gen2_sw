import QtQuick 2.12

// laserEnergyScreen
Item
{
    id: laserEnergyScreen
    property bool laserOn: false

    anchors.fill: parent

    Image{
        id: laserEnergyScreenLogo
        anchors{
            horizontalCenter: laserEnergyScreen.horizontalCenter
            top: laserEnergyScreen.top
            topMargin: laserEnergyScreen.height/20
        }
        width: laserEnergyScreen.width/4 ; height:laserEnergyScreen.height/10
        source: "qrc:/images/OpenwaterLogo.png"
    }

    Image{
        id: laserEnergyScreenHome

        signal clicked

        anchors{
            left: laserEnergyScreen.left
            top: laserEnergyScreen.top
            topMargin: laserEnergyScreen.height/40
            leftMargin: laserEnergyScreen.width/40
        }
        width: laserEnergyScreen.width/16 ; height:laserEnergyScreen.height/9
        source: "qrc:/images/home.svg"
        opacity: laserEnergyScreenHomeButton.pressed ? 0.25 : 1

        MouseArea {
            id: laserEnergyScreenHomeButton
            anchors.fill: parent
            enabled: !laserEnergyScreen.laserOn
            onClicked:{
                rootWin.currentScreen = CommonDefinitions.ScreensState.MaintainanceHome
            }
        }
    }

    Rectangle{
        id: laserModuleSelector
        anchors{
            left: laserEnergyScreen.left
            leftMargin: laserEnergyScreen.width/40
            top: laserEnergyScreenLogo.bottom
        }
        width: laserEnergyScreen.width*7/15 ; height:laserEnergyScreen.height*3/4

        ModuleSelector {
            id: moduleSelectorForLaserEnergy
            anchors.fill : parent
        }
        property alias rFissMod : moduleSelectorForLaserEnergy.rFissMod
        property alias lFissMod : moduleSelectorForLaserEnergy.lFissMod
        property alias rForeMod : moduleSelectorForLaserEnergy.rForeMod
        property alias lForeMod : moduleSelectorForLaserEnergy.lForeMod

        property alias rFissModEnable: moduleSelectorForLaserEnergy.rFissModEnable
        property alias rForeModEnable: moduleSelectorForLaserEnergy.rForeModEnable
        property alias lFissModEnable: moduleSelectorForLaserEnergy.lFissModEnable
        property alias lForeModEnable: moduleSelectorForLaserEnergy.lForeModEnable
    }

    Rectangle {
        id: laserInstructionsOrLaserWarning

        anchors{
            left: laserEnergyScreen.horizontalCenter
            top: laserEnergyScreen.top
            topMargin: laserEnergyScreen.height/4
        }

        height: laserEnergyScreen.height/5; width: laserEnergyScreen.width/2
        color: laserEnergyScreen.laserOn ? "yellow":"white"

        Text {
            text: laserEnergyScreen.laserOn ? "" : "Select a module on the left and press\nstart below to get continuous Laser\npulses"
            color: "darkMagenta"
            font.family: "Helvetica"
            font.bold: true
            font.pointSize: laserEnergyScreen.height/48
        }

        Image{
            id: laserWarningSticker
            anchors{
                left: parent.left
                top: parent.top
            }
            opacity: laserEnergyScreen.laserOn ? 1 : 0
            width: height; height:parent.height
            source: "qrc:/images/LaserWarning.png"
        }

        Rectangle{
            id: laserEnergyScreenLaserWarning
            anchors{
                left: laserWarningSticker.right
                top: parent.top
            }
            color: "yellow"
            width: laserEnergyScreen.laserOn ? parent.width-laserWarningSticker.width : undefined
            height: laserEnergyScreen.laserOn ? parent.height : undefined
            Text {
                text: laserEnergyScreen.laserOn ? " CAUTION\nLaser is on" : ""
                anchors.centerIn: parent
                color: "black"
                font.family: "Helvetica"
                font.bold: true
                font.pointSize: laserEnergyScreen.height/24
            }
        }
    }

    Rectangle {
        id: laserEnergyStartOrStopMeasButton

        anchors{
            left: laserEnergyScreen.horizontalCenter
            top: laserEnergyScreen.verticalCenter
            leftMargin: laserEnergyScreen.width/8
        }

        height: laserEnergyScreen.height/5; width: laserEnergyScreen.width/5
        antialiasing: true
        radius: 10
        color: laserEnergyMeasButtonMouseArea.pressed ? "darkCyan" : (laserEnergyScreen.laserOn ? "Red":"darkGreen")

        MouseArea {
            id: laserEnergyMeasButtonMouseArea
            anchors.fill: parent
            onClicked:{
                //Turn on laser if it is off and one of the modules is selected
                laserEnergyScreen.laserOn = !laserEnergyScreen.laserOn &&
                                           (laserModuleSelector.rFissMod || laserModuleSelector.lFissMod ||
                                            laserModuleSelector.rForeMod || laserModuleSelector.lForeMod ) ? true : false
                //Enable/disable module selection when laser is on
                laserModuleSelector.lForeModEnable = !laserEnergyScreen.laserOn
                laserModuleSelector.rForeModEnable = !laserEnergyScreen.laserOn
                laserModuleSelector.lFissModEnable = !laserEnergyScreen.laserOn
                laserModuleSelector.rFissModEnable = !laserEnergyScreen.laserOn

                // Call to D3 via c++
                var position = (laserModuleSelector.rFissMod * 1 + laserModuleSelector.lFissMod * 2 +
                               laserModuleSelector.rForeMod * 3 + laserModuleSelector.lForeMod * 4) * laserEnergyScreen.laserOn
                var moduleSelected = (laserModuleSelector.rFissMod * CommonDefinitions.ModuleSequence.RFissure +
                                     laserModuleSelector.rForeMod * CommonDefinitions.ModuleSequence.RForehead +
                                     laserModuleSelector.lFissMod * CommonDefinitions.ModuleSequence.LFissure +
                                     laserModuleSelector.lForeMod * CommonDefinitions.ModuleSequence.LForehead) * laserEnergyScreen.laserOn
                var jsonString = "{ \"cmd\": {\"laserScreen\": " + moduleSelected + " }}"
                var commString = qsTr(jsonString)
                console.log(commString)
                submitJSONString(commString)
            }
        }

        Text {
            text: laserEnergyScreen.laserOn ? "Stop" : "  Start Laser\n      Energy\nMeasurement"
            color: "white"
            anchors.centerIn: parent
            font.family: "Helvetica"
            font.bold: true
            font.pointSize: laserEnergyScreen.height/48
        }
    }
}
