require("./rhino").install();

$java.setUnsafeReflectionEnabled(true);

const ctx = com.mucheng.nodejava.MainActivity.currentMainActivity.get();

class MyButton extends android.widget.Button {
    constructor(context) {
        super(context);
    }

    onDraw(canvas) {
        console.log(canvas);
    }
}

const btn = new MyButton(ctx);
ctx.setContentView(btn);