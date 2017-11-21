import java.lang.Thread;
import org.cprover.CProver;

public class A
{

  // expected verification success
  public void me()
  {
    B b = new B();
    b.me();
  }

  // expected verification failure
  public void me2()
  {
    C c = new C();
    c.me();
  }

  // Create 2 Threads, no shared variables
  // expected verification success
  //
  // FIXME: attempting to create more threads will
  // currently result in an exponential blow-up
  // in then number of clauses.
  public void me3()
  {
    for(int i = 0; i<2; i++)
    {
       D d = new D();
       d.start();
    }
  }
}

class D extends Thread
{
  int x = 0;

  @Override
  public void run()
  {
    x = 44;
    int local_x = x;
    assert(local_x == 44 || local_x == 10);
  }
  public void setX()
  {
    x = 10;
  }
}

class B implements Runnable
{
  // FIXME: this assertion does not always hod as per the java spec (because x is not volatile),
  // but cbmc currently doesn't reason about volatile variables
  int x = 0;
  @Override
  public void run()
  {
    x += 1;
    int local_x = x;
    assert(local_x == 1 || local_x == 2 || local_x == 3 ||
      local_x == 10 || local_x == 11 || local_x == 12 || local_x == 13);
  }

  // verification success
  public void me()
  {
    Thread tmp = new Thread(this);
    tmp.start();
    tmp.start();
    tmp.start();
    x = 10;
  }
}

class C implements Runnable
{
  int x = 0;

  @Override
  public void run()
  {
    x += 1;
    int local_x = x;
    assert(local_x == 1 || local_x == 2 || local_x == 3);
  }

  // verification success
  public void me()
  {
    Thread tmp = new Thread(this);
    tmp.start();
    tmp.start();
    tmp.start();
    x = 10;
  }
}
