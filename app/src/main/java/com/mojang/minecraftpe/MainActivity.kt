package com.mojang.minecraftpe

import android.app.Activity
import android.graphics.Canvas
import android.os.Bundle
import android.util.AttributeSet
import android.widget.Button
import com.mucheng.nodejava.core.Context
import com.mucheng.nodejava.core.Isolate
import java.io.File
import java.lang.ref.WeakReference

class MainActivity : Activity() {

    companion object {

        @JvmStatic
        lateinit var currentMainActivity: WeakReference<MainActivity>

    }

    private lateinit var isolate: Isolate

    private lateinit var context: Context

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        currentMainActivity = WeakReference(this)
        extraAssets()

        isolate = Isolate()
        context = Context(isolate, filesDir.absolutePath)
        context.injectJavaBridge()
        context.evaluateScript(File(filesDir, "nodeJava.js").readText())
        context.evaluateScript(File(filesDir, "main.js").readText())
        context.spinEventLoopNoWait()
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