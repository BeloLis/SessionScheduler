#pragma once
#include <string>
#include <vector>
#include "Teacher.h"
#include "TimePreference.h"

// Класс для хранения информации об экзамене
class FinalExam {
public:
    std::string groupOrFlow;     // Группа или поток
    std::string subject;         // Дисциплина
    Teacher teacher;             // Преподаватель
    std::string examDuration;    // Продолжительность (например, "9+25")
    std::vector<std::string> preferredRooms;  // Предпочтительные аудитории
    std::vector<std::string> excludedDates;   // Недоступные даты
    TimePreference timePref;     // Временные предпочтения
    bool isFixed;                // Фиксированная аттестация (нельзя менять)
    bool consultationFixed;      // Консультация уже зафиксирована в листе "Итог" (не планировать заново)

    // Конструктор
    FinalExam(const std::string& groupOrFlow, const std::string& subject,
        const Teacher& teacher, const std::string& duration,
        const std::vector<std::string>& rooms,
        const std::vector<std::string>& dates,
        const TimePreference& timePref);
};