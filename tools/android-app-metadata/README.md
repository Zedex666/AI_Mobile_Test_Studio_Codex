# Android app metadata extractor

This helper runs through Android `app_process` and returns application labels and PNG icons as JSON.
The generated DEX JAR is bundled at `resources/tools/java/app_metadata.jar`.

Build prerequisites:

- JDK 8 or newer
- Android SDK platform `android.jar`
- Android SDK Build Tools `d8`

The desktop application pushes the JAR to `/data/local/tmp` on demand. It does not install an
application or leave a background service running on the connected device.
