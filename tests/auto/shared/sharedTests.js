var component;

function shared_context1Init() {
    mozContext.dumpTS("test_context1Init start")
    testcaseid.verify(mozContext.instance !== undefined)
    while (mozContext.instance.initialized() === false) {
        testcaseid.wait(500)
    }
    testcaseid.verify(mozContext.instance.initialized())
    mozContext.dumpTS("test_context1Init end")
}
function shared_context2AcceleratedAPI()
{
    mozContext.dumpTS("test_context2AcceleratedAPI start")
    mozContext.instance.setIsAccelerated(true);
    testcaseid.verify(mozContext.instance.isAccelerated() === true)
    mozContext.dumpTS("test_context2AcceleratedAPI end")
}
function shared_context3PrefAPI()
{
    mozContext.dumpTS("test_context3PrefAPI start")
    mozContext.instance.setPref("test.embedlite.pref", "result");
    mozContext.dumpTS("test_context3PrefAPI end")
}
function shared_context4ObserveAPI()
{
    mozContext.dumpTS("test_context4ObserveAPI start")
    mozContext.instance.sendObserve("memory-pressure", null);
    mozContext.instance.addObserver("test-observe-message");
    mozContext.instance.sendObserve("test-observe-message", {msg: "testMessage", val: 1});
    while (lastObserveMessage === undefined) {
        mozContext.waitLoop()
    }
    testcaseid.compare(lastObserveMessage.msg, "test-observe-message");
    testcaseid.compare(lastObserveMessage.data.val, 1);
    testcaseid.compare(lastObserveMessage.data.msg, "testMessage");
    mozContext.dumpTS("test_context4ObserveAPI end")
}


