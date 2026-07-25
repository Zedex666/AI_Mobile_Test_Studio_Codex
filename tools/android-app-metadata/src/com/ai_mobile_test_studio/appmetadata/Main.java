package com.ai_mobile_test_studio.appmetadata;

import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.drawable.Drawable;
import android.os.Looper;
import android.util.Base64;
import java.io.ByteArrayOutputStream;
import java.lang.reflect.Method;

public final class Main {
    private Main() {}

    public static void main(String[] args) {
        try {
            if (args.length == 0) {
                return;
            }
            if (Looper.myLooper() == null) {
                Looper.prepare();
            }

            Class<?> activityThreadClass = Class.forName("android.app.ActivityThread");
            Method systemMainMethod = activityThreadClass.getMethod("systemMain");
            Object activityThread = systemMainMethod.invoke(null);
            Method getSystemContextMethod = activityThreadClass.getMethod("getSystemContext");
            Context context = (Context) getSystemContextMethod.invoke(activityThread);
            PackageManager packageManager = context.getPackageManager();

            System.out.println("[");
            for (int index = 0; index < args.length; index++) {
                if (index > 0) {
                    System.out.println(",");
                }
                writePackage(packageManager, args[index]);
            }
            System.out.println("\n]");
            System.exit(0);
        } catch (Exception error) {
            System.out.println("{\"error\":\"" + escape(error.getMessage()) + "\"}");
            System.exit(1);
        }
    }

    private static void writePackage(PackageManager packageManager, String packageName) {
        try {
            ApplicationInfo appInfo = packageManager.getApplicationInfo(packageName, 0);
            String label = packageManager.getApplicationLabel(appInfo).toString();
            Drawable icon = packageManager.getApplicationIcon(appInfo);

            int width = icon.getIntrinsicWidth() > 0 ? icon.getIntrinsicWidth() : 128;
            int height = icon.getIntrinsicHeight() > 0 ? icon.getIntrinsicHeight() : 128;
            width = Math.min(width, 192);
            height = Math.min(height, 192);

            Bitmap bitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
            Canvas canvas = new Canvas(bitmap);
            icon.setBounds(0, 0, width, height);
            icon.draw(canvas);

            ByteArrayOutputStream output = new ByteArrayOutputStream();
            bitmap.compress(Bitmap.CompressFormat.PNG, 100, output);
            bitmap.recycle();
            String iconBase64 = Base64.encodeToString(output.toByteArray(), Base64.NO_WRAP);

            System.out.print("{\"package\":\"" + escape(packageName)
                    + "\",\"label\":\"" + escape(label)
                    + "\",\"icon\":\"data:image/png;base64," + iconBase64 + "\"}");
        } catch (Exception error) {
            System.out.print("{\"package\":\"" + escape(packageName)
                    + "\",\"error\":\"" + escape(error.getMessage()) + "\"}");
        }
    }

    private static String escape(String value) {
        if (value == null) {
            return "Unknown error";
        }
        return value.replace("\\", "\\\\")
                .replace("\"", "\\\"")
                .replace("\n", "")
                .replace("\r", "");
    }
}
