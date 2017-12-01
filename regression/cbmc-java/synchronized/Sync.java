public class Sync {
  public static void main(String[] args) {
    final Object o = new Object();
    try {
      synchronized (o) {}
    } catch (NullPointerException e) {
      assert false;
      return;
    }
  }
}
