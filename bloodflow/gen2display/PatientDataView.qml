import QtQuick 2.0
import QtQuick.Window 2.0
import QtCharts 2.0
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.12

Item {
    id: patientDataView
    Component.onCompleted: {
        console.log("Patient View Instance")
        rootWin.screenName = "Patient data"
    }


    property bool pair1DataLoading : false
    property bool pair1DataLoaded : false
    property bool pair2DataLoading : false
    property bool pair2DataLoaded : false
    property bool pair3DataLoading : false
    property bool pair3DataLoaded : false
    property bool pair4DataLoading : false
    property bool pair4DataLoaded : false


    function delay(delayTime, cb) {
        timer.interval = delayTime;
        timer.repeat = false;
        timer.triggered.connect(cb);
        timer.start();
    }

    Timer {
        id: timer
    }


    TabBar {
        id: bar
        width: parent.width
        height: 48
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }

        TabButton {
            id: btnPair1
            text: qsTr("Horiz-Near")
            font.pointSize: 12
            height: bar.height
            enabled: true
            onClicked: {
                if (this.activeFocus){
                    console.log("pair1 visible")
                    if(!pair1DataLoading && !pair1DataLoaded){
                        // call for data
                        sendUserCommandToTDA4(consts.cmd_patient_data, "1")
                        pair1DataLoading = true

                        btnPair1.enabled = true
                        btnPair2.enabled = false
                        btnPair3.enabled = false
                        btnPair4.enabled = false

                    }
                }
            }
        }
        TabButton {
            id: btnPair2
            enabled: false
            text: qsTr("Horiz-Temple")
            font.pointSize: 12
            height: bar.height
            onClicked: {
                if (this.activeFocus){
                    console.log("pair2 visible")
                    if(!pair2DataLoading && !pair2DataLoaded){
                        // call for data
                        sendUserCommandToTDA4(consts.cmd_patient_data, "2")
                        pair2DataLoading = true

                        btnPair1.enabled = false
                        btnPair2.enabled = true
                        btnPair3.enabled = false
                        btnPair4.enabled = false

                    }
                }
            }
        }
        TabButton {
            id: btnPair3
            enabled: false
            text: qsTr("Near-Temple")
            font.pointSize: 12
            height: bar.height
            onClicked: {
                if (this.activeFocus){
                    console.log("pair3 visible")
                    if(!pair3DataLoading && !pair3DataLoaded){
                        // call for data
                        sendUserCommandToTDA4(consts.cmd_patient_data, "3")
                        pair3DataLoading = true

                        btnPair1.enabled = false
                        btnPair2.enabled = false
                        btnPair3.enabled = true
                        btnPair4.enabled = false

                    }
                }
            }
        }
        TabButton {
            id: btnPair4
            enabled: false
            text: qsTr("Horiz-Near")
            font.pointSize: 12
            height: bar.height
            onClicked: {
                if (this.activeFocus){
                    console.log("pair4 visible")
                    if(!pair4DataLoading && !pair4DataLoaded){
                        // call for data
                        sendUserCommandToTDA4(consts.cmd_patient_data, "4")
                        pair4DataLoading = true

                        btnPair1.enabled = false
                        btnPair2.enabled = false
                        btnPair3.enabled = false
                        btnPair4.enabled = true

                    }
                }
            }
        }
    }

    StackLayout {
        width: parent.width
        height: parent.height - bar.height
        currentIndex: bar.currentIndex
        anchors {
            top: bar.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }

        Item {
            id: pair1

            Component.onCompleted: {

                delay(2, function() {
                    sendUserCommandToTDA4(consts.cmd_patient_data, "1")
                    pair1DataLoading = true

                    btnPair1.enabled = true
                    btnPair2.enabled = false
                    btnPair3.enabled = false
                    btnPair4.enabled = false

                })

            }

            Image {
                id: p1c4
                width: parent.width
                height: (parent.height - bar.height)
                anchors {
                    top: pair1.top
                }
            }

            BusyIndicator {
                id: busyIndicatorPair1
                anchors.centerIn: parent
                visible: pair1DataLoading
                running: pair1DataLoading
            }

            Text {
                id: processingDataPair1
                text: "Processing data..."
                font.pointSize: 10
                color: "black"
                visible: pair1DataLoading
                anchors.top: busyIndicatorPair1.bottom
                anchors.horizontalCenter: parent.horizontalCenter
            }

        }
        Item {
            id: pair2

            Component.onCompleted: {

            }

            Image {
                id: p2c4
                width: parent.width
                height: (parent.height - bar.height)
                anchors {
                    top: pair2.top
                }
            }

            BusyIndicator {
                id: busyIndicatorPair2
                anchors.centerIn: parent
                visible: pair2DataLoading
                running: pair2DataLoading
            }

            Text {
                id: processingDataPair2
                text: "Processing data..."
                font.pointSize: 10
                color: "black"
                visible: pair2DataLoading
                anchors.top: busyIndicatorPair2.bottom
                anchors.horizontalCenter: parent.horizontalCenter
            }

        }
        Item {
            id: pair3

            Component.onCompleted: {

            }

            Image {
                id: p3c4
                width: parent.width
                height: (parent.height - bar.height)
                anchors {
                    top: pair3.top
                }
            }

            BusyIndicator {
                id: busyIndicatorPair3
                anchors.centerIn: parent
                visible: pair3DataLoading
                running: pair3DataLoading
            }

            Text {
                id: processingDataPair3
                text: "Processing data..."
                font.pointSize: 10
                color: "black"
                visible: pair3DataLoading
                anchors.top: busyIndicatorPair3.bottom
                anchors.horizontalCenter: parent.horizontalCenter
            }

        }
        Item {
            id: pair4

            Component.onCompleted: {

            }

            Image {
                id: p4c4
                width: parent.width
                height: (parent.height - bar.height)
                anchors {
                    top: pair4.top
                }
            }

            BusyIndicator {
                id: busyIndicatorPair4
                anchors.centerIn: parent
                visible: pair4DataLoading
                running: pair4DataLoading
            }

            Text {
                id: processingDataPair4
                text: "Processing data..."
                font.pointSize: 10
                color: "black"
                visible: pair4DataLoading
                anchors.top: busyIndicatorPair4.bottom
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
    }

    Rectangle {
        id: backButton

        signal clicked

        height: 48; width: parent.width/8
        border.color: "black"
        radius: 0.25 * height
        color: "darkGreen"
        enabled: rootWin.appIsAlive & !rootWin.responsePending
        opacity: enabled ? 1.0 : 0.25

        anchors{
            right: parent.right
            rightMargin: 5
            top: bar.bottom
            topMargin: 5
        }

        property string text: 'Back'

        Text {
            text: backButton.text
            horizontalAlignment: Text.AlignHCenter
            color: "white"
            font.pointSize: 12
            anchors.centerIn: parent
            font.family: "Helvetica"
            font.bold: true
        }

        MouseArea {
            id: backButtonMouseArea
            anchors.fill: parent
            onPressed: parent.opacity = 0.5
            onReleased:parent.opacity = 1.0
            onCanceled: parent.opacity = 1.0
            onClicked:{
                rootWin.currentScreen = CommonDefinitions.ScreensState.ScanComplete
            }
        }
    }

    function setDataFinished() {
        console.log("Data Download Complete")

        pair1DataLoading = false
        pair2DataLoading = false
        pair3DataLoading = false
        pair4DataLoading = false

        btnPair1.enabled = true
        btnPair2.enabled = true
        btnPair3.enabled = true
        btnPair4.enabled = true
    }

    function setDataError(){
        console.log("Data Error")

        pair1DataLoading = false
        pair2DataLoading = false
        pair3DataLoading = false
        pair4DataLoading = false

        btnPair1.enabled = true
        btnPair2.enabled = true
        btnPair3.enabled = true
        btnPair4.enabled = true
    }

    function setChartData(chartName, chartData) {
        console.log("setChartDataCalled ", chartName)
        var pos = 0
        var i = 0
        if(chartName === "P1C4"){
            console.log("setChartData P1C4")
            p1c4.source = "data:image/jpeg;base64," + chartData
            console.log("end setData P1C4")
            pair1DataLoading = false
            pair1DataLoaded = true
        }
        else if(chartName === "P2C4"){
            console.log("Populating P2C4")
            p2c4.source = "data:image/jpeg;base64," + chartData
            console.log("end setData P2C4")

            pair2DataLoading = false
            pair2DataLoaded = true

        }
        else if(chartName === "P3C4"){
            console.log("Populating P3C4")
            p3c4.source = "data:image/jpeg;base64," + chartData
            console.log("end setData P3C4")

            pair3DataLoading = false
            pair3DataLoaded = true

        }
        else if(chartName === "P4C4"){
            console.log("Populating P4C4")
            p4c4.source = "data:image/jpeg;base64," + chartData
            console.log("end setData P4C4")

            pair4DataLoading = false
            pair4DataLoaded = true
        }

    }
}
