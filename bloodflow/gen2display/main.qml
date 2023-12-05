import QtQuick 2.12
import QtQuick.Window 2.12
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4

// Program flow
// Splash screen is presented on startup. This is displayed until
//  set_state=OW_STATE_HOME is received. The only UI activity before
//  the home state is reached are popup dialogs (info or error).
// Once home state is reached, the Home screen will appear. The home
//  state can be toggled between home and mainetnance screens. When
//  a patient ID is entered, the OW_STATE_PATIENT state is entered.
//  This state persists, using different patient screens, until a
//  return to the IDLE state, or SHUTDOWN.

// Note that an idiosyncracy of QML is that objects lose state when
//  they're invisible and are reloaded, restoring default values
//  to all variables and fields. If one needs to maintain state for
//  a screen/view, that state must be stored in the rootWin.


Window {
    id: rootWin

    signal submitJSONString(string json)
  
    visible: true
    width: screenWidth
    height: screenHeight
    title: qsTr("Openwater Gen2")

    // TODO define a mechanism/protocol for when to advance version number,
    //  and automate if possible
    readonly property string touchscreenVersion: "1.1.6"

    property string appState : consts.state_startup
    property int currentScreen : CommonDefinitions.ScreensState.Splash
    property string appVersionString : "App "
    property string patientID : "None"

    property string patientNote : ""

    property string screenName : "None"

    // When a user submits a request to the App, the UI controls should
    //  prevent further actions, so keep track of response pending state.
    property bool responsePending : false
    property bool backupPending : false
    property int responsePendingCountdown : 0

    // Once communication is established with the App, this is set to
    //  true. Once a shutdown signal is received from the App, it's set
    //  to false. If false, assume the app is not alive, so any shutdown
    //  request is for the display only.
    property bool appIsAlive : false
    // Stores if we've had successful communication with app. Like 'is alive'
    //  but never changes once true (is alive can be false on shutdown)
    property bool firstContact : false

    // Flag to set once test scan is complete
    property bool headsetTested : false

    // Flag to remember if flash drive is connected
    property bool flashDriveConnected : false
    property string flashDriveFreeStorage : "0MB"
    property string internalStorageFree : "0MB"

    // Popup dialog controls
    property bool infoPopupDisplay : false
    property bool infoShowCancel : false
    property string infoPopupString : ""
    property string infoPopupStringParam : ""
    // The popup strings are the text content of the dialogs having
    //  content to display to the user. The error param string is shown
    //  to the user but the contents are more technical in nature, e.g.,
    //  to provide to tech support.
    property bool errorPopupDisplay : false
    property string errorPopupString : ""
    property string errorPopupStringParam : ""
    property bool popupShutdownOnly : false

    // States to keep so dialog knows what to do if 'ok' is pressed
    property bool requestShutdown : false
    // Set to true if user has requested a deletion of previously
    //  backed-up files
    property bool purgeRequested : false

    property bool estop : false

    //Instantiate to access properties and functions
    CommonDefinitions{
        id: consts
    }

    // Camera module states
    property string rFissMod : consts.cam_module_unknown_color
    property string rForeMod : consts.cam_module_unknown_color
    property string lFissMod : consts.cam_module_unknown_color
    property string lForeMod : consts.cam_module_unknown_color

    // Send message to application on startup to check it's state and
    //  make sure it's alive and to see what state it's in (in case the
    //  UI restarted while the App is still running)
    // This function is called from C++ after the serial connection
    //  is established
    function onAlive() {
        sendUserCommandToTDA4(consts.cmd_get_state)
        // It doesn't matter if we get a response to this message or not.
        responsePending = false
    }

    Item {
        anchors.fill: parent
        transform: Rotation {
                  angle: isDevelopmentHost ? 0 : 180
                  origin.x: screenWidth/2
                  origin.y: screenHeight/2
               }
        ////////////////////////////////////////////////////////////////////
        // UI definitions

        Text {
            id: uiVersionTxt
            x: 10
            y: parent.height - 36
            text: "UI " + touchscreenVersion
            antialiasing: true
            font.family: "Helvetica"
            font.pointSize: 10
            color: "Gray"
        }

        Text {
            id: appVersionTxt
            x: parent.width/2 - width/2
            y: parent.height - 36
            text: appVersionString
            antialiasing: true
            font.family: "Helvetica"
            font.pointSize: 10
            color: "Gray"
        }

        Text {
            id: screenTxt
            x: parent.width - width - 10
            y: parent.height - 36
            text: screenName
            antialiasing: true
            font.family: "Helvetica"
            font.pointSize: 10
            color: "Gray"
        }


        OnScreenKeyboard {
            visible: true;
        }

        Loader{
            id: splashScreenLoader
            anchors.fill: parent
            visible: currentScreen ===
                    CommonDefinitions.ScreensState.Splash ? true : false
            active: visible
            source: "SplashScreen.qml"
        }

        Loader{
            id: homeScreenLoader
            anchors.fill: parent
            visible: currentScreen ===
                    CommonDefinitions.ScreensState.Home ? true : false
            active: visible
            source: "HomeScreen.qml"
        }

        Loader{
            id: placementScreenLoader
            anchors.fill: parent
            visible: currentScreen ===
                    CommonDefinitions.ScreensState.Placement ? true : false
            active: visible
            source: "PlacementScreen.qml"
        }

        Loader{
            id: scanCompleteScreenLoader
            anchors.fill: parent
            visible: currentScreen ===
                    CommonDefinitions.ScreensState.ScanComplete ? true : false
            active: visible
            source: "ScanCompleteScreen.qml"
        }

        Loader{
            id: patientDataViewLoader
            anchors.fill: parent
            visible: currentScreen ===
                    CommonDefinitions.ScreensState.PatientDataView ? true : false
            active: visible
            source: "PatientDataView.qml"

        }

        Loader{
            id: pwdScreen
            anchors.fill: parent
            visible: currentScreen ===
                    CommonDefinitions.ScreensState.PasswordScreen ? true : false
            active: visible
            source: "PasswordScreen.qml"
        }

        Loader{
            id: maintHomeScreenLoader
            anchors.fill: parent
            visible: currentScreen ===
                    CommonDefinitions.ScreensState.MaintainanceHome ? true : false
            active: visible
            source: "MaintainanceHomeScreen.qml"
        }

        Loader{
            id: laserEnergyScreenLoader
            anchors.fill: parent
            visible: currentScreen ===
                    CommonDefinitions.ScreensState.LaserEnergyScreen ? true : false
            active: visible
            source: "LaserEnergyScreen.qml"
        }

        Loader{
            id: cameraCalibrationScreenLoader
            anchors.fill: parent
            visible: currentScreen ===
                    CommonDefinitions.ScreensState.CameraCalibration ? true : false
            active: visible
            source: "CameraCalibration.qml"
        }

        Loader{
            id: infoPopupLoader
            anchors.fill: parent
            visible: infoPopupDisplay == true ? true : false
            active: visible
            source: "InfoPopup.qml"
        }

        Loader{
            id: errorPopupLoader
            anchors.fill: parent
            visible: errorPopupDisplay == true ? true : false
            active: visible
            source: "ErrorPopup.qml"
        }

        Loader{
            id: fileManagementLoader
            anchors.fill: parent
            visible: currentScreen ===
                    CommonDefinitions.ScreensState.FileManager ? true : false
            active: visible
            source: "FileManager.qml"
        }
    }
    ////////////////////////////////////////////////////////////////////
    // Message handling

    // Called upon first contac with app
    function appFirstContact() {
        console.log("App is alive")
        appIsAlive = true
        firstContact = true
        sendUserCommandToTDA4(consts.cmd_get_version)
    }

    function parseSerial(cmd) {
        // console.log("Received: " + cmd)
        var jsonObject = null
        try {

            jsonObject = JSON.parse(cmd);
        }
        catch (error) {
            console.log("Invalid JSON string. Error:", error)
            return
        }
        try {
            if (jsonObject !== null) {
                console.log("Received " + jsonObject.content + " packet")
                if (!firstContact && jsonObject.state === consts.state_home) {
                    appFirstContact()
                }
                var content = jsonObject.content
                if (content === consts.msg_set_state) {
                    handleStateMessage(jsonObject)
                } else if (content === consts.msg_response) {
                    handleResponseMessage(jsonObject)
                } else if (content === consts.msg_data) {
                    handleDataMessage(jsonObject)
                } else if (content === consts.msg_info) {
                    handleInfoMessage(jsonObject)
                } else if (content === consts.msg_error) {
                    handleErrorMessage(jsonObject)
                } else {
                    console.log("Error. Unrecognized content type: " +
                            jsonObject.content)
                }
            }
        }
        catch (error) {
            console.log(cmd)
            console.log("Error processing message:", error)
            console.log("Message:", cmd)
        }
    }

    function setState(jsonMsg) {
        // Set UI to the specified state
        if (jsonMsg.state === consts.state_startup) {
            // It shouldn't be possible to go to home state, but if the
            //  App rebooted and the UI didn't, it's conceivable
            if (appState !== consts.state_startup) {
                console.log("Changing state from " + appState + " to " 
                        + jsonMsg.state)
                currentScreen = CommonDefinitions.ScreensState.Splash
                appState = consts.state_startup
            }
        } else if (jsonMsg.state === consts.state_busy) {
            // App is busy. Keep our present state.

        } else {
            // If we're transitioning from startup state, clear any
            //  info dialogs that might be displayed as these will
            //  have startup status, and that's no longer relevant
            if (appState === consts.state_startup) {
                // Transitioning from startup to run state
                infoPopupDisplay = false
            }
            if (jsonMsg.state === consts.state_estop) {
                if (appState !== jsonMsg.state) {
                    console.log("Changing state from " + appState + " to " 
                            + jsonMsg.state)
                    currentScreen = CommonDefinitions.ScreensState.Home
                    estop = true
                    appState = jsonMsg.state
                }
            } else if (jsonMsg.state === consts.state_home) {
                if (appState !== jsonMsg.state) {
                    console.log("Changing state from " + appState + " to " 
                            + jsonMsg.state)
                    currentScreen = CommonDefinitions.ScreensState.Home
                    appState = jsonMsg.state
                }
            } else if (jsonMsg.state === consts.state_patient) {
                if (appState !== jsonMsg.state) {
                    console.log("Changing state from " + appState + " to " 
                            + jsonMsg.state)
                    currentScreen = CommonDefinitions.ScreensState.Placement
                    appState = jsonMsg.state
                }
            } else if ((jsonMsg.state === consts.state_error) ||
                    (jsonMsg.state === consts.state_shutdown)) {
                // Error state only happens as part of hard shutdown process.
                //  Treat these equivalently right now as we're going down.
                //  Protocol is to get hard error message, to display
                //  an error dialog to the user to report the error. When
                //  user clicks 'shutdown' a command is sent to the
                //  app to trigger the shutdown process. Once the app has
                //  done its shutdown work, it sends a response. Upon
                //  receipt of that response, a message is displayd saying
                //  that it's OK to power off device.
                if (appState !== jsonMsg.state) {
                    console.log("Changing state from " + appState + " to " 
                            + jsonMsg.state)
                    // Request to enter shutdown state. 
                    // An info or error dialog should be present. If
                    //  not, display a generic one to get user to 
                    //  acknowledge shutdown.
                    if (!infoPopupDisplay && !errorPopupDisplay) {
                        showInfoDialog(jsonMsg.msg, false, false)
                    }
                    // There's no shutdown screen so we can stay on
                    //  the current screen. Update app state.
                    appState = jsonMsg.state
                }
            } else {
                console.log("INTERNAL ERROR. Attempted to transition " +
                        "to unknown state: '" + jsonMsg.state + "'")
            }
        }
    }


    function handleStateMessage(jsonMsg) {
        console.log("Setting state to " + jsonMsg.state + " from " + appState)
        setState(jsonMsg)
    }


    function handleErrorMessage(jsonMsg) {
        console.log("Handling error: " + jsonMsg.msg + " :: " + jsonMsg.params)
        var hardError = (jsonMsg.state === consts.state_shutdown)
        showErrorDialog(jsonMsg.msg, hardError, jsonMsg.params)

        // Receiving an error for a request means the request was not
        //  successful. Clear the block on the UI and let the user do
        //  something else
        if ((jsonMsg.command !== "") && responsePending) {
            responsePending = false
        }
    }


    function handleInfoMessage(jsonMsg) {
        console.log("Handling info message: " + jsonMsg.msg)
        if (jsonMsg.command === consts.flash_drive_status){
            if(jsonMsg.msg === consts.flash_drive_connected){
                flashDriveConnected = true;
            }
            else {
                flashDriveConnected = false;
            }
        }
        else {
            var hardInfo = (jsonMsg.state === consts.state_shutdown)
            showInfoDialog(jsonMsg.msg, false, hardInfo, jsonMsg.params)
        }
    }


    function handleResponseMessage(jsonMsg) {
        console.log("Handling command response:" + jsonMsg.command)
        if (jsonMsg.command === consts.cmd_backup) {
            backupPending = false
            showInfoDialog(jsonMsg.msg, false, false)
            console.log("Backup complete")

        } else if (jsonMsg.command === consts.cmd_shutdown) {
            appIsAlive = false
            if (jsonMsg.msg !== "") {
                showInfoDialog(jsonMsg.msg, false, false)
            } else {
                showInfoDialog("It is OK to power off the device", false, false)
            }
            console.log("Safe to power off")

        } else if (jsonMsg.command === consts.cmd_get_state) {
            // set state done below (for all message types)
            console.log("Get state response received")

        } else if (jsonMsg.command === consts.cmd_get_version) {
            appVersionString = "App " + jsonMsg.msg
            console.log("App version is " + jsonMsg.msg)

        } else if (jsonMsg.command === consts.cmd_update) {
            showInfoDialog(jsonMsg.msg, false, false)
            console.log("Update complete")

        } else if (jsonMsg.command === consts.cmd_open_patient) {
            currentScreen = CommonDefinitions.ScreensState.Placement
            patientID = jsonMsg.msg
            console.log("Setting patient ID: " + patientID)

        } else if (jsonMsg.command === consts.cmd_scan_patient) {
            if (jsonMsg.params === "full") {
                console.log("Fullscan Completed")
                currentScreen = CommonDefinitions.ScreensState.ScanComplete
            } else if (jsonMsg.params === "test") {
                console.log("Testscan Completed")
                headsetTested = true
            } else {
                console.error("Unrecognized scan type: '" + 
                        jsonMsg.params + "'")
            }

        } else if (jsonMsg.command === consts.cmd_patient_data) {
            // Patient data has been received. 
            if (rootWin.currentScreen === 
                    CommonDefinitions.ScreensState.PatientDataView){
                patientDataViewLoader.item.setDataFinished()
                console.log("Patient data complete")
            } else {
                console.error("Received patient data on incorrect screen")
            }

        } else if (jsonMsg.command === consts.cmd_close_patient) {
            // Patient data has been received. 
            if (rootWin.currentScreen === 
                    CommonDefinitions.ScreensState.PatientDataView){
                patientDataViewLoader.item.setDataFinished()
            }

        } else if (jsonMsg.command === consts.cmd_save_note) {
            showInfoDialog("Note successfully saved", false, false)

        } else if (jsonMsg.command === consts.cmd_get_time) {
            console.log("Time Response Received Current Time: ", jsonMsg.msg)
            maintHomeScreenLoader.item.setSystemTime(jsonMsg.msg);
        } else if (jsonMsg.command === consts.cmd_get_temp) {
            console.log("Temp Response Received Current Temp: ", jsonMsg.msg)
            maintHomeScreenLoader.item.setSystemTemp(jsonMsg.msg);
        } else if (jsonMsg.command === consts.cmd_get_free_internal_storage){
            console.log("Current Internal Storage Free: ", jsonMsg.msg)
            fileManagementLoader.item.setSystemFreeStorage(jsonMsg.msg);

        } else if (jsonMsg.command === consts.cmd_get_files){
            console.log("Received files list %s" , jsonMsg.msg)
            fileManager.update(jsonMsg.msg);

        } else {
            console.log("Received response of unrecognized command: " +
                    jsonMsg.command)
        }

        // Reset state if that's appropriate. In most situations this call
        //  will be a no-op but if a state transition is called for, here's 
        //  the place to do it
        setState(jsonMsg)
        // On response, clear the response pending flag
        responsePending = false
        responsePendingCountdown = false
    }

    function assignCamModuleColor(state) {
        if (state == consts.cam_module_ready_num) {
            return consts.cam_module_ready_color;
        } else if (state == consts.cam_module_ambient_num) {
            return consts.cam_module_ambient_color;
        } else if (state == consts.cam_module_laser_num) {
            return consts.cam_module_laser_color;
        } else if (state == consts.cam_module_ambient_laser_num) {
            return consts.cam_module_ambient_laser_color;
        } 
        console.error("Assigning unknown color module status: " + state)
        return consts.cam_module_unknown_color;
    }

    function handleDataMessage(jsonMsg) {
        console.log("Handling data message from '" + jsonMsg.commanad + "'")
        if (jsonMsg.command === consts.cmd_scan_patient) {
            if (jsonMsg.params === "test") {
                console.log("Displaying headset scan results")
                if(jsonMsg.data.length === 4)
                {
                    console.log("Updating Contact Status")
                    rootWin.rFissMod = assignCamModuleColor(jsonMsg.data[0])
                    rootWin.lFissMod = assignCamModuleColor(jsonMsg.data[1])
                    rootWin.rForeMod = assignCamModuleColor(jsonMsg.data[2])
                    rootWin.lForeMod = assignCamModuleColor(jsonMsg.data[3])
                }
                console.log("Testscan Completed")
                headsetTested = true
            } else {
                console.error("Scan data w/ unexpected param: " + 
                        jsonMsg.command)
            }
        } else if (jsonMsg.command === consts.cmd_patient_data) {
            console.log("Full scan data")
            if (rootWin.currentScreen === 
                    CommonDefinitions.ScreensState.PatientDataView) {
                for (var d in jsonMsg.data){
                    if(d === "P1C4")
                    {
                        patientDataViewLoader.item.setChartData(d, jsonMsg.data.P1C4)
                    }
                    else if(d === "P2C4")
                    {
                        patientDataViewLoader.item.setChartData(d, jsonMsg.data.P2C4)
                    }
                    else if(d === "P3C4")
                    {
                        patientDataViewLoader.item.setChartData(d, jsonMsg.data.P3C4)
                    }
                    else if(d === "P4C4")
                    {
                        patientDataViewLoader.item.setChartData(d, jsonMsg.data.P4C4)
                    }
                }
            }
        } else {
            console.log("Error -- received patient data from unexpected " +
                    "command: " + jsonMsg.command)
        }
    }


    ////////////////////////////////////////////////////////////////////
    // info and error popup dialogs

    // simple error function called from C++ side
    function setError(msgStr) {
        showErrorDialog(msgStr, true, "")
    }

    function showErrorDialog(msgStr, shutdownOnly, paramStr="") {
        // If shutdown only was already set, override previous setting
        if (rootWin.popupShutdownOnly == false) {
            rootWin.popupShutdownOnly = shutdownOnly
        }
        errorPopupString = msgStr
        errorPopupStringParam = paramStr
        console.log("Error: " + msgStr + ", Param: " + paramStr)
        // Error display trumps info display
        infoPopupDisplay = false
        errorPopupDisplay = true
        if (responsePending) {
            responsePendingCountdown = consts.response_pending_timeout_sec
        }
    }

    function showInfoDialog(msgStr, showCancel, shutdownOnly, paramStr="") {
        // if shutdown only was already set, override previous setting
        if (popupShutdownOnly == false) {
            popupShutdownOnly = shutdownOnly
        }
        infoPopupString = msgStr
        infoShowCancel = showCancel
        infoPopupStringParam = (paramStr == null) ? "" : paramStr;
        console.log("Info: " + msgStr + ", Param: " + paramStr)
        infoPopupDisplay = true
        if (responsePending) {
            responsePendingCountdown = consts.response_pending_timeout_sec
        }
    }

    ////////////////////////////////////////////////////////////////////
    // Communication

    // outbound
    function sendUserCommandToTDA4(commandName, value="") {
        var objJsn = new Object()
        responsePending = true
        // set a timer to auto-recover if the app is non-responsive, but
        //  don't do this for backups as that is a potentially very long
        //  operation
        if (commandName === consts.cmd_backup) {
            backupPending = true
        } else {
            backupPending = false
        }
        if (appIsAlive) {
            responsePendingCountdown = consts.response_pending_timeout_sec
        }
        objJsn.command = commandName
        objJsn.params = value
        var jsonString= JSON.stringify(objJsn)
        console.log("Sending JSON string", jsonString)
        uart.sendMsg(jsonString)
    }

    // inbound. polled from receiving thread by a timer
    Timer {
        id: serialTimer
        repeat: true
        interval: 50
        running: true
        onTriggered: {
            if (uart.messageAvailable()) {
                var msg = uart.getNextMessage();
                if(msg && (msg.trim() !== "")){
                    parseSerial(msg);
                }
            }
        }
    }

    // in event that application fails to respond (e.g., packet lost or
    //  parse error from corrupt communication), release the UI and inform 
    //  the user that something is amiss
    Timer {
        id: overrideTimer
        repeat: true
        interval: 1000
        running: true
        onTriggered: {
            if (responsePendingCountdown > 0) {
                responsePendingCountdown--;
                if ((responsePendingCountdown === 0) &&
                        (backupPending == false)) {
                    // A response has been pending for too long. Assume
                    //  that a packet was dropped from the device. This
                    //  may be the result of a fatal error or it could
                    //  be benign. Let the user know that something may
                    //  be wrong and let them decide what to do.
                    var msg = "Internal error -- application failed to " +
                            "respond to last request.";
                    responsePending = false
                    showErrorDialog(msg, false)
                }
            }
        }
    }
}
