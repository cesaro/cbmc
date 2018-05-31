import java.lang.Thread;
import org.cprover.CProver;

public class A
{
  public static int g;

  // expected verification success
  public void me()
  {
    int g = CProver.getCurrentThreadID();
    assert(g == 0);
  }

  // expected verification success
  // --
  // KNOWN-BUG
  // FIXME: This should work, however there are problems with the symex that
  // prevent it from working.
  public void me2()
  {
    CProver.startThread(333);
    g = CProver.getCurrentThreadID();
    assert(g == 1);
    CProver.endThread(333);
  }

  // expected verification success
  // KNOWN-BUG
  // FIXME: This should work, however there are problems with the symex that
  // prevent it from working.
  public void me3()
  {
    CProver.startThread(333);
    int i = CProver.getCurrentThreadID();
    assert(g == 1);
    CProver.endThread(333);
  }

  // expected verification success.
  public void me4()
  {
    CProver.startThread(333);
    check();
    CProver.endThread(333);
  }

  // expected verification success.
  public void me5()
  {
    me();
    B b = new B();
    Thread tmp = new Thread(b);
    tmp.start();
  }

  // expected verification success.
  public void me6()
  {
    me();
    C c = new C();
    c.start();
  }

  public void check()
  {
    g = CProver.getCurrentThreadID();
    assert(g == 1);
  }
}

class B implements Runnable
{
  public static int g;
  @Override
  public void run()
  {
    g = CProver.getCurrentThreadID();
    assert(g == 1);
  }
}


class C extends Thread
{
  public static int g;
  @Override
  public void run()
  {
    g = CProver.getCurrentThreadID();
    assert(g == 1);
  }
}
