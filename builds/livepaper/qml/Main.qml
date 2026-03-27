import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.layershell 1.0 as LS

Window {
    id: win
    visible: false
    color: "transparent"
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint

    readonly property int thumbSize: 148
    readonly property int itemSpacing: 18
    readonly property int sidePadding: 24
    readonly property int labelHeight: 28
    readonly property int panelBaseHeight: thumbSize + labelHeight + 36
    readonly property int panelBottomExtension: 12
    readonly property int panelHeight: panelBaseHeight + panelBottomExtension
    readonly property int panelWidth: thumbSize * 3 + itemSpacing * 2 + sidePadding * 2

    property int realCount: Livepaper.count
    property bool closing: false
    property bool hadFocus: false
    property int animMs: 180
    readonly property int galleryRepeats: realCount > 1 ? 5 : realCount

    function startClose() {
        if (closing) return
        closing = true
        panel.y = panel.hiddenY
        closeTimer.restart()
    }

    width: Screen.width
    height: panelHeight + 40

    LS.Window.layer: LS.Window.LayerOverlay
    LS.Window.anchors: LS.Window.AnchorLeft | LS.Window.AnchorRight | LS.Window.AnchorBottom
    LS.Window.exclusionZone: -1
    LS.Window.keyboardInteractivity: LS.Window.KeyboardInteractivityOnDemand
    onActiveChanged: {
        if (active) hadFocus = true
        else if (hadFocus && !closing) startClose()
    }
    onClosing: (close) => {
        if (closing) return
        close.accepted = false
        startClose()
    }

    Timer {
        id: closeTimer
        interval: win.animMs
        repeat: false
        onTriggered: Livepaper.exitApp()
    }

    Connections {
        target: Livepaper

        function onCountChanged() {
            win.realCount = Livepaper.count
        }

        function onCloseRequested() {
            win.startClose()
        }
    }

    Rectangle {
        id: panel
        width: Math.min(panelWidth, win.width - 30)
        height: panelHeight
        anchors.horizontalCenter: parent.horizontalCenter
        readonly property real shownY: win.height - win.panelBaseHeight - 12
        readonly property real hiddenY: win.height
        y: hiddenY
        radius: 0
        color: Livepaper.overlay
        border.width: 0
        Behavior on y {
            NumberAnimation { duration: win.animMs; easing.type: Easing.OutCubic }
        }

        Rectangle {
            x: 0
            y: 0
            width: parent.width
            height: 2
            color: Livepaper.border
        }

        Rectangle {
            x: 0
            y: 0
            width: 2
            height: parent.height
            color: Livepaper.border
        }

        Rectangle {
            x: parent.width - 2
            y: 0
            width: 2
            height: parent.height
            color: Livepaper.border
        }

        Keys.onPressed: (event) => {
            if (event.key === Qt.Key_Left || event.key === Qt.Key_H) {
                Livepaper.moveLeft()
                event.accepted = true
            } else if (event.key === Qt.Key_Right || event.key === Qt.Key_L) {
                Livepaper.moveRight()
                event.accepted = true
            } else {
                win.startClose()
                event.accepted = true
            }
        }

        Component.onCompleted: {
            forceActiveFocus()
            y = shownY
        }

        ListView {
            id: listView
            anchors.fill: parent
            anchors.margins: 16
            model: galleryRepeats > 0 ? realCount * galleryRepeats : 0
            orientation: ListView.Horizontal
            spacing: itemSpacing
            clip: true
            interactive: false
            contentX: {
                if (realCount <= 0 || width <= 0) return 0
                const step = thumbSize + itemSpacing
                const centerX = Livepaper.galleryIndex * step + (thumbSize * 0.5)
                return centerX - width * 0.5
            }

            delegate: Item {
                width: thumbSize
                height: thumbSize + labelHeight + 8

                readonly property int mappedIndex: index % realCount
                readonly property bool selected: mappedIndex === Livepaper.currentIndex

                Rectangle {
                    width: thumbSize
                    height: thumbSize
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                    radius: 0
                    color: selected ? Livepaper.hover : Livepaper.idle
                    border.width: selected ? 3 : 1
                    border.color: selected ? Livepaper.border : Qt.rgba(Livepaper.border.r, Livepaper.border.g, Livepaper.border.b, 0.35)

                    Image {
                        anchors.fill: parent
                        anchors.margins: 4
                        source: "file://" + Livepaper.thumbPathAt(mappedIndex)
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true
                        cache: true
                        smooth: true
                    }
                }

                Text {
                    anchors.top: parent.top
                    anchors.topMargin: thumbSize + 6
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight
                    color: Livepaper.text
                    font.pixelSize: 15
                    text: Livepaper.labelAt(mappedIndex)
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: Livepaper.setCurrentIndex(mappedIndex)
                }
            }
        }
    }
}
