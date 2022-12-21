#include "pch.h"
#include <Shlwapi.h>

#include "../ProtocollTool/conversions.h"
#include "../ProtocollTool/utils.h"
#include "../ProtocollTool/log.h"
#include "../ProtocollTool/Config.h"
#include "../ProtocollTool/trietree.h"
#include "../ProtocollTool/autocomplete.h"
#include "../ProtocollTool/enums.h"
//#include <boost/date_time/gregorian/gregorian.hpp>

#include "../ProtocollTool/conversions.cpp"
#include "../ProtocollTool/log.cpp"
#include "../ProtocollTool/file_manager.cpp"
#include "../ProtocollTool/utils.cpp"
#include "../ProtocollTool/Config.cpp"
#include "../ProtocollTool/trietree.cpp"
#include "../ProtocollTool/autocomplete.cpp"

#include <string>




TEST(Conversions, str2int) {
	std::string s1 = { "Hallo Welt!" };
	std::string s2 = { "132" };
	std::string s3 = { "0132" };
	int i1, i2, i3;
	CONV_ERROR ret1, ret2, ret3;

	ret1 = str2int(i1, s1.c_str());
	ret2 = str2int(i2, s2.c_str());
	ret3 = str2int(i3, s3.c_str());

	EXPECT_EQ(ret1, CONV_INCONVERTIBLE);
	EXPECT_EQ(ret2, CONV_SUCCESS);
	EXPECT_EQ(ret3, CONV_SUCCESS);
	EXPECT_EQ(i2, 132);
	EXPECT_EQ(i3, 132);
}

TEST(Conversions, str2float) {
	std::string s1 = { "Hallo Welt!" };
	std::string s2 = { "132.2" };
	float i1, i2;
	CONV_ERROR ret1, ret2;

	ret1 = str2float(i1, s1.c_str());
	ret2 = str2float(i2, s2.c_str());

	EXPECT_EQ(ret1, CONV_INCONVERTIBLE);
	EXPECT_EQ(ret2, CONV_SUCCESS);
	EXPECT_FLOAT_EQ(i2, 132.2);
}

TEST(Conversions, str2date) {
	std::string s1 = { "Hallo Welt!" };
	std::string s2 = { "24" };
	std::string s3 = { "24.1" };
	std::string s4 = { "24.1.2021" };
	time_t i1, i2, i3, i4;
	CONV_ERROR ret1, ret2, ret3, ret4;

	ret1 = str2date(i1, s1.c_str());
	ret2 = str2date(i2, s2.c_str());
	ret3 = str2date(i3, s3.c_str());
	ret4 = str2date(i4, s4.c_str());

	time_t t1, t2, t3, t4;
	tm td1, td2, td3, td4;
	t1 = std::time(NULL);
	localtime_s(&td1, &t1);
	boost::gregorian::date today = boost::gregorian::date_from_tm(td1);

	boost::gregorian::date d2, d3, d4;

	d2 = boost::gregorian::date(today.year(), today.month(), 24);
	d3 = boost::gregorian::date(today.year(), 1, 24);
	d4 = boost::gregorian::date(2021, 1, 24);

	td2 = boost::gregorian::to_tm(d2);
	td3 = boost::gregorian::to_tm(d3);
	td4 = boost::gregorian::to_tm(d4);

	t2 = mktime(&td2);
	t3 = mktime(&td3);
	t4 = mktime(&td4);

	

	EXPECT_EQ(ret1, CONV_INCONVERTIBLE);
	EXPECT_EQ(ret2, CONV_SUCCESS);
	EXPECT_EQ(ret3, CONV_SUCCESS);
	EXPECT_EQ(ret4, CONV_SUCCESS);
	EXPECT_EQ(i2, t2);
	EXPECT_EQ(i3, t3);
	EXPECT_EQ(i4, t4);
}

