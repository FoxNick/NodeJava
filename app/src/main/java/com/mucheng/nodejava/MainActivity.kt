package com.mucheng.nodejava

import android.app.Activity
import android.os.Bundle
import com.mucheng.nodejava.core.Context
import com.mucheng.nodejava.core.Isolate
import java.lang.ref.WeakReference

class MainActivity : Activity() {

    companion object {

        @JvmStatic
        lateinit var currentMainActivity: WeakReference<MainActivity>
            private set

    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        currentMainActivity = WeakReference(this)

        val isolate = Isolate()
        val context = Context(isolate)
        context.injectJavaBridge()
        context.evaluateScript(readFromAssets("nodeJava.js"))
        context.evaluateScript(readFromAssets("test.js"))
    }

    private fun readFromAssets(path: String) = assets.open(path).bufferedReader().use { it.readText() }

}
