#include "StudentGroup.h"

// Конструктор с кодом группы
StudentGroup::StudentGroup(const std::string& code) : code(code) {}

// Возвращает код группы
const std::string& StudentGroup::getCode() const {
    return code;
}