require("./rhino").install();

$java.setUnsafeReflectionEnabled(true);
const mainActivity = com.mucheng.nodejava.MainActivity.currentMainActivity.get();
