/*------------------------------------------------------------------------
 *  NativeCleanup
 *------------------------------------------------------------------------*/

package net.sourceforge.zbar;

import java.lang.ref.Cleaner;

/** Shared Cleaner for objects that own native resources. */
final class NativeCleanup
{
    private static final Cleaner cleaner = Cleaner.create();

    private NativeCleanup ()
    {
    }

    static Cleaner.Cleanable register (Object owner, Runnable cleanup)
    {
        return(cleaner.register(owner, cleanup));
    }
}