TEST(Conversions, date2str)
{
	std::string s4 = { "24.1.2021" };
	std::string i1, i2, i3, i4;
	CONV_ERROR ret1, ret2, ret3, ret4;


	time_t t1, t2, t3, t4;
	tm td1, td2, td3, td4;
	
	boost::gregorian::date d1, d2, d3, d4;
	d1 = boost::gregorian::date(2022, 1, 24);
	d2 = boost::gregorian::date(2022, 3, 1);
	d3 = boost::gregorian::date(2022, 12, 24);
	d4 = boost::gregorian::date(2021, 1, 24);

	td1 = boost::gregorian::to_tm(d1);
	td2 = boost::gregorian::to_tm(d2);
	td3 = boost::gregorian::to_tm(d3);
	td4 = boost::gregorian::to_tm(d4);

	t1 = mktime(&td1);
	t2 = mktime(&td2);
	t3 = mktime(&td3);
	t4 = mktime(&td4);

	ret1 = date2str(i1, t1);
	ret2 = date2str(i2, t2);
	ret3 = date2str(i3, t3);
	ret4 = date2str(i4, t4);

	EXPECT_EQ(i1, "Mon, 24.01.2022");
	EXPECT_EQ(i2, "Tue, 01.03.2022");
	EXPECT_EQ(i3, "Sat, 24.12.2022");
	EXPECT_EQ(i4, "Sun, 24.01.2021");
}

TEST(Conversions, date2str_short)
{
	std::string s4 = { "24.1.2021" };
	std::string i1, i2, i3, i4;
	CONV_ERROR ret1, ret2, ret3, ret4;


	time_t t1, t2, t3, t4;
	tm td1, td2, td3, td4;

	boost::gregorian::date d1, d2, d3, d4;
	d1 = boost::gregorian::date(2022, 1, 24);
	d2 = boost::gregorian::date(2022, 3, 1);
	d3 = boost::gregorian::date(2022, 12, 24);
	d4 = boost::gregorian::date(2021, 1, 24);

	td1 = boost::gregorian::to_tm(d1);
	td2 = boost::gregorian::to_tm(d2);
	td3 = boost::gregorian::to_tm(d3);
	td4 = boost::gregorian::to_tm(d4);

	t1 = mktime(&td1);
	t2 = mktime(&td2);
	t3 = mktime(&td3);
	t4 = mktime(&td4);

	ret1 = date2str_short(i1, t1);
	ret2 = date2str_short(i2, t2);
	ret3 = date2str_short(i3, t3);
	ret4 = date2str_short(i4, t4);

	EXPECT_EQ(i1, "24012022");
	EXPECT_EQ(i2, "01032022");
	EXPECT_EQ(i3, "24122022");
	EXPECT_EQ(i4, "24012021");
}

