#pragma once
#include <string>
#include <vector>

// Класс для хранения информации о преподавателе
class Teacher {
public:
    std::string name;           // ФИО преподавателя
    std::vector<std::string> unavailableDays;  // Дни недели, когда преподаватель недоступен
    std::vector<std::pair<std::string, std::string>> dateRanges; // Диапазоны дат недоступности

    // Конструктор по умолчанию
    Teacher();

    // Конструктор с именем преподавателя
    explicit Teacher(const std::string& name);

    // Добавляет недоступный день недели
    void addUnavailableDay(const std::string& day);

    // Добавляет диапазон недоступных дат
    void addDateRange(const std::string& start, const std::string& end);
};