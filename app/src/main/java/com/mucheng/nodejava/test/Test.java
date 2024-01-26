package com.mucheng.nodejava.test;

import android.util.Log;

public class Test {

    public int a = 10;

    public static int b = 20;

    public void print(String s) {
        Log.e("Test", "print: " + s);
    }

    public static void print(int v) {
        Log.e("Test", "print: " + v);
    }

}
