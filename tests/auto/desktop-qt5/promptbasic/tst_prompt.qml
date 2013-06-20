import QtTest 1.0
import QtQuick 2.0
import Qt5Mozilla 1.0
import "../../shared/componentCreation.js" as MyScript
import "../../shared/sharedTests.js" as SharedTests

Item {
    id: appWindow

    property bool mozViewInitialized : false
    property variant promptReceived : null
    property variant testResult : null
    property variant testCaseNum : 0

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
                webViewport.child.addMessageListener("testembed:elementinnervalue");
                webViewport.child.addMessageListener("embed:prompt");
            }
            onRecvAsyncMessage: {
                // print("onRecvAsyncMessage:" + message + ", data:" + data)
                if (message == "embed:prompt") {
                    testcaseid.compare(data.defaultValue, "Your name");
                    testcaseid.compare(data.text, "Please enter your name:");
                    var responsePrompt = null;
                    switch(appWindow.testCaseNum) {
                        case 0:
                            responsePrompt="expectedPromptResult";
                            break;
                        case 1:
                            responsePrompt="unexpectedPromptResult";
                            break;
                    }
                    if (responsePrompt) {
                        webViewport.child.sendAsyncMessage("promptresponse", {
                                                            winid: data.winid,
                                                            checkval: true,
                                                            accepted: true,
                                                            promptvalue: responsePrompt
                                                          })
                    }
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
            SharedTests.shared_TestPromptPage()
        }

        function test_TestPromptWithBadResponse()
        {
            SharedTests.shared_TestPromptWithBadResponse()
        }

        function test_TestPromptWithoutResponse()
        {
            SharedTests.shared_TestPromptWithoutResponse()
        }
    }
}
