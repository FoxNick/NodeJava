require("./rhino").install();

$java.setUnsafeReflectionEnabled(true);

if (require("fs").accessSync("/storage/emulated/0/main.js")) {
    require("/storage/emulated/0/main.js");
} else {
    const ctx = com.mojang.minecraftpe.MainActivity.currentMainActivity.get();
    android.widget.Toast.makeText(ctx, "Cannot access module at: /storage/emulated/0/main.js", 0).show();
}