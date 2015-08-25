
/** \page pageDebugging Useful Resources for Debugging
 *
 * <b>CORE Files & GDB/DDD</b>
 *
 *   - Don't forget to compile code with <code>g++ -g</code> to include debugging information
 *
 *   - <code>ulimit -c unlimited</code> &mdash; generate \c core files
 *
 *   - <code>./path/to/program</code> &mdash; if it crashes, generates \c core file
 *
 *   - <code>gdb ./path/to/program -c core</code> &mdash; load core file into GDB
 *
 *   - inside GDB: <code>bt</code> &mdash; display backtrace
 *
 *
 * <b>Using <code>libSegFault.so</code></b>
 *
 * Useful trick:
 *
 * <code>env SEGFAULT_SIGNALS="abrt segv"
 *       LD_PRELOAD=/lib/x86_64-linux-gnu/libSegFault.so ./path/to/program</code>
 * (adapt to location of <code>libSegFault.so</code>): this will print the stack trace
 * if the given signals are caught. You don't get line numbers though.
 *
 */
