require("./rhino").install();

$java.setUnsafeReflectionEnabled(true);

const thread = java.lang.Thread(() => {
    console.log("exec")
    while (true) {
        java.lang.Thread.sleep(500)
    }
    console.log("Wtf")
});
thread.start();

setInterval(() => console.log('ticked'), 200);