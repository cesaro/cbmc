package java.lang;

import org.cprover.CProver;

public class Thread implements Runnable
{
  private Runnable __CPROVER_target;
  private long __CPROVER_tid;
  private volatile static long __CPROVER_threadSeqNumber;

  public Thread()
  {
    __CPROVER_target = this;
    __CPROVER_tid = ++__CPROVER_threadSeqNumber;
  }

  public Thread(Runnable target)
  {
    this.__CPROVER_target = target;
    __CPROVER_tid = ++__CPROVER_threadSeqNumber;
  }

  public void start()
  {
    CProver.startThread(333);
    __CPROVER_target.run();
    CProver.endThread(333);
  }

  @Override
  public void run()
  {
    // FIXME:, uncommenting the following code leads to infinite recursion.
    // if (__CPROVER_target != null)
    // {
    //   __CPROVER_target.run();
    // }
  }

  public long get_tid()
  {
    return __CPROVER_tid;
  }
}
