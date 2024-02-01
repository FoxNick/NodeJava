require("./rhino").install();

$java.setUnsafeReflectionEnabled(true);
const mainActivity = com.mucheng.nodejava.MainActivity.currentMainActivity.get();
console.log(mainActivity);

console.log(process.cwd());