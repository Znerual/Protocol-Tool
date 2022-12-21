#pragma once

#include <stdlib.h>
#include <string>
#include <list>
#include <map>
#include <vector>
#include "conversions.h"
#include "enums.h"

struct TIMESLOT {
	time_t start = -1;
	time_t end = -1;
};
/**
* Check if child is in parent
*/
bool in_timeslot(TIMESLOT child, TIMESLOT parent) {
	return (child.start >= parent.start && child.end <= parent.end);
}

time_t get_day_start(time_t day, std::map<WEEKDAY, time_t> start_times) {
	tm schedule_day = get_localtime(day);
	remove_time(&schedule_day);
	time_t schedule_day_start = mktime(&schedule_day);
	schedule_day_start += start_times.at(static_cast<WEEKDAY>(schedule_day.tm_wday));
	return schedule_day_start;
}

time_t get_day_end(time_t day, std::map<WEEKDAY, time_t> start_times, std::map<WEEKDAY, int> working_times) {
	tm schedule_day = get_localtime(day);
	time_t day_start = get_day_start(day, start_times);
	return day_start + static_cast<time_t>(working_times.at(static_cast<WEEKDAY>(schedule_day.tm_wday)));
}

void increase_by_day(time_t& day) {
	day += 60 * 60 * 24;
}

std::map<DEMAND, float> DEMAND_WEIGHT{
	{DEMAND::LOW, 0.5},
	{DEMAND::NORMAL, 1},
	{DEMAND::HIGH, 1.5}
};

struct DATE_CONSTRAINT {
	time_t date;
	DATE_TYPE date_type;
};



struct RANK_CONSTRAINT {
	int task_id;
	RANK_TYPE ranke_type;
};



class Task {
public:
	Task() : id(-1), name(""), duration(0), progress(0), priority(PRIORITY::NORMAL), demand(DEMAND::NORMAL), date_constraints(), rank_constraints(), max_split_to_subtasks(0) {};
	Task(int id, std::string name, time_t duration, time_t progress, PRIORITY priority, DEMAND demand, std::list<DATE_CONSTRAINT> date_constraints, int max_split_to_subtasks, std::list<RANK_CONSTRAINT> rank_constraints) : id(id), name(name), duration(duration), progress(progress), priority(priority), demand(demand), date_constraints(date_constraints),max_split_to_subtasks(max_split_to_subtasks), rank_constraints(rank_constraints) { valid_timeslot = get_valid_timeslot(); };
	bool is_in_date_window(time_t start, time_t end) {};
	int get_weight() { return int((duration) * DEMAND_WEIGHT[demand]); };
	int get_value() { return -1; };
	int id;
	void set_date_constraints(std::list<DATE_CONSTRAINT> date_constraints) { this->date_constraints = date_constraints; get_valid_timeslot(); }
	std::list<DATE_CONSTRAINT> get_date_constraints() { return date_constraints; };
	std::string name ;
	time_t duration;
	time_t progress;
	PRIORITY priority;
	DEMAND demand;
	TIMESLOT valid_timeslot;
	int max_split_to_subtasks;
	std::list<RANK_CONSTRAINT> rank_constraints;
private:
	TIMESLOT get_valid_timeslot();
	std::list<DATE_CONSTRAINT> date_constraints;

};

class Schedule {
public:
	Schedule() : schedule(std::map<time_t, Task>()), working_times(), start_times() {};
	Schedule(std::map<WEEKDAY, int> working_times, std::map<WEEKDAY, time_t> start_times) : schedule(), working_times(working_times), start_times(start_times) {};
	TIMESLOT can_add(time_t duration, time_t day); // carefull! day is not == time_t which will be added! Use next_free_timeslot for getting the right time_t
	bool timeslot_is_free(TIMESLOT timeslot);
	TIMESLOT add(Task task, time_t day); // carefull! day is not == time_t which will be added! Use next_free_timeslot for getting the right time_t
	TIMESLOT add(Task task, TIMESLOT timeslot);
	bool remove(int task_id);
	std::vector<Task> get_past();
	std::vector<Task> get_day(time_t day);
	std::vector<Task> get_today();
	std::vector<std::vector<Task>> get_week();
	std::vector<std::vector<Task>> get_month();
	std::map<WEEKDAY, int> working_times; // in seconds
	std::map<WEEKDAY, time_t> start_times;
private:
	TIMESLOT next_free_timeslot(time_t date); // iterate over map and find next "free" keys
	std::map<time_t, Task> schedule;
	
};

class ScheduleManager {
public:
	ScheduleManager(Schedule schedule, std::list<Task> tasks) : schedule(schedule) { update(tasks); };
	void update(std::list<Task> tasks);
	Schedule schedule;
};

bool compare_date_constraints(const Task& task1, const Task& task2) {
	return (task1.valid_timeslot.end < task2.valid_timeslot.end);
}

bool compare_priority(const Task& task1, const Task& task2) {
	return (task1.priority < task2.priority);
}