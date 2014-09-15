import QtTest 1.0
import QtQuick 2.0
import Qt5Mozilla 1.0
import "../../shared/componentCreation.js" as MyScript
import "../../shared/sharedTests.js" as SharedTests

Item {
    id: appWindow
    width: 480
    height: 800
    focus: true

    property bool mozViewInitialized : false
    property int scrollX : 0
    property int scrollY : 0
    property int clickX : 0
    property int clickY : 0

    QmlMozContext {
        id: mozContext
    }
    Connections {
        target: mozContext.instance
        onOnInitialized: {
            // Gecko does not switch to SW mode if gl context failed to init
            // and qmlmoztestrunner does not build in GL mode
            // Let's put it here for now in SW mode always
            mozContext.instance.setIsAccelerated(true);
            mozContext.instance.addComponentManifest(mozContext.getenv("QTTESTSROOT") + "/components/TestHelpers.manifest");
        }
    }

    QmlMozView {
        id: webViewport
        visible: true
        focus: true
        active: true
        anchors.fill: parent

        Connections {
            target: webViewport.child
            onViewInitialized: {
                appWindow.mozViewInitialized = true
            }
            onHandleSingleTap: {
                appWindow.clickX = point.x
                appWindow.clickY = point.y
            }
            onViewAreaChanged: {
                print("onViewAreaChanged: ", webViewport.child.scrollableOffset.x, webViewport.child.scrollableOffset.y);
                var offset = webViewport.child.scrollableOffset
                appWindow.scrollX = offset.x
                appWindow.scrollY = offset.y
            }
        }
    }

    resources: TestCase {
        id: testcaseid
        name: "mozContextPage"
        when: windowShown
        parent: appWindow

        function cleanup() {
            mozContext.dumpTS("tst_scrolltest cleanup")
        }

        function test_TestScrollPaintOperations()
        {
            SharedTests.shared_TestScrollPaintOperations()
        }
    }
}
