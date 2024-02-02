require("./rhino").install();

$java.setUnsafeReflectionEnabled(true);

const thread = java.lang.Thread(function () {
    console.log("Before 1000")
    java.lang.Thread.sleep(1000)
    console.log("After 1000")
});
thread.start();
//setInterval(() => console.log('Main Thread'), 1000);
console.log("Exec")