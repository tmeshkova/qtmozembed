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
    property variant favicon : null

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
        active: true
        anchors.fill: parent
        Connections {
            target: webViewport.child
            onViewInitialized: {
                appWindow.mozViewInitialized = true
                webViewport.child.addMessageListener("chrome:linkadded");
            }
            onRecvAsyncMessage: {
                print("onRecvAsyncMessage:" + message + ", data:" + data)
                if (message == "chrome:linkadded" && data.get == "image/x-icon") {
                    appWindow.favicon = data.href;
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
            mozContext.dumpTS("tst_multitouch cleanup")
        }

        function test_TestFaviconPage()
        {
            SharedTests.shared_TestFaviconPage()
        }
    }
}
