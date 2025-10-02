# Schedd
## Task-Scheduling for Chores

Schedd schedules your tasks and chores for the day.
By inputting your tasks, their duration, timeframe of repetition and priority Schedd outputs the best fitted TODO-List for the day.

Via Web-UI:
- Input your available time for chores for each day of the week
- Add tasks including duration, timeframe of repetition, priority

C++ Webserver (via crow):
- Scheduling the tasks to fit the most tasks for the day (sorted by priority and how long ago they were due)
- Rescheduling each day (taking into account taks that weren't done the day they were scheduled as well as new tasks that are due this day)


Support for multiple Users and shared Tasks.
