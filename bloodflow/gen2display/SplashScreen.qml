import QtQuick 2.12
import QtQuick.Layouts 1.12

Item {
    id: splashScreen
    anchors.fill: parent

    Component.onCompleted: {
        rootWin.screenName = "Splash"
    }

    Image{
        id: logo
        anchors{
            horizontalCenter: splashScreen.horizontalCenter
            top: splashScreen.top
            topMargin: splashScreen.height/9
        }
        width: splashScreen.width/2 ; height:splashScreen.height/5
        source: "qrc:/images/OpenwaterLogo.png"
    }

    Text {
        id: shutdownTxt
        anchors{
            right: splashScreen.right
            top: splashScreen.top
            rightMargin: powerOffButton.width * 3 / 2
        }
        text: "To prevent damage tap this shutdown button\nbefore using " +
                "the main power switch"
        antialiasing: true
        font.family: "Helvetica"
        font.pointSize: splashScreen.height / 58
        color: "Gray"
    }

    Text {
        id: startupTxt
        anchors{
            horizontalCenter: splashScreen.horizontalCenter
            verticalCenter: splashScreen.verticalCenter
        }
        text: "Initializing..."
        antialiasing: true
        font.family: "Helvetica"
        font.pointSize: splashScreen.height / 14
        color: "Black"
    }

    Image{
        id: powerOffButton

        signal clicked

        anchors{
            right: splashScreen.right
            top: splashScreen.top
            topMargin: splashScreen.height/40
            rightMargin: splashScreen.width/40
        }
        width: splashScreen.width/16 ; height:splashScreen.height/9
        source: "qrc:/images/powerButton.png"
        opacity: powerOffButton.pressed ? 0.25 : 1

        MouseArea {
            id: powerOffButtonArea
            anchors.fill: parent
            onClicked:{
                if (rootWin.appIsAlive) {
                    showInfoDialog("Would you like to shutdown the device?", 
                            true, false)
                    console.log("Splash shutdown button pressed")
                    rootWin.requestShutdown = true
                } else {
                    // It shouldn't be possible for the display to power
                    //  up and the application to not be responsive, but
                    //  if it happens...
                    setError("Unable to communicate with application. " +
                            "Please see service manual.")
                }
            }
        }
    }
}

/*##^##
Designer {
    D{i:0;autoSize:true;height:480;width:640}
}
##^##*/
