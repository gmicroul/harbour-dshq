import QtQuick 2.2
import Sailfish.Silica 1.0
import "pages"

ApplicationWindow {
    initialPage: Component { ServicePage {} }
    cover: Qt.resolvedUrl("cover/CoverPage.qml")
}
