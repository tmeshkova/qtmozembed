import QtQuickTest 1.0
import QtQuick 1.0
import Sailfish.Silica 1.0
import QtMozilla 1.0
import "../../shared/componentCreation.js" as MyScript
import "../../shared/sharedTests.js" as SharedTests

ApplicationWindow {
    id: appWindow

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

        function test_1newcontextPrepareViewContext()
        {
            SharedTests.shared_1newcontextPrepareViewContext()
        }
        function test_2newviewInit()
        {
            SharedTests.shared_2newviewInit()
        }
        function test_viewTestNewWindowAPI()
        {
            SharedTests.shared_viewTestNewWindowAPI()
        }
    }
}
