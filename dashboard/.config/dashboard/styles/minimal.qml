pragma Singleton
import QtQuick

QtObject {
    /* ---- styling ---- */
    property int borderWidth: 2
    property bool borderLeft: false
    property bool borderRight: false
    property bool borderTop: false
    property bool borderBottom: false
    property int radiusTopLeft: 0
    property int radiusTopRight: 0
    property int radiusBottomRight: 0
    property int radiusBottomLeft: 0
    property int barHeight: 30
    property int finalPosition: 0

    property string font: "CaskaydiaMono Nerd Font"
    property int fontSize: 16

    property bool tabSlideEnabled: true
    property int tabSlideDuration: 220
    property int tabSlideEasing: Easing.OutCubic
    property real tabSlideDistanceMultiplier: 1.0
}
