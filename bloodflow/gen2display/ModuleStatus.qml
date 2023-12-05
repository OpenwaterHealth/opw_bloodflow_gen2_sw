import QtQuick 2.12
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4

Item {
    Image{
        id: laserHead
        anchors.fill: parent
        source: "qrc:/images/Headset.png"
    }

    property alias rFissMod: rightFissureModule.checked
    property alias rForeMod: rightForeheadModule.checked
    property alias lFissMod: leftFissureModule.checked
    property alias lForeMod: leftForeheadModule.checked

    property alias rFissModEnable: rightFissureModule.enabled
    property alias rForeModEnable: rightForeheadModule.enabled
    property alias lFissModEnable: leftFissureModule.enabled
    property alias lForeModEnable: leftForeheadModule.enabled

    RadioButton {
        id: rightFissureModule
        anchors{
            horizontalCenter: laserHead.horizontalCenter
            horizontalCenterOffset: -laserHead.width*2/7
            top:  laserHead.top
            topMargin: laserHead.height*3/10
        }
        style: RadioButtonStyle {
            indicator: Rectangle {
                height: laserHead.height/10; width: height
                antialiasing: true
                radius: height*0.5
                color: rootWin.rFissMod
                enabled: false
            }
        }
    }

    RadioButton {
        id: leftFissureModule
        anchors{
            horizontalCenter: laserHead.horizontalCenter
            horizontalCenterOffset: laserHead.width*2/7
            top:  laserHead.top
            topMargin: laserHead.height*3/10
        }
        style: RadioButtonStyle {
            indicator: Rectangle {
                height: laserHead.height/10; width: height
                antialiasing: true
                radius: height*0.5
                color: rootWin.lFissMod
                enabled: false
            }
        }
    }

    RadioButton {
        id: rightForeheadModule
        anchors{
            horizontalCenter: laserHead.horizontalCenter
            horizontalCenterOffset: -laserHead.width*1/6
            top:  laserHead.top
            topMargin: laserHead.height*1/6
        }
        style: RadioButtonStyle {
            indicator: Rectangle {
                height: laserHead.height/10; width: height
                antialiasing: true
                radius: height*0.5
                color: rootWin.rForeMod
                enabled: false
            }
        }
    }

    RadioButton {
        id: leftForeheadModule
        anchors{
            horizontalCenter: laserHead.horizontalCenter
            horizontalCenterOffset: laserHead.width*1/6
            top:  laserHead.top
            topMargin: laserHead.height*1/6
        }
        style: RadioButtonStyle {
            indicator: Rectangle {
                height: laserHead.height/10; width: height
                antialiasing: true
                radius: height*0.5
                color: rootWin.lForeMod
                enabled: false
            }
        }
    }
}
