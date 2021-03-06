/*
 * Copyright (c) 1994, 2012, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.  Oracle designates this
 * particular file as subject to the "Classpath" exception as provided
 * by Oracle in the LICENSE file that accompanied this code.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */

package java.lang;

import org.cprover.CProver;
import java.lang.NullPointerException;
import java.lang.IllegalMonitorStateException;
import java.lang.Thread;

public class Object {

    // lock needed for synchronization in cbmc
    // used by monitorenter, monitorexit, wait, and notify
    // Not present in the original Object class
    public int __CPROVER_monitorCount;
    public int __CPROVER_holdingThreadId;
    public boolean __CPROVER_ready;

    public Object() {
      __CPROVER_monitorCount = 0;
      __CPROVER_ready = false;
    }

    public final Class<?> getClass() {
      /*
       * MODELS LIBRARY {
       *   We make this call to Class.forName to ensure it is loaded
       *   by CBMC even with --lazy-methods on. We have to do this
       *   because the internal support for getClass use the model of
       *   Class.forName.
       * }
       */
      Class c = Class.forName("");
      return CProver.nondetWithoutNull();
    }

    public int hashCode() {
      return 0;
    }

    public boolean equals(Object obj) {
        return (this == obj);
    }

    protected Object clone() throws CloneNotSupportedException {
      throw new CloneNotSupportedException();
    }

    public String toString() {
        return getClass().getName() + "@" + Integer.toHexString(hashCode());
    }

    public final void notify()
    {
/*        // The thread must own the lock when it calls notify
        if(monitorCount == 0)
        {
            throw new IllegalMonitorStateException();
        }
        // We release all locks temporarilly to allow waiting threads to
        // reacquire it.
        int tempMonitorCount = monitorCount;
        monitorCount=0;

        // FIXME: notify should only wake up one thread

        // We reacquire all locks and continue
        monitorCount=tempMonitorCount;*/
    }

    // See implementation of notify
    public final void notifyAll()
    {
      __CPROVER_ready = true;
/*        if (monitorCount == 0)
        {
            throw new IllegalMonitorStateException();
        }
        int tempMonitorCount = monitorCount;
        monitorCount=0;

        // FIXME: notify should only wake up all threads

        monitorCount=tempMonitorCount;*/
    }

    public final void wait(long timeout) throws InterruptedException {
      // The thread must own the lock when it calls wait
      /*if (monitorCount == 0)
      {
          throw new IllegalMonitorStateException();
      }

      int tempMonitorCount = monitorCount;
      monitorCount=0;

      // FIXME: we require notifies to be able to interpose here

      monitorCount=tempMonitorCount;*/

      // FIXME: should only throw if the interrupted flag in Thread is enabled
      throw new InterruptedException();
    }

    public final void wait(long timeout, int nanos) throws InterruptedException
    {
        if (timeout < 0)
        {
            throw new IllegalArgumentException("timeout value is negative");
        }

        if (nanos < 0 || nanos > 999999) {
          throw new IllegalArgumentException(
            "nanosecond timeout value out of range");
        }

        if (nanos > 0) {
          timeout++;
        }

        wait(timeout);
    }

    public final void wait() throws InterruptedException {
      CProver.atomicBegin();
      monitorexit(this);
      CProver.atomicEnd();
      CProver.atomicBegin();
      CProver.assume(__CPROVER_ready);
      __CPROVER_ready = false;
      CProver.atomicEnd();
      CProver.atomicBegin();
      monitorenter(this);
      CProver.atomicEnd();
      // wait(0);
    }

    protected void finalize() throws Throwable { }

    public static void monitorenter(Object object)
    {
      // FIXME: Temporarily removed functionality to simplify concurrency
      // if (object == null) //FIXME : remove last entry of the stack trace
      //   throw new NullPointerException();
      // int id=CProver.getCurrentThreadID();
      CProver.atomicBegin();
      CProver.assume(object.__CPROVER_monitorCount == 0);
      //  || (object.holdingThreadId==id));
      object.__CPROVER_monitorCount++;
      // object.holdingThreadId=id;
      CProver.atomicEnd();
    }

    public static void monitorexit(Object object)
    {
/*
      if (object == null) //FIXME : this should throw outside
        throw new NullPointerException();
      if (object.monitorCount == 0)
      {
        throw new IllegalMonitorStateException();
      }*/
      CProver.atomicBegin();
      object.__CPROVER_monitorCount--;
      CProver.atomicEnd();
    }
}
