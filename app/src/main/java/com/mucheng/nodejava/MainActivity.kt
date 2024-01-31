package com.mucheng.nodejava

import android.app.Activity
import android.os.Bundle
import android.util.Log
import com.mucheng.nodejava.core.Context
import com.mucheng.nodejava.core.Isolate
import java.io.File
import java.lang.ref.WeakReference

class MainActivity : Activity() {

    companion object {

        @JvmStatic
        lateinit var currentMainActivity: WeakReference<MainActivity>

    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        currentMainActivity = WeakReference(this)
        extraAssets()

        val isolate = Isolate()
        val context = Context(isolate, filesDir.absolutePath)
        context.injectJavaBridge()
        context.evaluateScript(File(filesDir, "nodeJava.js").readText())
        context.evaluateScript(File(filesDir, "main.js").readText())
    }

    private fun extraAssets() {
        val baseName = "nodejava"
        val items = assets.list(baseName) ?: emptyArray()
        for (item in items) {
            val fullName = "$baseName/$item"
            val content = readFromAssets(fullName)
            File(filesDir, item).writeText(content)
        }
    }

    private fun readFromAssets(path: String) =
        assets.open(path).bufferedReader().use { it.readText() }

}
