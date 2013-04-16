import QtQuickTest 1.0
import QtQuick 1.0
import Sailfish.Silica 1.0
import QtMozilla 1.0
import "../componentCreation.js" as MyScript

ApplicationWindow {
    id: appWindow

    property string currentPageName: pageStack.currentPage != null
            ? pageStack.currentPage.objectName
            : ""

    property bool mozViewInitialized : false
    property bool viewRendered : false
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
            mozContext.instance.setIsAccelerated(false);
            mozContext.instance.addComponentManifest("/opt/tests/qtmozembed/components/TestHelpers.manifest");
        }
    }

    QmlMozView {
        id: webViewport
        visible: true
        focus: true
        anchors.fill: parent
        Connections {
            target: webViewport.child
            onViewInitialized: {
                appWindow.mozViewInitialized = true
            }
            onFirstPaint: {
                viewRendered = true;
            }
            onHandleSingleTap: {
                appWindow.clickX = point.x
                appWindow.clickY = point.y
            }
            onViewAreaChanged: {
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

        function cleanup() {
            mozContext.dumpTS("cleanup")
        }

        function test_TestScrollPaintOperations()
        {
            mozContext.dumpTS("start")
            verify(MyScript.waitMozContext())
            verify(MyScript.waitMozView())
            webViewport.child.url = "data:text/html,<body bgcolor=red leftmargin=0 topmargin=0 marginwidth=0 marginheight=0><input style='position:absolute; left:0px; top:1200px;'>";
            verify(MyScript.waitLoadStarted(webViewport))
            verify(MyScript.waitLoadFinished(webViewport))
            compare(webViewport.child.loadProgress, 100);
            while (!appWindow.viewRendered) {
                wait();
            }
            MyScript.scrollBy(1, 401, 0, -400, 100, false);
            while (appWindow.scrollY === 0) {
                wait();
            }
            verify(appWindow.scrollX === 0)
            while (appWindow.clickX === 0) {
                wait();
            }
            verify(appWindow.clickX === 1)
            verify(appWindow.clickY === 1)
            appWindow.clickX = 0
            mouseClick(webViewport, 10, 20)
            while (appWindow.clickX === 0) {
                wait();
            }
            verify(appWindow.clickX === 10)
            verify(appWindow.clickY === 20)
            mozContext.dumpTS("end");
        }
    }
}
