import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.2
import ServerCommandManager 1.0

Rectangle {
    id: quickMenu
    // Fixed size for the menu - small and centered
    width: 500
    height: 400
    // Don't use anchors with SizeViewToRootObject - position manually
    color: "#2d2d2d"
    radius: 10
    border.color: "#444"
    border.width: 1
    visible: true  // Always visible when created
    opacity: 1.0
    
    // Menu state management
    property string currentMenu: "main"  // "main" or "server_commands"
    
    // Toast notification system
    property string toastMessage: ""
    property bool showToast: false
    
    // Animation for smooth show/hide
    Behavior on opacity {
        NumberAnimation { duration: 200 }
    }
    
    // Handle keyboard input for navigation
    Keys.onPressed: function(event) {
        if (event.key === Qt.Key_Escape) {
            closeMenu()
        } else if (event.key === Qt.Key_Up) {
            menuListView.decrementCurrentIndex()
        } else if (event.key === Qt.Key_Down) {
            menuListView.incrementCurrentIndex()
        } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
            executeCurrentItem()
        }
    }
    
    // Menu content (no longer nested in another rectangle)
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 15

        // Title
        Text {
            text: currentMenu === "main" ? qsTr("Quick Menu") : qsTr("Server Commands")
            font.pointSize: 24
            font.bold: true
            color: "#00cccc"
            Layout.alignment: Qt.AlignHCenter
        }

        // Menu items
        ListView {
            id: menuListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            focus: true
            
            model: currentMenu === "main" ? mainMenuModel : serverCommandsModel
            
            delegate: Rectangle {
                width: menuListView.width
                height: 60
                color: index === menuListView.currentIndex ? "#444" : "transparent"
                border.color: index === menuListView.currentIndex ? "#00cccc" : "transparent"
                border.width: 2
                radius: 5
                
                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    onEntered: menuListView.currentIndex = index
                    onClicked: {
                        executeAction(model.action)
                        closeMenuDelayed()
                    }
                }
                
                RowLayout {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.margins: 15
                    spacing: 15
                    
                    // Fixed-width icon container for consistent alignment
                    Rectangle {
                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 40
                        Layout.alignment: Qt.AlignVCenter
                        color: "transparent"
                        
                        Text {
                            text: model.icon
                            font.pointSize: 20
                            color: "white"
                            anchors.centerIn: parent
                        }
                    }
                    
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignVCenter
                        spacing: 2
                        
                        Text {
                            text: model.text
                            font.pointSize: 14
                            font.bold: true
                            color: "white"
                            Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                        }
                        
                        Text {
                            text: model.description
                            font.pointSize: 10
                            color: "#cccccc"
                            Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                        }
                    }
                    
                    // Status indicator (for visual feedback)
                    Rectangle {
                        Layout.preferredWidth: 8
                        Layout.preferredHeight: 8
                        Layout.alignment: Qt.AlignVCenter
                        radius: 4
                        color: {
                            if (model.action === "server_restart" || model.action === "server_shutdown" || model.action === "server_suspend") {
                                return quickMenuManager.hasServerCommands ? "#00ff00" : "#ff6666"
                            }
                            return "transparent"
                        }
                        visible: model.action === "server_restart" || model.action === "server_shutdown" || model.action === "server_suspend"
                    }
                }
            }
        }

        // Back/Close button
        Button {
            text: currentMenu === "main" ? qsTr("Close (Esc)") : qsTr("← Back")
            Layout.alignment: Qt.AlignHCenter
            onClicked: {
                if (currentMenu === "main") {
                    closeMenu()
                } else {
                    currentMenu = "main"
                }
            }
        }
    }
    
    // Toast notification overlay
    Rectangle {
        id: toastNotification
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottomMargin: 20
        width: Math.min(parent.width - 40, toastText.implicitWidth + 20)
        height: 40
        radius: 20
        color: "#333"
        border.color: "#666"
        border.width: 1
        visible: showToast
        opacity: showToast ? 1.0 : 0.0
        
        Behavior on opacity {
            NumberAnimation { duration: 200 }
        }
        
        Text {
            id: toastText
            text: toastMessage
            color: "white"
            font.pointSize: 12
            anchors.centerIn: parent
        }
    }
    
    // Timer for auto-hiding toast
    Timer {
        id: toastTimer
        interval: 2000
        onTriggered: {
            showToast = false
        }
    }
    
    // Timer for delayed close
    Timer {
        id: closeTimer
        interval: 1000
        onTriggered: {
            closeMenu()
        }
    }
    
    // Main menu model
    ListModel {
        id: mainMenuModel
        ListElement {
            text: qsTr("Disconnect")
            icon: "⏹"
            action: "disconnect"
            description: qsTr("Disconnect from server")
        }
        ListElement {
            text: qsTr("Quit")
            icon: "❌"
            action: "quit"
            description: qsTr("Quit streaming session")
        }
        ListElement {
            text: qsTr("Server Commands")
            icon: "⚙"
            action: "server_commands"
            description: qsTr("Access server control commands")
        }
        ListElement {
            text: qsTr("Clipboard Upload")
            icon: "📋"
            action: "clipboard_upload"
            description: qsTr("Upload clipboard to server")
        }
        ListElement {
            text: qsTr("Fetch Clipboard")
            icon: "📥"
            action: "clipboard_fetch"
            description: qsTr("Fetch clipboard from server")
        }
        ListElement {
            text: qsTr("Toggle Performance Stats")
            icon: "📊"
            action: "toggle_stats"
            description: qsTr("Show/hide performance statistics")
        }
        ListElement {
            text: qsTr("Toggle Mouse Capture")
            icon: "🖱"
            action: "toggle_mouse"
            description: qsTr("Toggle mouse capture mode")
        }
        ListElement {
            text: qsTr("Toggle Keyboard Capture")
            icon: "⌨"
            action: "toggle_keyboard"
            description: qsTr("Toggle keyboard capture mode")
        }
        ListElement {
            text: qsTr("Toggle Fullscreen")
            icon: "🖥"
            action: "toggle_fullscreen"
            description: qsTr("Toggle fullscreen mode")
        }
    }
    
    // Server commands menu model
    ListModel {
        id: serverCommandsModel
        ListElement {
            text: qsTr("Restart Server")
            icon: "🔄"
            action: "server_restart"
            description: qsTr("Restart the streaming server")
        }
        ListElement {
            text: qsTr("Shutdown Server")
            icon: "⏹"
            action: "server_shutdown"
            description: qsTr("Shutdown the streaming server")
        }
        ListElement {
            text: qsTr("Suspend Server")
            icon: "⏸"
            action: "server_suspend"
            description: qsTr("Suspend the streaming server")
        }
    }
    
    // Functions
