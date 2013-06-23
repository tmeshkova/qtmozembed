import QtTest 1.0
import QtQuick 2.0
import Qt5Mozilla 1.0
import "../../shared/componentCreation.js" as MyScript
import "../../shared/sharedTests.js" as SharedTests

Item {
    id: appWindow
    width: 480
    height: 800

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
        parent: appWindow

        function cleanup() {
            mozContext.dumpTS("tst_basicview cleanup")
        }

        function test_1contextPrepareViewContext()
        {
            SharedTests.shared_1contextPrepareViewContext()
        }
        function test_2viewInit()
        {
            SharedTests.shared_2viewInit(true)
        }
        function test_3viewLoadURL()
        {
            SharedTests.shared_3viewLoadURL()
        }
    }
}
