import QtQuick 2.2
import Sailfish.Silica 1.0

// Wallpaper-style mascot background: "正面" sprite of 大飞鱼
// (from https://github.com/1190fasheqi/dafeiyu-pet, sprites/正面.png).
// Place as the first child of a Page / CoverBackground so it renders
// behind the content; anchors are set by the using page.
Image {
    source: Qt.resolvedUrl("images/zhengmian.png")
    visible: status === Image.Ready

    opacity: 0.22
    fillMode: Image.PreserveAspectFit
    smooth: true

    width: Math.round(Math.min(parent.width * 0.78,
                               parent.height * 0.6 * sourceSize.width
                               / Math.max(1, sourceSize.height)))
    height: Math.round(width * sourceSize.height / Math.max(1, sourceSize.width))
}
