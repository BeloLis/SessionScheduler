#include "Teacher.h"
#include <algorithm>

// Конструктор по умолчанию
Teacher::Teacher() : name("") {}

// Конструктор с именем преподавателя
Teacher::Teacher(const std::string& name) : name(name) {}

// Добавляет недоступный день недели
void Teacher::addUnavailableDay(const std::string& day) {
    unavailableDays.push_back(day);
}

// Добавляет диапазон недоступных дат
void Teacher::addDateRange(const std::string& start, const std::string& end) {
    dateRanges.emplace_back(start, end);
}