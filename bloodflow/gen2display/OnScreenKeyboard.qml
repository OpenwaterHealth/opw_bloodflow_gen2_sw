import QtQuick 2.12
// Deliberately imported after QtQuick to avoid missing restoreMode property in Binding. Fix in Qt 6.
import QtQml 2.12
import QtQuick.Window 2.2
import QtQuick.Controls 2.5
import QtQuick.VirtualKeyboard 2.2
import QtQuick.VirtualKeyboard.Settings 2.2

Item {
    InputPanel {
        id: inputPanel
        z: 99
        x: 0
        y: Qt.inputMethod.visible ? 800 - inputPanel.height : 800
        width: 1280

//        Component.onCompleted: {
//            keyboard.style.keyboardBackground = null;
//            keyboard.style.selectionListBackground = null;
//        }
    }
}
