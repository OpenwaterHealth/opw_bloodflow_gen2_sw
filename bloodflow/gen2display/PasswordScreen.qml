import QtQuick 2.12

//import QtQuick.Controls 2.15
import QtQuick.Dialogs 1.3
import Crypto 1.0


Item {
    id: pwdScreen
    anchors.fill: parent

    property string storedPasswordHash: "d7a4a8193b9183c915c5e2859b9c790cc70df6d6317e943c3a15abe5bd0dee3e" // ProblemChild

    Component.onCompleted: {
        rootWin.screenName = "Password"
    }

    Image{
        id: logo
        anchors{
            horizontalCenter: pwdScreen.horizontalCenter
            top: pwdScreen.top
            topMargin: pwdScreen.height/20
        }
        width: pwdScreen.width/4 ; height:pwdScreen.height/10
        source: "qrc:/images/OpenwaterLogo.png"
    }

    Image{
        id: pwdScreenHome

        signal clicked

        anchors{
            left: pwdScreen.left
            top: pwdScreen.top
            topMargin: pwdScreen.height/40
            leftMargin: pwdScreen.width/40
        }
        width: pwdScreen.width/16 ; height:pwdScreen.height/9
        source: "qrc:/images/home.svg"
        opacity: pwdScreenHomeButton.pressed ? 0.25 : 1

        MouseArea {
            id: pwdScreenHomeButton
            anchors.fill: parent
            onClicked:{
                rootWin.currentScreen = CommonDefinitions.ScreensState.Home
            }
        }
    }

    Text {
        id: pwdBoxTxt
        anchors{
            left: pwdScreen.left
            leftMargin: pwdScreen.width/4
            bottom: pwdScreen.verticalCenter
            bottomMargin: pwdScreen.height/12
        }
        text: "Password"
        antialiasing: true
        font.family: "Helvetica"
        font.pointSize: pwdScreen.height/32
        color: "Gray"
    }

    Rectangle {
        id: pwdBox

        property alias password: pwdEntered.text

        anchors{
            left: pwdBoxTxt.right
            leftMargin: pwdScreen.width/40
            top: pwdBoxTxt.top
        }
        height: pwdBoxTxt.height; width: pwdScreen.width/4
        color: "lightGray"
        border{
            color: "gray"
            width: pwdBox.height/20
        }
        radius: 4

        TextInput{
            id: pwdEntered
            anchors{
                leftMargin: parent.width/20
                fill: parent
            }
            color: focus ? "black" : "gray"
//            font.pixelSize: parent.height - 10
            font.pointSize: 12

            text: ""
            echoMode: TextInput.Password
        }
    }

    Text {
        id: messageText
        visible: false
        anchors{
            top: logo.bottom
            topMargin: 2*pwdBox.height/3
        }
        anchors.horizontalCenter: parent.horizontalCenter
        font.pointSize: 16
        font.bold: true
        color: "red"
        text: "Incorrect password!"
    }

    Rectangle {
        id: pwdButton

        signal clicked

        anchors{
            left: pwdBox.right
            leftMargin: pwdScreen.width/40
            top: pwdBoxTxt.top
        }

        height: pwdBoxTxt.height; width: pwdBoxTxt.width
        antialiasing: true
        radius: 10
        color: pwdMouseArea.pressed ? "darkCyan" : "darkGreen"

        MouseArea {
            id: pwdMouseArea
            anchors.fill: parent
            onClicked:{
                checkPassword();
            }
        }

        Text {
            text: "Enter"
            color: "white"
            anchors.centerIn: parent
            font.family: "Helvetica"
            font.bold: true
            font.pointSize: pwdScreen.height/32
        }
    }

    CryptoManager {
            id: crypto
    }

    function checkPassword() {
        var inputPassword = pwdEntered.text;
        var ciphertext = crypto.encrypt(inputPassword);

        if (ciphertext === storedPasswordHash) {
            // Password is correct, push a new screen
            rootWin.currentScreen = CommonDefinitions.ScreensState.MaintainanceHome

        } else {
            // Password is incorrect, show error message
            // console.log("Password hash: " + ciphertext)
            messageText.visible = true;
        }
    }
}
