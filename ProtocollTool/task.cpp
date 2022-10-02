#include "task.h"
#include "conversions.h"

#include <limits>
#include <time.h>
#include <algorithm>
#include <iostream>

int Task::get_value() {
	return -1;
}

TIMESLOT Task::get_valid_timeslot()
{
	TIMESLOT timeslot;
	for (const DATE_CONSTRAINT& constr : date_constraints) {
		if (constr.date_type == DATE_TYPE::AT) {
			timeslot.start = constr.date;
			timeslot.end = constr.date + duration;
			return timeslot;
		}
		if (constr.date_type == DATE_TYPE::ANYTIME) {
			timeslot.start = 0;
			timeslot.end = std::numeric_limits<time_t>::max();
			return timeslot;
		}
		if (constr.date_type == DATE_TYPE::AFTER) {
			timeslot.start = constr.date;
		}
		if (constr.date_type == DATE_TYPE::BEFORE) {
			timeslot.end = constr.date;
		}
	}
	return timeslot;
}

TIMESLOT Schedule::next_free_timeslot(time_t date) {
	/**
	* Get next free timeslot for the day given with date.
	* Note that only times in the range from start_times to working_times 
	* and times that are after the given date are considered.
	* 
	* If no timeslot is found, a timeslot with -1 begin and end is returned
	* 
	* @param date Date after which the timeslot will be searched for
	* @return next free timeslot, with begin = -1 and end = -1 if none is found for that day.
	*/
	std::map<time_t, Task>::iterator it = schedule.begin();

	// skip past dates
	while (it != schedule.end()) {
		if (it->first >= date) {
			break;
		}
		it++;
	}
	
	time_t schedule_day_start = get_day_start(date, start_times);
	time_t schedule_day_end = get_day_end(date, start_times, working_times);
	// no event planned on this day
	if (it == schedule.end() || it->first >= schedule_day_end) {
		TIMESLOT timeslot;
		timeslot.start = schedule_day_start;
		timeslot.end = schedule_day_end;
		return timeslot;
	}

	// find out if there is free time from start of day to first event
	if (it->first > schedule_day_start) {
		TIMESLOT timeslot;
		timeslot.start = schedule_day_start;
		timeslot.end = it->first;
		return timeslot;
	}

	// find out if there is time between events
	while (std::next(it) != schedule.end()) {
		time_t current_end = it->first + it->second.duration;
		time_t next_start = std::next(it)->first;
		if (current_end != next_start) {
			TIMESLOT timeslot;
			timeslot.start = current_end;
			if (next_start > schedule_day_end) {
				timeslot.end = schedule_day_end;
				return timeslot;
			}
			
			timeslot.end = next_start;
			return timeslot;
		}

		// nothing free found for this day
		if (next_start >= schedule_day_end) {
			TIMESLOT timeslot;
			timeslot.start = -1;
			timeslot.end = -1;
			return timeslot;
		}
		it++;
	}

	// find out if there is time between last event on this day and end of 
	// work day
	if (std::next(it)->first + std::next(it)->second.duration < schedule_day_end) {
		TIMESLOT timeslot;
		timeslot.start = std::next(it)->first + std::next(it)->second.duration;
		timeslot.end = schedule_day_end;
		return timeslot;
	}

	TIMESLOT timeslot;
	timeslot.start = -1;
	timeslot.end = -1;
	return timeslot;
}

bool Schedule::timeslot_is_free(TIMESLOT timeslot) {
	std::map<time_t, Task>::iterator it = schedule.begin();

	// skip past dates
	while (it != schedule.end()) {
		if (it->first >= timeslot.start) {
			break;
		}
		it++;
	}

	if (it == schedule.end() || it->first >= timeslot.end) {
		return true;
	}

	return false;
}

