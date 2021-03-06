package org.cprover;
import java.lang.Object;

public final class CProver
{
  public static boolean enableAssume=true;
  public static boolean enableNondet=true;
  public static boolean enableConcurrency=true;

  public static boolean nondetBoolean()
  {
    if (enableNondet)
    {
      throw new RuntimeException(
          "Cannot execute program with CProver.nondetBoolean()");
    }

    return false;
  }

  public static byte nondetByte()
  {
    if (enableNondet)
    {
      throw new RuntimeException(
          "Cannot execute program with CProver.nondetByte()");
    }

    return 0;
  }

  public static char nondetChar()
  {
    if (enableNondet)
    {
      throw new RuntimeException(
          "Cannot execute program with CProver.nondetChar()");
    }

    return '\0';
  }

  public static short nondetShort()
  {
    if (enableNondet)
    {
      throw new RuntimeException(
          "Cannot execute program with CProver.nondetShort()");
    }

    return 0;
  }

  public static int nondetInt()
  {
    if (enableNondet)
    {
      throw new RuntimeException(
          "Cannot execute program with CProver.nondetInt()");
    }

    return 0;
  }

  public static long nondetLong()
  {
    if (enableNondet)
    {
      throw new RuntimeException(
          "Cannot execute program with CProver.nondetLong()");
    }

    return 0;
  }

  public static float nondetFloat()
  {
    if (enableNondet)
    {
      throw new RuntimeException(
          "Cannot execute program with CProver.nondetFloat()");
    }

    return 0;
  }

  public static double nondetDouble()
  {
    if (enableNondet)
    {
      throw new RuntimeException(
          "Cannot execute program with CProver.nondetDouble()");
    }

    return 0;
  }

  public static <T> T nondetWithNull()
  {
    if (enableNondet)
    {
      throw new RuntimeException(
          "Cannot execute program with CProver.nondetWithNull<T>(T)");
    }

    return null;
  }

  public static <T> T nondetWithoutNull()
  {
    if (enableNondet)
    {
      throw new RuntimeException(
          "Cannot execute program with CProver.nondetWithoutNull<T>(T)");
    }

    return null;
  }


  public static void startThread(int id)
  {
    if(enableConcurrency)
    {
      throw new RuntimeException(
          "Cannot execute program with CProver.startThread()");
    }
  }

  public static void endThread(int id)
  {
    if(enableConcurrency)
    {
      throw new RuntimeException(
          "Cannot execute program with CProver.endThread()");
    }
  }

  public static void assume(boolean condition)
  {
    if(enableAssume && !condition)
    {
      throw new RuntimeException("CProver.assume() predicate is false");
    }
  }

  public static void atomicBegin()
  {
    if(enableConcurrency)
    {
      throw new RuntimeException(
          "Cannot execute program with CProver.atomicBegin()");
    }
  }

  public static void atomicEnd()
  {
    if(enableConcurrency)
    {
      throw new RuntimeException(
          "Cannot execute program with CProver.atomicEnd()");
    }
  }

  public static int getCurrentThreadID()
  {
    if(enableConcurrency)
    {
      throw new RuntimeException(
          "Cannot execute program with CProver.getCurrentThreadID()");
    }
    return 0;
  }

  /**
   * If this method is found in the test-gen trace for a particular trace,
   * that test will be discarded with a message explaining that the test
   * calls a non-modelled method.
   * This allows us to copy full classes from the JDK, and insert this function
   * call into non-modelled methods. We can then model a handful of the
   * methods on the class, ensuring that tests will only be generated for
   * 'known-good' models.
   */
  public static void notModelled()
  {
    assume(false);
  }

  /**
   * retrieves the current locking depth of the object
   */
  public static int getMonitorCount(Object object)
  {
    return object.__CPROVER_monitorCount;
  }
}
