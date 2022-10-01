#pragma once
#include <stdlib.h>
#include <string>
#include <list>
#include <map>
#include <vector>

enum class PRIORITY { LOW, NORMAL, HIGH };
enum class DEMAND { LOW , NORMAL, HIGH };
enum class DATE_TYPE { ANYTIME, BEFORE, AT, AFTER };
enum class RANK_TYPE { ANYWHERE, BEFORE, AFTER };
enum class WEEKDAY { SUNDAY, MONDAY, TUESDAY, WEDNESDAY, THURSDAY, FRIDAY, SATURDAY };
enum class TASK_COLLITION { NONE, MOVE_NEW, MOVE_OLD};
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

struct TIMESLOT {
	time_t start;
	time_t end;
};

class Task {
public:
	Task() : id(-1), name(""), duration(0), progress(0), priority(PRIORITY::NORMAL), demand(DEMAND::NORMAL), date_constraints(std::list<DATE_CONSTRAINT>()), rank_constraints(std::list<RANK_CONSTRAINT>()) {};
	Task(int id, std::string name, time_t duration, time_t progress, PRIORITY priority, DEMAND demand, std::list<DATE_CONSTRAINT> date_constraints, std::list<RANK_CONSTRAINT> rank_constraints) : id(id), name(name), duration(duration), progress(progress), priority(priority), demand(demand), date_constraints(date_constraints), rank_constraints(rank_constraints) {};
	bool is_in_date_window(time_t start, time_t end) {};
	int get_weight() { return int((duration) * DEMAND_WEIGHT[demand]); };
	int get_value() {};
	int id;
	std::string name ;
	time_t duration;
	time_t progress;
	PRIORITY priority;
	DEMAND demand;
	std::list<DATE_CONSTRAINT> date_constraints;
	std::list<RANK_CONSTRAINT> rank_constraints;


};

class Schedule {
public:
	Schedule() : schedule(std::map<time_t, Task>()), working_times(std::map<WEEKDAY, int>()), start_times(std::map<WEEKDAY, time_t>()) {};
	TIMESLOT can_add(time_t duration, time_t day); // carefull! day is not == time_t which will be added! Use next_free_timeslot for getting the right time_t
	bool timeslot_is_free(TIMESLOT timeslot);
	TIMESLOT add(Task task, time_t day); // carefull! day is not == time_t which will be added! Use next_free_timeslot for getting the right time_t
	TIMESLOT add(Task task, TIMESLOT timeslot);
	bool remove(int task_id);
	void set_working_hours(std::map<WEEKDAY, int> working_hours) { this->working_times = working_hours; };
	void set_start_times(std::map<WEEKDAY, time_t> start_times) { this->start_times = start_times; }
	std::vector<Task> get_past();
	std::vector<Task> get_day(time_t day);
	std::vector<Task> get_today();
	std::vector<std::vector<Task>> get_week();
	std::vector<std::vector<Task>> get_month();
private:
	TIMESLOT next_free_timeslot(time_t date); // iterate over map and find next "free" keys
	std::map<time_t, Task> schedule;
	std::map<WEEKDAY, int> working_times; // in seconds
	std::map<WEEKDAY, time_t> start_times;
};

class ScheduleManager {
public:
	ScheduleManager(Schedule schedule, std::list<Task> tasks);
	void update(std::list<Task> tasks);
};