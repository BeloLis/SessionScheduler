#include "GradedExam.h"

// Конструктор
GradedExam::GradedExam(const std::string& groupOrFlow, const std::string& subject,
    const Teacher& teacher, const std::vector<std::string>& rooms,
    GradedExamType type, const TimePreference& timePref,
    const std::vector<std::string>& excludedDates)
    : groupOrFlow(groupOrFlow), subject(subject), teacher(teacher),
    preferredRooms(rooms), type(type), timePref(timePref),
    excludedDates(excludedDates), isFixed(false) {
}