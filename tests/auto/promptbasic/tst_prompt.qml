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
    property variant testResult : null

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
                webViewport.child.addMessageListener("testembed:elementinnervalue");
                webViewport.child.addMessageListener("embed:prompt");
            }
            onRecvAsyncMessage: {
                // print("onRecvAsyncMessage:" + message + ", data:" + data)
                if (message == "embed:prompt") {
                    testcaseid.compare(data.defaultValue, "Your name");
                    testcaseid.compare(data.text, "Please enter your name:");
                    webViewport.child.sendAsyncMessage("promptresponse", {
                                                         winid: data.winid,
                                                         checkval: true,
                                                         accepted: true,
                                                         promptvalue: "expectedPromptResult"
                                                     })
                    appWindow.promptReceived = true;
                } else if (message == "testembed:elementinnervalue") {
                    appWindow.testResult = data.value;
                }
            }
        }
    }

    resources: TestCase {
        id: testcaseid
        name: "mozContextPage"
        when: windowShown

        function cleanup() {
            mozContext.dumpTS("tst_multitouch cleanup")
        }

        function test_TestPromptPage()
        {
            mozContext.dumpTS("test_TestPromptPage start")
            verify(MyScript.waitMozContext())
            verify(MyScript.waitMozView())
            webViewport.child.url = mozContext.getenv("QTTESTPATH") + "/auto/promptbasic/prompt.html";
            verify(MyScript.waitLoadFinished(webViewport))
            compare(webViewport.child.loadProgress, 100);
            while (!webViewport.child.painted) {
                wait();
            }
            while (!appWindow.promptReceived) {
                wait();
            }
            webViewport.child.sendAsyncMessage("embedtest:getelementinner", {
                                                name: "result" })
            while (!appWindow.testResult) {
                wait();
            }
            compare(appWindow.testResult, "ok");
            mozContext.dumpTS("test_TestPromptPage end");
        }
    }
}
