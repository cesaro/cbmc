import org.cprover.CProver;

// This test checks that two synchronized blocks interleave (before and after)
// and do not interfere with each other
public class Sync {
  
  // Counter for testing interleavings
  public int finalCounter=0;

  // Counter for testing synchronization
  public int testCounter=0;
  
  public boolean done=false;

  // Records which thread was taken on the first pass.
  public long firstPass=-1;
  
  public void test()
  {
    synchronized(this)
    {
      testCounter++;
      assert (testCounter==1);
      finalCounter++;
      testCounter--;
      if (finalCounter==1)
        firstPass=CProver.getCurrentThreadID();
      if (finalCounter==2)
        done=true;
    }
  }
  
  public static long run() throws InterruptedException
  {
    boolean threadFirst;
    final Sync o = new Sync();
    Thread1 thread=new Thread1(o);
    thread.start();
    o.test();
    if (o.finalCounter==1)
      o.firstPass=0;
    CProver.assume(thread.done);
    thread.join();
    if (thread.done)
    {
      assert(o.done);
      assert(o.firstPass>=0);
      assert(o.finalCounter<=2);
      return o.firstPass;
    }
    return -1;
  }
  
  public static void test1() throws InterruptedException
  {
    assert(run()==0);
  }
  
  public static void test2() throws InterruptedException
  {
    assert(run()==1);
  }
}

class Thread1 extends Thread
{
  /** Reference to the object the thread operates on
   * It is necessary because it represents the main synchronizing
   * mechanism between threads.
   */
  Sync object;
  
  public boolean done=false;
  
  public Thread1(Sync object)
  {
    super();    
    this.object = object;
  }

  /**
   * The run function executes the test function specified for each thread
   */
  @Override
  public void run ()
  {
    object.test();
    if (object.finalCounter==1)
      object.firstPass=1;
    done=true;
  }
}
