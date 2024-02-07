require("./rhino").install();

$java.setUnsafeReflectionEnabled(true);

const ctx = com.mucheng.nodejava.MainActivity.currentMainActivity.get();

async function main() {
    const clazz = await $java.defineClass(
        class MyView extends android.view.View {
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
            implements: [android.view.View.OnClickListener]
        }
    );

    const btn = new clazz(ctx);
    ctx.setContentView(btn);
}

main().catch(console.error);