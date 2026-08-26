import QtQuick 2.2
import Sailfish.Silica 1.0
import Dsh 1.0
import ".."

Page {
    id: page

    allowedOrientations: Orientation.All

    property var allSessions: []
    property string searchText: ""

    function refreshState() {
        if (svc.busy)
            return
        svc.refresh()
    }

    function refreshSessions() {
        client.listSessions()
    }

    function applyFilter() {
        var needle = page.searchText.trim().toLowerCase()
        var filtered = []
        for (var i = 0; i < allSessions.length; i++) {
            var item = allSessions[i]
            if (needle.length > 0
                    && item.title.toLowerCase().indexOf(needle) < 0
                    && item.sessionId.toLowerCase().indexOf(needle) < 0)
                continue
            filtered.push(item)
        }
        sessionModel.replaceAll(filtered)
    }

    onStatusChanged: {
        if (status === PageStatus.Activating) {
            refreshSessions()
            refreshState()
        }
    }

    DshClient { id: client }
    SessionModel { id: sessionModel }

    SystemdControl {
        id: svc
    }

    Connections {
        target: client
        onSessionListReady: {
            page.allSessions = items
            page.applyFilter()
        }
    }

    Timer {
        interval: 5000
        running: Qt.application.active && page.status === PageStatus.Active
        repeat: true
        triggeredOnStart: true
        onTriggered: page.refreshState()
    }

    FishBackground {
        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: parent.bottom
            bottomMargin: Theme.itemSizeLarge
        }
    }

    SilicaListView {
        id: listView

        anchors.fill: parent
        model: sessionModel

        VerticalScrollDecorator {}

        PullDownMenu {
            MenuItem {
                text: qsTr("Open Web UI")
                onClicked: Qt.openUrlExternally("http://127.0.0.1:3080")
            }
            MenuItem {
                text: qsTr("Refresh")
                onClicked: {
                    page.refreshSessions()
                    page.refreshState()
                }
            }
            MenuItem {
                text: qsTr("New chat")
                onClicked: pageStack.push(Qt.resolvedUrl("ChatPage.qml"))
            }
        }

        ViewPlaceholder {
            enabled: listView.count === 0
            text: qsTr("No chat history yet")
            hintText: qsTr("Pull down to start a new chat")
        }

        header: Column {
            width: listView.width
            spacing: Theme.paddingLarge

            PageHeader {
                title: "DeepSeek Harness"
            }

            SearchField {
                id: searchField

                width: parent.width
                placeholderText: qsTr("Search chat history")
                onTextChanged: {
                    page.searchText = text
                    page.applyFilter()
                }
            }

            DetailItem {
                label: qsTr("Service status")
                value: svc.state
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: svc.state === "active"
                      ? qsTr("Stop service")
                      : qsTr("Start service")
                enabled: !svc.busy && svc.state !== "unknown"
                onClicked: svc.state === "active" ? svc.stop() : svc.start()
            }

            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                visible: svc.busy || client.loadingList
                text: qsTr("Working…")
                color: Theme.secondaryColor
            }

            SectionHeader {
                text: qsTr("Chat history")
            }
        }

        delegate: ListItem {
            id: entry

            contentHeight: Theme.itemSizeMedium

            Label {
                id: titleLabel

                x: Theme.horizontalPageMargin
                y: Theme.paddingSmall
                width: parent.width - 2 * Theme.horizontalPageMargin - runningLabel.width
                       - (runningLabel.visible ? Theme.paddingSmall : 0)
                text: model.title
                color: entry.highlighted ? Theme.highlightColor : Theme.primaryColor
                font.pixelSize: Theme.fontSizeMedium
                truncationMode: TruncationMode.Fade
            }

            Label {
                id: runningLabel

                visible: model.running
                anchors.right: parent.right
                anchors.rightMargin: Theme.horizontalPageMargin
                y: Theme.paddingSmall
                text: qsTr("running")
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeExtraSmall
            }

            Label {
                x: Theme.horizontalPageMargin
                anchors.bottom: parent.bottom
                anchors.bottomMargin: Theme.paddingSmall
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: Qt.formatDateTime(new Date(model.updatedAt), "yyyy-MM-dd hh:mm")
                      + (model.cwd.length > 0 ? "  ·  " + model.cwd.replace(/^.*\//, "/") : "")
                color: entry.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                truncationMode: TruncationMode.Fade
            }

            onClicked: {
                pageStack.push(Qt.resolvedUrl("ChatPage.qml"),
                               { sessionId: model.sessionId })
            }
        }
    }
}