TEST(Conversions, str2dateANDstr2date_short) {
	string s1{ "06.06.2021" }, s1b{ "06.6.2021" }, s1c{"6.06.2021"}, s2{"06062021a"};
	time_t d1, d1b, d1c, d2;
	str2date(d1, s1);
	str2date(d1b, s1b);
	str2date(d1c, s1c);
	str2date_short(d2, s2);

	EXPECT_EQ(d1, d2);
	EXPECT_EQ(d1b, d2);
	EXPECT_EQ(d1c, d2);
}
/*
TEST(utils, parse_find_args) 
{
	Log logger("", false);

	string command1 = { "-d 12.1-28.6" };
	string command2 = { "-date 12.1 - 28.6 -rt Dann" };
	string command3 = { "-date 12.1-28.6 -do -ct test" };
	string command4 = { "-date 12.1 to 28.6 -do -cat test laurenz -nt wrapping" };

	istringstream iss1(command1);
	istringstream iss2(command2);
	istringstream iss3(command3);
	istringstream iss4(command4);

	bool do1;
	bool do2;
	bool do3;
	bool do4;

	vector<time_t> da1;
	vector<time_t> da2;
	vector<time_t> da3;
	vector<time_t> da4;

	vector<string> ct1;
	vector<string> ct2;
	vector<string> ct3;
	vector<string> ct4;

	vector<string> cat1;
	vector<string> cat2;
	vector<string> cat3;
	vector<string> cat4;

	vector<string> nt1;
	vector<string> nt2;
	vector<string> nt3;
	vector<string> nt4;

	string re1;
	string re2;
	string re3;
	string re4;
	
	string rec1;
	string rec2;
	string rec3;
	string rec4;

	vector<char> va1;
	vector<char> va2;
	vector<char> va3;
	vector<char> va4;

	parse_find_args(logger, iss1, do1, da1, ct1, cat1, nt1, re1, rec1, va1);
	parse_find_args(logger, iss2, do2, da2, ct2, cat2, nt2, re2, rec2, va2);
	parse_find_args(logger, iss3, do3, da3, ct3, cat3, nt3, re3, rec3, va3);
	parse_find_args(logger, iss4, do4, da4, ct4, cat4, nt4, re4, rec4, va4);

	EXPECT_FALSE(do1);
	EXPECT_FALSE(do2);
	EXPECT_TRUE(do3);
	EXPECT_TRUE(do4);

	EXPECT_EQ(ct1.size(), 0);
	EXPECT_EQ(ct2.size(), 0);
	EXPECT_EQ(ct3.size(), 1);
	EXPECT_EQ(ct3.at(0), "test");
	EXPECT_EQ(ct4.size(), 0);

	EXPECT_EQ(cat1.size(), 0);
	EXPECT_EQ(cat2.size(), 0);
	EXPECT_EQ(cat3.size(), 0);
	EXPECT_EQ(cat4.size(), 2);
	EXPECT_EQ(cat4.at(0), "test");
	EXPECT_EQ(cat4.at(1), "laurenz");

	EXPECT_EQ(nt1.size(), 0);
	EXPECT_EQ(nt2.size(), 0);
	EXPECT_EQ(nt3.size(), 0);
	EXPECT_EQ(nt4.size(), 1);
	EXPECT_EQ(nt4.at(0), "wrapping");

	EXPECT_EQ(re1, "");
	EXPECT_EQ(re2, "Dann");
	EXPECT_EQ(re3, "");
	EXPECT_EQ(re4, "");

	// TODO test version args
}
*/
TEST(triertree, creation) {
	TrieTree trt1 = TrieTree();
	TrieTree trt2({ "test", "testen", "ganz", "anders" });
}

TEST(triertree, insertion) {
	TrieTree trt1 = TrieTree();
	trt1.insert("test");
	trt1.insert("testen");
	trt1.insert("ganz");
	trt1.insert("anders");
}

TEST(triertree, tree_structure) {
	TrieTree trt1 = TrieTree();
	trt1.insert("test");

	string s1;
	trt1.findAutoSuggestion("t", s1);
	EXPECT_EQ(s1, "est");

	string s2;
	trt1.insert("testen");
	trt1.findAutoSuggestion("t", s2);
	EXPECT_EQ(s2, "est");

	string s3;
	trt1.insert("ganz");
	trt1.findAutoSuggestion("ges", s3);
	EXPECT_EQ(s3, "");

	string s4;
	int ret = trt1.findAutoSuggestion("test", s4);
	EXPECT_EQ(s4, "en");
}

TEST(triertree, tree_structure2) {
	TrieTree trt1 = TrieTree();
	trt1.insert("test");

	list<string> s1;
	list<string> e1 {"est"};
	trt1.findAutoSuggestions("t", s1);
	EXPECT_EQ(s1, e1);

	list<string> s2;
	list<string> e2{ "est", "esten" };
	trt1.insert("testen");
	trt1.findAutoSuggestions("t", s2);
	EXPECT_EQ(s2, e2);

	list<string> s3;
	trt1.insert("ganz");
	trt1.findAutoSuggestions("ges", s3);
	EXPECT_EQ(s3, list<string>());

	list<string> s4;
	list<string> e4{ "", "en" };
	int ret = trt1.findAutoSuggestions("test", s4);
	EXPECT_EQ(s4, e4);
}

