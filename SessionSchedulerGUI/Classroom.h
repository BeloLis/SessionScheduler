#pragma once
#include <string>

// Тип аудитории
enum class ClassroomType {
    COMPUTER,  // Компьютерная (например, "3-КА")
    REGULAR,    // Обычная (например, "3-ОА")
    UNKNOWN     // Неизвестный тип (если не указан)
};

// Класс для хранения информации об аудитории
class Classroom {
public:
    std::string name;       // Название (например, "3-105")
    ClassroomType type;     // Тип (компьютерная/обычная)
    std::string groupName;  // Группа (например, "3-КА")

    Classroom(const std::string& name, ClassroomType type, const std::string& groupName);
};