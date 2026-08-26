import QtQuick 2.2
import Sailfish.Silica 1.0
import ".."

Page {
    id: page

    allowedOrientations: Orientation.All

    // injected by the pushing page
    property var client
    property string sessionId
    property string curProvider: ""
    property string curModel: ""
    property bool loaded: false
    property string errorText: ""

    Component.onCompleted: {
        if (client && sessionId.length > 0)
            client.fetchModels(sessionId)
    }

    function flatten(providers) {
        flat.clear()
        for (var i = 0; i < providers.length; i++) {
            var group = providers[i]
            var models = group.models
            for (var j = 0; j < models.length; j++) {
                flat.append({
                    providerId: group.id,
                    providerName: group.name.length > 0 ? group.name : group.id,
                    modelId: models[j].id,
                    modelName: models[j].name.length > 0 ? models[j].name : models[j].id
                })
            }
        }
    }

    ListModel { id: flat }

    FishBackground {
        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: parent.bottom
            bottomMargin: Theme.itemSizeLarge
        }
    }


    Connections {
        target: page.client
        onModelsLoaded: {
            if (sessionId !== page.sessionId) return
            page.loaded = true
            page.curProvider = curProvider
            page.curModel = curModel
            page.flatten(providers)
        }
        onModelSelected: {
            if (sessionId !== page.sessionId) return
            page.curProvider = provider
            page.curModel = model
            pageStack.pop()
        }
        onRequestFailed: {
            if (method === "session.models") {
                page.loaded = true
                page.errorText = message
            }
        }
    }

    SilicaListView {
        id: listView

        anchors.fill: parent
        model: flat

        section.property: "providerName"
        section.delegate: SectionHeader { text: section }

        VerticalScrollDecorator {}

        ViewPlaceholder {
            enabled: page.loaded && flat.count === 0
            text: page.errorText.length > 0 ? page.errorText
                                            : qsTr("No models available")
            hintText: page.errorText.length > 0 ? qsTr("Go back and retry") : ""
        }

        delegate: ListItem {
            id: row

            contentHeight: Theme.itemSizeMedium + Theme.paddingSmall

            property bool isCurrent: model.modelId === page.curModel
                                     && model.providerId === page.curProvider

            Label {
                id: nameLabel

                x: Theme.horizontalPageMargin
                y: Theme.paddingSmall
                width: parent.width - 2 * Theme.horizontalPageMargin
                       - (checkIcon.visible ? checkIcon.width + Theme.paddingMedium : 0)
                text: model.modelName
                color: row.highlighted || row.isCurrent
                       ? Theme.highlightColor : Theme.primaryColor
                font.pixelSize: Theme.fontSizeMedium
                truncationMode: TruncationMode.Fade
            }

            Label {
                x: Theme.horizontalPageMargin
                anchors.bottom: parent.bottom
                anchors.bottomMargin: Theme.paddingSmall
                width: parent.width - 2 * Theme.horizontalPageMargin
                       - (checkIcon.visible ? checkIcon.width + Theme.paddingMedium : 0)
                text: model.modelId
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                truncationMode: TruncationMode.Fade
            }

            Icon {
                id: checkIcon

                visible: row.isCurrent
                anchors.right: parent.right
                anchors.rightMargin: Theme.horizontalPageMargin
                anchors.verticalCenter: parent.verticalCenter
                source: "image://theme/icon-m-accept"
            }

            onClicked: {
                if (!row.isCurrent)
                    page.client.selectModel(page.sessionId, model.providerId, model.modelId)
            }
        }
    }

    BusyIndicator {
        visible: page.client.loadingModels && flat.count === 0
        running: visible
        anchors.centerIn: listView
        size: BusyIndicatorSize.Large
    }
}
