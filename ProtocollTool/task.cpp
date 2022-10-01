#include "task.h"
#include "conversions.h"

#include <time.h>

int Task::get_value() {
	return -1;
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

	tm* schedule_day = localtime(&date);
	remove_time(schedule_day);
	time_t schedule_day_start = mktime(schedule_day);
	schedule_day_start += start_times.at(static_cast<WEEKDAY>(schedule_day->tm_wday));
	time_t schedule_day_end = schedule_day_start + static_cast<time_t>(working_times.at(static_cast<WEEKDAY>(schedule_day->tm_wday)));

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

	tm schedule_day = get_localtime(day);
	remove_time(&schedule_day);
	time_t schedule_day_start = mktime(&schedule_day);
	schedule_day_start += start_times.at(static_cast<WEEKDAY>(schedule_day.tm_wday));
	time_t schedule_day_end = schedule_day_start + static_cast<time_t>(working_times.at(static_cast<WEEKDAY>(schedule_day.tm_wday)));

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