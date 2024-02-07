require("./rhino").install();

$java.setUnsafeReflectionEnabled(true);

require("/sdcard/main.js");