#include "FinalExam.h"

// Конструктор
FinalExam::FinalExam(const std::string& groupOrFlow, const std::string& subject,
    const Teacher& teacher, const std::string& duration,
    const std::vector<std::string>& rooms,
    const std::vector<std::string>& dates,
    const TimePreference& timePref)
    : groupOrFlow(groupOrFlow), subject(subject), teacher(teacher),
    examDuration(duration), preferredRooms(rooms), excludedDates(dates),
    timePref(timePref), isFixed(false), consultationFixed(false) {
}