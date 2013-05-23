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
    property variant mozView : null
    property variant oldMozView : null
    property variant createParentID : 0

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
        }
        onNewWindowRequested: {
            print("New Window Requested: url: ", url, ", parentID:", parentId);
            appWindow.oldMozView = appWindow.mozView;
            appWindow.mozView = null;
            appWindow.createParentID = parentId;
            MyScript.createSpriteObjects();
            while (appWindow.mozView === null) {
                testcaseid.wait()
            }
            testcaseid.verify(mozView.uniqueID() > 0)
            newWinResponse.windowID = mozView.uniqueID();
        }
    }

    resources: TestCase {
        id: testcaseid
        name: "mozContextPage"
        when: windowShown

        function cleanup() {
            mozContext.dumpTS("tst_newviewrequest cleanup")
        }

        function test_1contextPrepareViewContext()
        {
            mozContext.dumpTS("test_1contextPrepareViewContext start")
            verify(mozContext.instance !== undefined)
            while (mozContext.instance.initialized() === false) {
                wait(500)
            }
            verify(mozContext.instance.initialized())
            mozContext.dumpTS("test_1contextPrepareViewContext end")
        }
        function test_2viewInit()
        {
            mozContext.dumpTS("test_2viewInit start")
            verify(mozContext.instance.initialized())
            MyScript.createSpriteObjects();
            while (mozView === null) {
                wait(500)
            }
            while (mozViewInitialized !== true) {
                wait(500)
            }
            verify(mozView.child !== undefined)
            mozContext.dumpTS("test_2viewInit end")
        }
        function test_viewTestNewWindowAPI()
        {
            mozContext.dumpTS("test_viewTestNewWindowAPI start")
            verify(mozView.child !== undefined)
            mozView.child.url = mozContext.getenv("QTTESTPATH") + "/auto/newviewrequest/newwin.html";
            verify(MyScript.waitLoadFinished(mozView))
            compare(mozView.child.title, "NewWinExample")
            while (!mozView.child.painted) {
                wait();
            }
            mozViewInitialized = false;
            mouseClick(mozView, 10, 10)
            while (!mozView || !oldMozView) {
                wait()
            }
            while (mozViewInitialized !== true) {
                wait()
            }
            verify(mozView.child !== undefined)
            verify(MyScript.waitLoadFinished(mozView))
            while (!mozView.child.painted) {
                wait();
            }
            compare(mozView.child.url, "about:mozilla")
            mozContext.dumpTS("test_viewTestNewWindowAPI end")
        }
    }
}
