pragma Singleton
import QtQuick

QtObject {
    property int borderWidth: 2
    property bool borderLeft: false
    property bool borderRight: false
    property bool borderTop: false
    property bool borderBottom: false
    property int radiusTopLeft: 0
    property int radiusTopRight: 0
    property int radiusBottomRight: 16
    property int radiusBottomLeft: 16
    property int barHeight: 50
    property int finalPosition: 0

    property string font: "CaskaydiaMono Nerd Font"
    property int fontSize: 16

    property bool tabSlideEnabled: true
    property int tabSlideDuration: 220
    property int tabSlideEasing: Easing.OutCubic
    property real tabSlideDistanceMultiplier: 1.0
    property int mediaArtworkSpinDurationMs: 14000
}
