pragma Singleton
import QtQuick

QtObject {
    /* ---- styling ---- */
    property int borderWidth: 2
    property bool borderLeft: false
    property bool borderRight: false
    property bool borderTop: false
    property bool borderBottom: false
    property int radius: 0
    property int barHeight: 28
    property int finalPosition: 0

    property string font: "JetBrainsMono Nerd Font"
    property int fontSize: 16
}
