import QtQuick

GlassPanel {
    id: drawer
    property bool open: false
    property bool rightSide: false
    property real drawerWidth: 340
    signal visibleWidthChangedByAnimation(real width)

    width: drawerWidth
    height: parent ? parent.height : 0
    x: rightSide
       ? (parent ? parent.width : 0) - (open ? width : 0)
       : (open ? 0 : -width)
    radius: 0

    Behavior on x {
        NumberAnimation {
            duration: 180
            easing.type: Easing.OutCubic
            onRunningChanged: drawer.visibleWidthChangedByAnimation(
                                  drawer.open ? drawer.width : 0)
        }
    }
}