TIMESLOT Schedule::can_add(time_t duration, time_t day) {
	TIMESLOT timeslot = next_free_timeslot(day);

	// no timeslot at all
	if (timeslot.start == -1 || timeslot.end == -1)
	{
		return timeslot;
	}
		

	// skip too short timeslots
	while (timeslot.end - timeslot.start < duration) {
		timeslot = next_free_timeslot(timeslot.end);
		if (timeslot.start == -1 || timeslot.end == -1) {
			return timeslot;
		}
	}

	return timeslot;
}

TIMESLOT Schedule::add(Task task, time_t day) {
	TIMESLOT timeslot = can_add(task.duration, day);

	// not success full, TODO: try splitting task. Avoid too strong fragmentation, split into in roughly equal time parts
	if (timeslot.start == -1 || timeslot.end == -1) {
		return timeslot;
	}
	schedule[timeslot.start] = task;
	return timeslot;
}

TIMESLOT Schedule::add(Task task, TIMESLOT timeslot) {
	if (timeslot_is_free(timeslot)) {
		schedule[timeslot.start] = task;
		return timeslot;
	}
	TIMESLOT timeslot;
	timeslot.start = -1;
	timeslot.end = -1;
	return timeslot;
}

bool Schedule::remove(int task_id) {
	std::map<time_t, Task>::iterator it = schedule.begin();
	while (it != schedule.end()) {
		if (it->second.id == task_id) {
			schedule.erase(it);
			return true;
		}
		it++;
	}
	return false;
}

std::vector<Task> Schedule::get_day(time_t day) {
	std::vector<Task> tasks = std::vector<Task>();

	time_t schedule_day_start = get_day_start(day, start_times);
	time_t schedule_day_end = get_day_end(day, start_times, working_times);

	std::map<time_t, Task>::iterator it = schedule.begin();
	while (it != schedule.end()) {
		if (it->first >= schedule_day_start) {
			break;
		}
		it++;
	}
	if (it == schedule.end() || it->first > schedule_day_end) {
		return tasks;
	}

	while (it != schedule.end()) {
		if (it->first <= schedule_day_end) {
			tasks.push_back(it->second);
		}
		if (it->first > schedule_day_end) {
			return tasks;
		}
		it++;
	}

}

std::vector<Task> Schedule::get_today() {
	time_t now = time(NULL);
	return get_day(now);
}


void ScheduleManager::update(std::list<Task> tasks)
{


	// sort tasks by end date
	tasks.sort([](const Task& task1, const Task& task2) {return compare_date_constraints(task1, task2); });
	std::list<Task>::iterator it = tasks.begin();

	// get all tasks that are due to same day
	time_t last_day_end = get_day_end(it->valid_timeslot.end, schedule.start_times, schedule.working_times);
	std::list<Task> due_same_day;
	while (it != tasks.end()) {
		// collect tasks with same end date
		if (it->valid_timeslot.end <= last_day_end) {
			due_same_day.push_back(*it);
			it++;
		}

		// sort by priority and fill schedule
		// set new last_day_end
		else {
			due_same_day.sort([](const Task& task1, const Task& task2) {return compare_priority(task1, task2); });

			// iterate over days and fill free gaps
			time_t current_day = get_day_start(time(NULL), schedule.start_times);
			while (due_same_day.size() > 0) {

				// TODO: check if tasks should be splitted into subtasks because it is too large for one timeslot
				// could add task to that day
				TIMESLOT result = schedule.add(due_same_day.front(), current_day);
				if (result.start != -1 && result.end != -1) {
					due_same_day.pop_front();
				}

				// try again next day
				else {
					increase_by_day(current_day);
					// task could not be scheduled until due date
					if (current_day > last_day_end) {
						std::cout << "Behaviour not defined. Task could not be scheduled because the schedule until the due date is already full. Implement switching out an existing low priority task." << std::endl;
					}
				}
			}
			last_day_end = get_day_end(it->valid_timeslot.end, schedule.start_times, schedule.working_times);
		}
		
	}
}


