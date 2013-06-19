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

        function cleanup() {
            mozContext.dumpTS("tst_basicmozcontext cleanup")
        }

        function test_context1Init()
        {
            mozContext.dumpTS("test_context1Init start")
            verify(mozContext.instance !== undefined)
            while (mozContext.instance.initialized() === false) {
                wait(500)
            }
            verify(mozContext.instance.initialized())
            mozContext.dumpTS("test_context1Init end")
        }
        function test_context2AcceleratedAPI()
        {
            mozContext.dumpTS("test_context2AcceleratedAPI start")
            mozContext.instance.setIsAccelerated(true);
            verify(mozContext.instance.isAccelerated() === true)
            mozContext.dumpTS("test_context2AcceleratedAPI end")
        }
        function test_context3PrefAPI()
        {
            mozContext.dumpTS("test_context3PrefAPI start")
            mozContext.instance.setPref("test.embedlite.pref", "result");
            mozContext.dumpTS("test_context3PrefAPI end")
        }
        function test_context4ObserveAPI()
        {
            mozContext.dumpTS("test_context4ObserveAPI start")
            mozContext.instance.sendObserve("memory-pressure", null);
            mozContext.instance.addObserver("test-observe-message");
            mozContext.instance.sendObserve("test-observe-message", {msg: "testMessage", val: 1});
            while (lastObserveMessage === undefined) {
                mozContext.waitLoop()
            }
            compare(lastObserveMessage.msg, "test-observe-message");
            compare(lastObserveMessage.data.val, 1);
            compare(lastObserveMessage.data.msg, "testMessage");
            mozContext.dumpTS("test_context4ObserveAPI end")
        }
    }
}
