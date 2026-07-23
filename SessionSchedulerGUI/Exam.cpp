#include "Exam.h"

// Конструктор
Exam::Exam(const std::string& group, const std::string& subject,
    const Teacher& teacher, const std::vector<std::string>& rooms,
    ExamType type, const TimePreference& timePref,
    const std::vector<std::string>& excludedDates)
    : group(group), subject(subject), teacher(teacher),
    preferredRooms(rooms), type(type), timePref(timePref),
    excludedDates(excludedDates), isFixed(false) {
}