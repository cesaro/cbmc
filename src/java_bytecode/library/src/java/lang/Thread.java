package java.lang;

import org.cprover.CProver;

public class Thread implements Runnable
{
  private Runnable target;
  private long tid;
  private volatile static long threadSeqNumber;

  public Thread()
  {
    target = this;
    tid = ++threadSeqNumber;
  }

  public Thread(Runnable target)
  {
    this.target = target;
    tid = ++threadSeqNumber;
  }

  public void start()
  {
    CProver.startThread(333);
    target.run();
    CProver.endThread(333);
  }

  @Override
  public void run()
  {
    // FIXME:, uncommenting the following code leads to infinite recursion.
    // if (target != null)
    // {
    //   target.run();
    // }
  }

  public long getId()
  {
    return tid;
  }
}
