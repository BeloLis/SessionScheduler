#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include "Classroom.h"

#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include "Classroom.h"

// Класс, представляющий учебное здание
class Building {
public:
    // Конструктор для создания здания с указанным кодом
    // @param building_code Код здания (например, "11-КА")
    Building(const std::string& building_code);

    // Добавление аудитории в здание
    // @param classroom_name Название аудитории
    // @param type Тип аудитории (обычная или компьютерная)
    // @param group_name Название группы, связанной с аудиторией
    void addClassroom(const std::string& classroom_name, ClassroomType type, const std::string& group_name);

    // Получение списка аудиторий в здании
    // @return Вектор объектов Classroom
    const std::vector<Classroom>& getClassrooms() const;

    // Получение кода здания
    // @return Код здания (например, "11-КА")
    std::string getCode() const { return building_code; }

    // Получение типа аудиторий в здании
    // @return Нормализованный тип (например, "COMP")
    std::string getType() const { return room_type; }

    // Получение номера здания
    // @return Номер здания (например, "11")
    std::string getBuildingNumber() const { return building_number; }

private:
    // Нормализация типа аудитории
    // @param type Входной тип аудитории
    // @return Нормализованный тип (например, "COMP" для "КА")
    std::string normalizeType(const std::string& type) const;

    std::string building_code;    // Код здания (например, "11-КА")
    std::string building_number;  // Номер здания (например, "11")
    std::string room_type;        // Нормализованный тип аудиторий (например, "COMP")
    std::vector<Classroom> classrooms; // Список аудиторий в здании
};