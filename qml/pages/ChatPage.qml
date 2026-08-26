import QtQuick 2.2
import Sailfish.Silica 1.0
import Dsh 1.0
import ".."

Page {
    id: page

    allowedOrientations: Orientation.All

    property string sessionId: ""
    property bool awaitingReply: false
    property string errorText: ""
    // first message typed before the session exists; sent once created
    property string pendingText: ""
    property bool hasMoreHistory: false
    property int oldestSeq: -1
    property string curProvider: ""
    property string curModel: ""

    function send() {
        var text = inputField.text.trim()
        if (text.length === 0) return
        chatModel.addUserMessage(text)
        inputField.text = ""
        awaitingReply = true
        scrollToBottom()
        if (sessionId.length === 0) {
            pendingText = text
            client.createSession("")
            return
        }
        client.prompt(sessionId, text)
    }

    function scrollToBottom() {
        if (listView.count > 0)
            listView.positionViewAtEnd()
    }

    function reportError(method, message) {
        awaitingReply = false
        errorBannerTimer.restart()
        errorText = method + ": " + message
    }

    ChatModel { id: chatModel }
    DshClient { id: client }

    Component.onCompleted: {
        client.open()
        if (sessionId.length > 0) {
            client.fetchHistory(sessionId)
            client.fetchModels(sessionId)
        }
    }

    Connections {
        target: client
        onSessionCreated: {
            page.sessionId = sessionId
            client.fetchModels(sessionId)
            if (page.pendingText.length > 0) {
                client.prompt(sessionId, page.pendingText)
                page.pendingText = ""
            }
            page.scrollToBottom()
        }
        onHistoryLoaded: {
            if (sessionId !== page.sessionId) return
            page.hasMoreHistory = hasMore
            page.oldestSeq = oldestSeq
            chatModel.loadMessages(messages)
            page.scrollToBottom()
        }
        onOlderPageLoaded: {
            if (sessionId !== page.sessionId) return
            page.hasMoreHistory = hasMore
            var topIndex = listView.indexAt(listView.width / 2,
                                            listView.contentY + Theme.paddingLarge)
            var added = messages.length
            chatModel.prependMessages(messages)
            if (topIndex >= 0)
                listView.positionViewAtIndex(Math.min(topIndex + added, chatModel.count - 1),
                                             ListView.Beginning)
        }
        onTextDelta: {
            if (sessionId !== page.sessionId) return
            chatModel.appendAssistantDelta(delta)
            page.awaitingReply = false
            page.scrollToBottom()
        }
        onTurnEnded: {
            if (sessionId !== page.sessionId) return
            chatModel.endAssistantTurn()
            page.awaitingReply = false
        }
        onModelsLoaded: {
            if (sessionId !== page.sessionId) return
            page.curProvider = curProvider
            page.curModel = curModel
        }
        onModelSelected: {
            if (sessionId !== page.sessionId) return
            page.curProvider = provider
            page.curModel = model
        }
        onRequestFailed: {
            page.reportError(method, message)
        }
        onTurnFailed: {
            if (sessionId !== page.sessionId) return
            page.reportError("turn", message)
        }
    }

    Timer {
        id: errorBannerTimer
        interval: 5000
        onTriggered: page.errorText = ""
    }

    FishBackground {
        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: composer.top
        }
    }

    SilicaListView {
        id: listView

        anchors.fill: parent
        anchors.bottomMargin: composer.height
        model: chatModel

        PullDownMenu {
            MenuItem {
                visible: page.sessionId.length > 0
                text: page.curModel.length > 0
                      ? qsTr("Model") + ": " + page.curModel
                      : qsTr("Switch model")
                onClicked: pageStack.push(Qt.resolvedUrl("ModelPickerPage.qml"), {
                    client: client,
                    sessionId: page.sessionId,
                    curProvider: page.curProvider,
                    curModel: page.curModel
                })
            }
            MenuItem {
                visible: page.sessionId.length > 0 && page.hasMoreHistory
                text: qsTr("Load earlier messages")
                onClicked: client.fetchHistoryBefore(page.sessionId, page.oldestSeq)
            }
        }

        ViewPlaceholder {
            enabled: listView.count === 0 && !client.loadingHistory
            text: qsTr("DeepSeek Harness")
            hintText: qsTr("Send a message to start")
        }

        delegate: Item {
            width: listView.width
            height: bubble.height + Theme.paddingMedium

            Rectangle {
                id: bubble

                property real maxWidth: listView.width - 2 * Theme.horizontalPageMargin
                property bool collapsed: model.text.length > 600 || model.text.split("\n").length > 12
                property bool expanded: false

                x: model.isUser ? parent.width - width - Theme.horizontalPageMargin
                                : Theme.horizontalPageMargin
                width: Math.min(messageText.width + 2 * Theme.paddingLarge,
                                maxWidth)
                height: messageText.height + 2 * Theme.paddingSmall
                color: model.isUser ? Theme.rgba(Theme.highlightColor, 0.25)
                                    : Theme.rgba(Theme.secondaryColor, 0.15)
                radius: Theme.paddingSmall

                Label {
                    id: messageText

                    property real maxWidth: bubble.maxWidth - 2 * Theme.paddingLarge

                    x: Theme.paddingLarge
                    y: Theme.paddingSmall
                    width: Math.min(implicitWidth, maxWidth)
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.primaryColor
                    textFormat: Text.PlainText
                    maximumLineCount: bubble.collapsed && !bubble.expanded ? 12 : 0
                    elide: Text.ElideRight
                    text: model.text + (model.streaming ? " ▌" : "")
                }

                MouseArea {
                    anchors.fill: parent
                    enabled: bubble.collapsed
                    onClicked: bubble.expanded = !bubble.expanded
                }
            }
        }

        VerticalScrollDecorator {}
    }

    BusyIndicator {
        visible: client.loadingHistory && chatModel.count === 0
        running: visible
        anchors.centerIn: listView
        size: BusyIndicatorSize.Large
    }

    DockedPanel {
        id: composer

        open: true
        width: parent.width
        height: composerRow.height + 2 * Theme.paddingSmall

        Row {
            id: composerRow

            anchors.centerIn: parent
            width: parent.width - 2 * Theme.horizontalPageMargin
            spacing: Theme.paddingMedium

            TextArea {
                id: inputField

                width: parent.width - sendButton.width - parent.spacing
                placeholderText: qsTr("Message")
                font.pixelSize: Theme.fontSizeSmall
                labelVisible: false
                EnterKey.enabled: text.trim().length > 0 && !page.awaitingReply
                EnterKey.onClicked: page.send()
            }

            Button {
                id: sendButton

                anchors.bottom: parent.bottom
                width: Theme.itemSizeExtraLarge * 1.5
                text: page.awaitingReply ? qsTr("Stop") : qsTr("Send")
                enabled: page.awaitingReply || inputField.text.trim().length > 0
                onClicked: {
                    if (page.awaitingReply) {
                        if (page.sessionId.length > 0)
                            client.cancelSession(page.sessionId)
                        page.awaitingReply = false
                    } else {
                        page.send()
                    }
                }
            }
        }
    }

    Label {
        id: errorBanner

        z: 10
        visible: page.errorText.length > 0
        anchors {
            left: parent.left
            right: parent.right
            bottom: composer.top
            margins: Theme.horizontalPageMargin
            bottomMargin: Theme.paddingLarge
        }
        wrapMode: Text.Wrap
        font.pixelSize: Theme.fontSizeExtraSmall
        color: Theme.errorColor
        text: page.errorText
    }
}
