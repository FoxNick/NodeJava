require("./rhino").install();
$java.setUnsafeReflectionEnabled(true);

const ctx = com.mojang.minecraftpe.MainActivity.currentMainActivity.get();

async function main() {
    const myViewClass = await $java.defineClass(
        class MyView extends android.widget.Button {
            constructor(...args) {
                super(...args);
            }
            onDraw(canvas) {
                super.onDraw(canvas);
                console.log(canvas);
            }
        },
        {
            packageName: "com.mucheng",
            implements: []
        }
    );
    ctx.setContentView(new myViewClass(ctx));
}

main().catch(console.error);