TEST(utils, read_cmd_names)
{
	CMD_NAMES cmd_names;
	read_cmd_names(filesystem::path("D:\\Code\\C++\\VisualStudioProjects\\ProtocollTool\\ProtocollTool\\cmd_names.dat"), cmd_names);
	EXPECT_EQ(cmd_names.cmd_names.right.at(CMD::NEW), "new");
	// TODO for all parameter
}

TEST(utils, read_command_structure_pa)
{
	CMD_STRUCTURE cmd_structure;
	read_cmd_structure(filesystem::path("D:\\Code\\C++\\VisualStudioProjects\\ProtocollTool\\ProtocollTool\\cmd.dat"), cmd_structure);

	// check pa
	EXPECT_EQ(cmd_structure[CMD::NEW].first.front(), PA::DATE);
	EXPECT_EQ(*next(cmd_structure[CMD::NEW].first.begin()), PA::TAGS);
	// TODO for all parameter
}

TEST(utils, read_command_structure_oa)
{
	CMD_STRUCTURE cmd_structure;
	read_cmd_structure(filesystem::path("D:\\Code\\C++\\VisualStudioProjects\\ProtocollTool\\ProtocollTool\\cmd.dat"), cmd_structure);

	// check oa
	EXPECT_TRUE(cmd_structure[CMD::NEW].second.contains(OA::DATA));
	// TODO for all parameter
}
TEST(utils, read_command_structure_oaoa)
{
	CMD_STRUCTURE cmd_structure;
	read_cmd_structure(filesystem::path("D:\\Code\\C++\\VisualStudioProjects\\ProtocollTool\\ProtocollTool\\cmd.dat"), cmd_structure);

	// check oa
	EXPECT_EQ(cmd_structure[CMD::NEW].second.at(OA::DATA).size(), 0);
	EXPECT_EQ(cmd_structure[CMD::FIND].second.at(OA::VERS_R), list<OA> {OA::VERSIONS});
	EXPECT_EQ(cmd_structure[CMD::FILTER].second.at(OA::REGT), list<OA> {OA::REGTEXT});
}

TEST(utils, find_cmd_suggestion) {
	COMMAND_INPUT auto_input;
	list<string> tags{"Tag1", "TaestTag2"};
	list<string> mode_names{"mode1", "mode2", "fancy_mode"};
	read_cmd_names(filesystem::path("D:\\Code\\C++\\VisualStudioProjects\\ProtocollTool\\ProtocollTool\\cmd_names.dat"), auto_input.cmd_names);
	read_cmd_structure(filesystem::path("D:\\Code\\C++\\VisualStudioProjects\\ProtocollTool\\ProtocollTool\\cmd.dat"), auto_input.cmd_structure);
	AUTOCOMPLETE auto_comp(auto_input.cmd_names, tags, mode_names);

	auto_input.input =  "fin";
	AUTO_SUGGESTIONS auto_suggestions = AUTO_SUGGESTIONS();
	find_cmd_suggestion(auto_input, auto_comp, auto_suggestions);
	EXPECT_EQ(auto_suggestions.auto_sug, "d");

	auto_input.input = "find -d" ;
	auto_suggestions = AUTO_SUGGESTIONS();
	find_cmd_suggestion(auto_input, auto_comp, auto_suggestions);
	EXPECT_EQ(auto_suggestions.auto_sug, "at");

	auto_input.input = "new t ";
	auto_suggestions = AUTO_SUGGESTIONS();
	find_cmd_suggestion(auto_input, auto_comp, auto_suggestions);
	EXPECT_EQ(auto_suggestions.auto_sug, "ta");

	auto_input.input = "new t -";
	auto_suggestions = AUTO_SUGGESTIONS();
	find_cmd_suggestion(auto_input, auto_comp, auto_suggestions);
	EXPECT_EQ(auto_suggestions.auto_sug, "data");

	auto_input.input = "edit_mode m";
	auto_suggestions = AUTO_SUGGESTIONS();
	find_cmd_suggestion(auto_input, auto_comp, auto_suggestions);
	EXPECT_EQ(auto_suggestions.auto_sug, "ode");
}
