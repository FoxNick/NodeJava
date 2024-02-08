require("./rhino").install();

$java.setUnsafeReflectionEnabled(true);

const ctx = com.mojang.minecraftpe.MainActivity.currentMainActivity.get();
const btn = android.widget.Button(ctx);

ctx.runOnUiThread(function() {

});