#ifndef SLAVE_THREAD_HXX_
#define SLAVE_THREAD_HXX_

/**
 * @file
 * @author Jason Lingse
 * @brief This file provides an interface that allows Tcl to perform
 * work on a separate thread.
 *
 * This is especially useful for
 * true asynchronous HTTP requests (::http performs a synchronous
 * DNS query and connection, which often can take quite a while).
 *
 * The interface allows passing arbitrary strings between the threads
 * (bkg_req/bkg_rcv for main->slave and bkg_ans/bkg_get for slave->main)
 * as well as a facility for the background thread to wait until new
 * data arrives (bkg_wait).
 */

/** Sends the specified string to the background thread.
 * If the specified name is non-empty, any currently-queued strings
 * with that name are discarded.
 */
void bkg_req(const char*, const char* = "");
/** Sends the specified string to the foreground thread.
 * If the specified name is non-empty, any currently-queued strings
 * with that name are discarded.
 */
void bkg_ans(const char*, const char* = "");

/** Returns the next string sent from the foreground to the background
 * thread, or an empty string if there is none.
 */
const char* bkg_rcv();
/** Returns the next string sent from the background to the foreground
 * thread, or an empty string if there is none.
 */
const char* bkg_get();

/** Starts the background thread. */
void bkg_start();

/** Terminates the background thread.
 * This does NOT reready the system for another call to
 * bkg_start() --- it only kills the thread.
 * It is rather important that this function will leak all memory
 * used by the slave thread. It should only be used at shutdown,
 * since SDL segfaults if a thread is terminated while locked
 * or waiting on a condition.
 */
void bkg_term();

/** Blocks until the background thread has messages. */
void bkg_wait();

#endif /* SLAVE_THREAD_HXX_ */
