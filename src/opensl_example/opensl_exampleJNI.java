package opensl_example;

public class opensl_exampleJNI {

  static {
    try {
        java.lang.System.loadLibrary("opensl_example");
    } catch (UnsatisfiedLinkError e) {
        java.lang.System.err.println("native code library failed to load.\n" + e);
        java.lang.System.exit(1);
    }
  }

  public final static native void start_process(int SDK);
  public final static native void stop_process();
}
