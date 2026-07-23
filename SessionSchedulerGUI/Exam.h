#pragma once
#include <string>
#include <vector>
#include "Teacher.h"
#include "TimePreference.h"

enum class ExamType {
    CREDIT,    // З (Зачет)
    COURSEWORK // КР (Курсовая работа)
};

class Exam {
public:
    std::string group;           // Группа
    std::string subject;         // Дисциплина
    Teacher teacher;             // Преподаватель
    std::vector<std::string> preferredRooms; // Предпочтительные аудитории
    ExamType type;               // Тип аттестации
    TimePreference timePref;     // Временные предпочтения
    std::vector<std::string> excludedDates; // Исключенные даты
    bool isFixed;                // Фиксированная аттестация (нельзя менять)

    Exam(const std::string& group, const std::string& subject,
        const Teacher& teacher, const std::vector<std::string>& rooms,
        ExamType type, const TimePreference& timePref,
        const std::vector<std::string>& excludedDates);
};