function closeMenu() {
        // Only call backend hide - don't set QML invisible
            if (typeof quickMenuManager !== 'undefined') {
                showActionFeedback("Closing menu...")  // Feedback when closing
            quickMenuManager.hide();
        }
    }
    
    function closeMenuDelayed() {
        // Don't close immediately for navigation actions
        if (currentMenu === "server_commands") {
            return
        }
        
        // Close after a short delay to allow toast to be visible
        closeTimer.start()
    }
    
    function executeCurrentItem() {
        var currentItem = menuListView.model.get(menuListView.currentIndex)
        if (currentItem) {
            executeAction(currentItem.action)
        }
    }
    
    function executeAction(action) {
        console.log("Executing action:", action)
        
        // Handle navigation actions
        if (action === "server_commands") {
            currentMenu = "server_commands"
            return
        }
        
        // Show action feedback
        showActionFeedback(action)
        
        // Call the backend manager to execute the action
        if (typeof quickMenuManager !== 'undefined') {
            quickMenuManager.executeAction(action)
        }
        
        // Close menu after action (except for navigation)
        closeMenu()
    }
    
    function showActionFeedback(action) {
        var message = ""
        switch(action) {
            case "disconnect":
                message = "Disconnecting from server..."
                break
            case "quit":
                message = "Quitting session..."
                break
            case "server_restart":
                message = quickMenuManager.hasServerCommands ? "Restarting server..." : "Server commands not available"
                break
            case "server_shutdown":
                message = quickMenuManager.hasServerCommands ? "Shutting down server..." : "Server commands not available"
                break
            case "server_suspend":
                message = quickMenuManager.hasServerCommands ? "Suspending server..." : "Server commands not available"
                break
            case "clipboard_upload":
                message = "Uploading clipboard to server..."
                break
            case "clipboard_fetch":
                message = "Fetching clipboard from server..."
                break
            case "toggle_stats":
                message = "Toggling performance stats..."
                break
            case "toggle_mouse":
                message = "Toggling mouse capture..."
                break
            case "toggle_keyboard":
                message = "Toggling keyboard capture..."
                break
            case "toggle_fullscreen":
                message = "Toggling fullscreen..."
                break
            default:
                message = "Executing action..."
        }
        
        if (message) {
            toastMessage = message
            showToast = true
            toastTimer.restart()
        }
    }
    
    function startToastTimer() {
        toastTimer.restart()
    }
    
    // Make menu focusable and reset state
    Component.onCompleted: {
        focus = true
        currentMenu = "main"  // Always start with main menu
    }
}
