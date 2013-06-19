import QtQuick 2.0
import Qt5Mozilla 1.0

QmlMozView {
    id: webViewport
    parentid: appWindow.createParentID ? appWindow.createParentID : 0
    visible: true
    focus: true
    anchors.fill: parent
    Connections {
        target: webViewport.child
    }
}
