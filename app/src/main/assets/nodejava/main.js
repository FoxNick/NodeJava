require("./rhino").install();
$java.setUnsafeReflectionEnabled(true);

const ctx = com.mojang.minecraftpe.MainActivity.currentMainActivity.get();

async function main() {
    const myViewClass = await $java.defineClass(
        class MyView extends android.view.View {
            constructor(...args) {
                super(...args);
            }
            onDraw(canvas) {
                console.log(canvas);
            }
            onClick() {

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