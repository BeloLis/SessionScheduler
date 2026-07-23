#pragma once
#include <vector>
#include <string>
#include "StudentGroup.h"

// Класс для хранения информации о потоке (наборе групп)
class StudentFlow {
public:
    std::string name;                   // Название потока (например, "Поток_1_ПИ")
    std::vector<StudentGroup> groups;    // Группы в потоке

    // Конструктор с названием потока
    explicit StudentFlow(const std::string& name);

    // Добавляет группу в поток
    void addGroup(const StudentGroup& group);

    // Возвращает список групп в потоке
    const std::vector<StudentGroup>& getGroups() const;
};