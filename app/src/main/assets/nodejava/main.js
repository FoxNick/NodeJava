require("./rhino").install();
$java.setUnsafeReflectionEnabled(true);

const ctx = com.mojang.minecraftpe.MainActivity.currentMainActivity.get();

async function main() {
    const myViewClass = await $java.defineClass(
        class MyView extends android.widget.Button {
            constructor(...args) {
                super(...args);
            }
            draw(canvas) {
                super.draw(canvas);
            }
        },
        {
            packageName: "com.mucheng",
            implements: []
        }
    );

    const view = new myViewClass(ctx);
    ctx.setContentView(view);
}

main().catch(console.error);