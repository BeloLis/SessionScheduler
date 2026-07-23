#pragma once
#include <string>
#include <vector>
#include "Teacher.h"
#include "TimePreference.h"

// Тип зачета или курсового проекта
enum class GradedExamType {
    GRADED_CREDIT,  // ЗаО (Зачет с оценкой)
    COURSE_PROJECT   // КП (Курсовой проект)
};

// Класс для хранения информации о зачете с оценкой или курсовом проекте
class GradedExam {
public:
    std::string groupOrFlow;     // Группа или название потока
    std::string subject;         // Дисциплина
    Teacher teacher;             // Преподаватель
    std::vector<std::string> preferredRooms;  // Предпочтительные аудитории
    GradedExamType type;         // Тип аттестации
    TimePreference timePref;     // Временные предпочтения
    std::vector<std::string> excludedDates;  // Недоступные даты
    bool isFixed;                // Фиксированная аттестация (нельзя менять)

    // Конструктор
    GradedExam(const std::string& groupOrFlow, const std::string& subject,
        const Teacher& teacher, const std::vector<std::string>& rooms,
        GradedExamType type, const TimePreference& timePref,
        const std::vector<std::string>& excludedDates);
};