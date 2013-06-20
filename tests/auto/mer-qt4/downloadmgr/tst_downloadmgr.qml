import QtQuickTest 1.0
import QtQuick 1.0
import Sailfish.Silica 1.0
import QtMozilla 1.0
import "../../shared/componentCreation.js" as MyScript
import "../../shared/sharedTests.js" as SharedTests

ApplicationWindow {
    id: appWindow

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
        onRecvObserve: {
            if (message == "embed:download") {
                // print("onRecvObserve: msg:" + message + ", dmsg:" + data.msg);
                if (data.msg == "dl-done") {
                    appWindow.promptReceived = true;
                }
            }
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
                webViewport.child.addMessageListener("embed:filepicker");
                appWindow.mozViewInitialized = true
            }
            onRecvAsyncMessage: {
                // print("onRecvAsyncMessage:" + message + ", data:" + data)
                if (message == "embed:filepicker") {
                    webViewport.child.sendAsyncMessage("filepickerresponse", {
                                                     winid: data.winid,
                                                     accepted: true,
                                                     items: ["/tmp/tt.bin"]
                                                 })
                }
            }
        }
    }

    resources: TestCase {
        id: testcaseid
        name: "mozContextPage"
        when: windowShown

        function cleanup() {
            mozContext.dumpTS("tst_downloadmgr cleanup")
        }

        function test_TestDownloadMgrPage()
        {
            SharedTests.shared_TestDownloadMgrPage()
        }
    }
}
