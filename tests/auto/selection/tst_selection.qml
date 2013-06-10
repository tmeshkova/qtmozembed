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
    property string selectedContent : ""

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
            mozContext.instance.addObserver("clipboard:setdata");
        }
        onRecvObserve: {
            print("onRecvObserve: msg:", message, ", data:", data.data);
            appWindow.selectedContent = data.data
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
                webViewport.child.loadFrameScript("chrome://embedlite/content/embedhelper.js");
                webViewport.child.loadFrameScript("chrome://embedlite/content/SelectHelper.js");
                appWindow.mozViewInitialized = true
                webViewport.child.addMessageListeners([ "Content:ContextMenu", "Content:SelectionRange", "Content:SelectionCopied" ])
            }
            onRecvAsyncMessage: {
                // print("onRecvAsyncMessage:" + message + ", data:" + data)
            }
        }
    }

    resources: TestCase {
        id: testcaseid
        name: "mozContextPage"
        when: windowShown

        function cleanup() {
            mozContext.dumpTS("tst_inputtest cleanup")
        }

        function test_SelectionInit()
        {
            mozContext.dumpTS("test_SelectionInit start")
            verify(MyScript.waitMozContext())
            verify(MyScript.waitMozView())
            webViewport.child.url = "data:text/html,hello test selection";
            verify(MyScript.waitLoadFinished(webViewport))
            compare(webViewport.child.loadProgress, 100);
            while (!webViewport.child.painted) {
                wait();
            }
            webViewport.child.sendAsyncMessage("Browser:SelectionStart", {
                                                xPos: 56,
                                                yPos: 16
                                              })
            webViewport.child.sendAsyncMessage("Browser:SelectionMoveStart", {
                                                change: "start"
                                              })
            webViewport.child.sendAsyncMessage("Browser:SelectionCopy", {
                                                xPos: 56,
                                                yPos: 16
                                              })
            while (appWindow.selectedContent == "") {
                wait();
            }
            compare(appWindow.selectedContent, "test");
            mozContext.dumpTS("test_SelectionInit end")
        }
    }
}
