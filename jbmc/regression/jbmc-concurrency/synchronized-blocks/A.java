public class A
{
  public static void main()
  {
    final Object o = new Object();
    try
    {
      synchronized (o) {}
    }
    catch (NullPointerException e)
    {
      assert false;
      return;
    }
  }
}
