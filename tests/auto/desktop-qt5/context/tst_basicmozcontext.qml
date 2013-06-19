import QtTest 1.0
import QtQuick 2.0
import Qt5Mozilla 1.0
import "../../shared/componentCreation.js" as MyScript
import "../../shared/sharedTests.js" as SharedTests

Item {
    id: appWindow

    property bool mozViewInitialized : false
    property variant mozView : null
    property variant lastObserveMessage

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
        onRecvObserve: {
            lastObserveMessage = { msg: message, data: data }
        }
    }

    resources: TestCase {
        id: testcaseid
        name: "mozContextPage"
        when: windowShown

        function cleanup()
        {
            mozContext.dumpTS("tst_basicmozcontext cleanup")
        }
        function test_context1Init()
        {
            SharedTests.shared_context1Init()
        }
        function test_context2AcceleratedAPI()
        {
            SharedTests.shared_context2AcceleratedAPI()
        }
        function test_context3PrefAPI()
        {
            SharedTests.shared_context3PrefAPI()
        }
        function test_context4ObserveAPI()
        {
            SharedTests.shared_context4ObserveAPI()
        }
    }
}
