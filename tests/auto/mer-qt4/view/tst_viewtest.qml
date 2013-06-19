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
            verify(MyScript.waitMozContext())
            verify(MyScript.waitMozView())
            webViewport.child.url = "about:blank";
            verify(MyScript.waitLoadFinished(webViewport))
            compare(webViewport.child.loadProgress, 100)
            while (!webViewport.child.painted) {
                wait();
            }
            mozContext.dumpTS("test_Test1LoadSimpleBlank end")
        }
        function test_Test2LoadAboutMozillaCheckTitle()
        {
            mozContext.dumpTS("test_Test2LoadAboutMozillaCheckTitle start")
            webViewport.child.url = "about:mozilla";
            verify(MyScript.waitLoadFinished(webViewport))
            compare(webViewport.child.title, "The Book of Mozilla, 15:1")
            while (!webViewport.child.painted) {
                wait();
            }
            mozContext.dumpTS("test_Test2LoadAboutMozillaCheckTitle end")
        }
    }
}
