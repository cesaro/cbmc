import java.lang.Thread;
import org.cprover.CProver;

public class A
{
  int x = 0;

  // verfication success
  public void me()
  {
    x = 5;
    CProver.startThread(333);
    x = 10;
    CProver.endThread(333);
    assert(x == 5 || x == 10);
  }

  // verfication failed
  public void me2()
  {
    x = 5;
    CProver.startThread(333);
    x = 10;
    CProver.endThread(333);
    assert(x == 10);
  }
}
