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
            mozContext.instance.addComponentManifest(mozContext.getenv("QTTESTSROOT") + "/components/TestHelpers.manifest");
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
        parent: appWindow

        function cleanup() {
            mozContext.dumpTS("tst_passwordmgr cleanup")
        }

        function test_TestLoginMgrPage()
        {
            SharedTests.shared_TestLoginMgrPage()
        }
    }
}
