import QtQuickTest 1.0
import QtQuick 1.0
import Sailfish.Silica 1.0
import QtMozilla 1.0
import "../../shared/componentCreation.js" as MyScript
import "../../shared/sharedTests.js" as SharedTests

ApplicationWindow {
    id: appWindow

    property bool mozViewInitialized : false

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
        }
    }

    resources: TestCase {
        id: testcaseid
        name: "mozContextPage"
        when: windowShown

        function cleanup() {
            mozContext.dumpTS("tst_viewtest cleanup")
        }

        function test_Test1LoadSimpleBlank()
        {
            mozContext.dumpTS("test_Test1LoadSimpleBlank start")
            testcaseid.verify(MyScript.waitMozContext())
            testcaseid.verify(MyScript.waitMozView())
            webViewport.child.url = "about:blank";
            testcaseid.verify(MyScript.waitLoadFinished(webViewport))
            testcaseid.compare(webViewport.child.loadProgress, 100)
            while (!webViewport.child.painted) {
                testcaseid.wait();
            }
            mozContext.dumpTS("test_Test1LoadSimpleBlank end")
        }
        function test_Test2LoadAboutMozillaCheckTitle()
        {
            mozContext.dumpTS("test_Test2LoadAboutMozillaCheckTitle start")
            webViewport.child.url = "about:mozilla";
            testcaseid.verify(MyScript.waitLoadFinished(webViewport))
            testcaseid.compare(webViewport.child.title, "The Book of Mozilla, 15:1")
            while (!webViewport.child.painted) {
                testcaseid.wait();
            }
            mozContext.dumpTS("test_Test2LoadAboutMozillaCheckTitle end")
        }
    }
}
