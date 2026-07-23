#pragma once
#include <string>

// Класс для хранения информации о группе студентов
class StudentGroup {
public:
    std::string code;  // Код группы (например, "5130904/10101")

    // Конструктор с кодом группы
    explicit StudentGroup(const std::string& code);

    // Возвращает код группы
    const std::string& getCode() const;
};