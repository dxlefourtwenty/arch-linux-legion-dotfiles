pragma Singleton
import QtQuick

QtObject {
    /* ---- styling ---- */
    property int borderWidth: 2
    property bool borderLeft: true
    property bool borderRight: true
    property bool borderTop: true
    property bool borderBottom: true
    property int radius: 0
    property int barHeight: 50

    property string font: "JetBrainsMono Nerd Font"
    property int fontSize: 16
}
