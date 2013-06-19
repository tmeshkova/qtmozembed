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
    }

    resources: TestCase {
        id: testcaseid
        name: "mozContextPage"
        when: windowShown

        function cleanup() {
            mozContext.dumpTS("tst_basicview cleanup")
        }

        function test_1contextPrepareViewContext()
        {
            mozContext.dumpTS("test_1contextPrepareViewContext start")
            testcaseid.verify(mozContext.instance !== undefined)
            while (mozContext.instance.initialized() === false) {
                testcaseid.wait(500)
            }
            testcaseid.verify(mozContext.instance.initialized())
            mozContext.dumpTS("test_1contextPrepareViewContext end")
        }
        function test_2viewInit()
        {
            mozContext.dumpTS("test_2viewInit start")
            testcaseid.verify(mozContext.instance.initialized())
            appWindow.createParentID = 0;
            MyScript.createSpriteObjects();
            while (mozView === null) {
                testcaseid.wait(500)
            }
            while (mozViewInitialized !== true) {
                testcaseid.wait(500)
            }
            testcaseid.verify(mozView.child !== undefined)
            mozContext.dumpTS("test_2viewInit end")
        }
        function test_3viewLoadURL()
        {
            mozContext.dumpTS("test_3viewLoadURL start")
            testcaseid.verify(mozView.child !== undefined)
            mozView.child.url = "about:mozilla";
            testcaseid.verify(MyScript.waitLoadFinished(mozView))
            testcaseid.compare(mozView.child.url, "about:mozilla")
            while (!mozView.child.painted) {
                testcaseid.wait();
            }
            mozContext.dumpTS("test_3viewLoadURL end")
        }
    }
}
