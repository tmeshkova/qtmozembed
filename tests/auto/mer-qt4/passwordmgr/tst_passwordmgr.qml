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
    property variant promptReceived : null

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
            mozContext.instance.addComponentManifest(mozContext.getenv("QTTESTPATH") + "/components/TestHelpers.manifest");
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
                webViewport.child.loadFrameScript("chrome://tests/content/testHelper.js");
                appWindow.mozViewInitialized = true
                webViewport.child.addMessageListener("embed:login");
            }
            onRecvAsyncMessage: {
                // print("onRecvAsyncMessage:" + message + ", data:" + data)
                if (message == "embed:login") {
                    webViewport.child.sendAsyncMessage("embedui:login", {
                                                        buttonidx: 1,
                                                        id: data.id
                                                       })
                    appWindow.promptReceived = true;
                }
            }
        }
    }

    resources: TestCase {
        id: testcaseid
        name: "mozContextPage"
        when: windowShown

        function cleanup() {
            mozContext.dumpTS("tst_passwordmgr cleanup")
        }

        function test_TestLoginMgrPage()
        {
            mozContext.dumpTS("test_TestLoginMgrPage start")
            verify(MyScript.waitMozContext())
            verify(MyScript.waitMozView())
            appWindow.promptReceived = null
            webViewport.child.url = mozContext.getenv("QTTESTPATH") + "/auto/passwordmgr/subtst_notifications_1.html";
            verify(MyScript.waitLoadFinished(webViewport))
            compare(webViewport.child.loadProgress, 100);
            while (!webViewport.child.painted) {
                wait();
            }
            while (!appWindow.promptReceived) {
                wait();
            }
            mozContext.dumpTS("test_TestLoginMgrPage end");
        }
    }
}
