/*
 * Copyright (c) 1997, 2008, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
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
 *
 */

// A PeriodicTask has the sole purpose of executing its task
// function with regular intervals.
// Usage:
//   PeriodicTask pf(10);
//   pf.enroll();
//   ...
//   pf.disenroll();

class PeriodicTask: public CHeapObj {
 public:
  // Useful constants.
  // The interval constants are used to ensure the declared interval
  // is appropriate;  it must be between min_interval and max_interval,
  // and have a granularity of interval_gran (all in millis).
  enum { max_tasks     = 10,       // Max number of periodic tasks in system
         interval_gran = 10,
         min_interval  = 10,
         max_interval  = 10000 };

  static int num_tasks()   { return _num_tasks; }

 private:
  size_t _counter;
  const size_t _interval;

  static int _num_tasks;
  static PeriodicTask* _tasks[PeriodicTask::max_tasks];
  static void real_time_tick(size_t delay_time);

#ifndef PRODUCT
  static elapsedTimer _timer;                      // measures time between ticks
  static int _ticks;                               // total number of ticks
  static int _intervalHistogram[max_interval];     // to check spacing of timer interrupts
 public:
  static void print_intervals();
#endif
  // Only the WatcherThread can cause us to execute PeriodicTasks
  friend class WatcherThread;
 public:
  PeriodicTask(size_t interval_time); // interval is in milliseconds of elapsed time
  ~PeriodicTask();

  // Tells whether is enrolled
  bool is_enrolled() const;

  // Make the task active
  // NOTE: this may only be called before the WatcherThread has been started
  void enroll();

  // Make the task deactive
  // NOTE: this may only be called either while the WatcherThread is
  // inactive or by a task from within its task() method. One-shot or
  // several-shot tasks may be implemented this way.
  void disenroll();

  void execute_if_pending(size_t delay_time) {
    _counter += delay_time;
    if (_counter >= _interval) {
      _counter = 0;
      task();
    }
  }

  // Returns how long (time in milliseconds) before the next time we should
  // execute this task.
  size_t time_to_next_interval() const {
    assert(_interval > _counter,  "task counter greater than interval?");
    return _interval - _counter;
  }

  // Calculate when the next periodic task will fire.
  // Called by the WatcherThread's run method.
  // This assumes that periodic tasks aren't entering the system
  // dynamically, except for during startup.
  static size_t time_to_wait() {
    if (_num_tasks == 0) {
      // Don't wait any more; shut down the thread since we don't
      // currently support dynamic enrollment.
      return 0;
    }

    size_t delay = _tasks[0]->time_to_next_interval();
    for (int index = 1; index < _num_tasks; index++) {
      delay = MIN2(delay, _tasks[index]->time_to_next_interval());
    }
    return delay;
  }

  // The task to perform at each period
  virtual void task() = 0;
};
