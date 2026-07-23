#pragma once
#include <string>
#include <vector>
#include <memory>
#include "SessionDay.h"
#include "Building.h"
#include "StudentFlow.h"
#include "Exam.h"
#include "GradedExam.h"
#include "FinalExam.h"
#include "ScheduleEntry.h"

// Класс для чтения данных из Excel-файлов и записи результатов
class ExcelReader {
public:
    // Конструктор по умолчанию
    ExcelReader();

    // Деструктор
    ~ExcelReader();

    // Чтение списка дней сессии из файла
    // @param filePath Путь к Excel-файлу
    // @return Вектор объектов SessionDay
    std::vector<SessionDay> readSessionDays(const std::string& filePath);

    // Чтение списка зданий и аудиторий из файла
    // @param filePath Путь к Excel-файлу
    // @return Вектор объектов Building
    std::vector<Building> readBuildings(const std::string& filePath);

    // Чтение списка потоков студентов из файла
    // @param filePath Путь к Excel-файлу
    // @return Вектор объектов StudentFlow
    std::vector<StudentFlow> readFlows(const std::string& filePath);

    // Чтение списка зачётов и курсовых работ из файла
    // @param filePath Путь к Excel-файлу
    // @return Вектор объектов Exam
    std::vector<Exam> readExams(const std::string& filePath);

    // Чтение списка зачёрков и курсовых проектов из файла
    // @param filePath Путь к Excel-файлу
    // @return Вектор объектов GradedExam
    std::vector<GradedExam> readGradedExams(const std::string& filePath);

    // Чтение списка экзаменов из файла
    // @param filePath Путь к Excel-файлу
    // @return Вектор объектов FinalExam
    std::vector<FinalExam> readFinalExams(const std::string& filePath);

    // Чтение существующего расписания из файла
    // @param filePath Путь к Excel-файлу
    // @return Вектор объектов ScheduleEntry
    std::vector<ScheduleEntry> readExistingSchedule(const std::string& filePath);

    // Запись распределённых аттестаций в лист "Распределенные_аттестации"
    // @param inputFilePath Путь к входному Excel-файлу
    // @param outputFilePath Путь к выходному Excel-файлу
    // @param schedule Вектор записей расписания
    void writeDistributedExams(const std::string& inputFilePath, const std::string& outputFilePath,
        const std::vector<ScheduleEntry>& schedule);

    // Запись несоставленных задач в лист "Несоставленные_аттестации"
    // @param inputFilePath Путь к входному Excel-файлу
    // @param outputFilePath Путь к выходному Excel-файлу
    // @param unscheduledTasks Вектор пар (запись, причина)
    void writeUnscheduledTasks(const std::string& inputFilePath, const std::string& outputFilePath,
        const std::vector<std::pair<ScheduleEntry, std::string>>& unscheduledTasks);

    // Запись всех результатов (расписание и несоставленные задачи)
    // @param inputFilePath Путь к входному Excel-файлу
    // @param outputFilePath Путь к выходному Excel-файлу
    // @param schedule Вектор записей расписания
    // @param unscheduledTasks Вектор пар (запись, причина)
    void writeAllResults(const std::string& inputFilePath, const std::string& outputFilePath,
        const std::vector<ScheduleEntry>& schedule,
        const std::vector<std::pair<ScheduleEntry, std::string>>& unscheduledTasks);

private:
    // Внутренний класс реализации (Pimpl идиома)
    class Impl;

    // Указатель на реализацию
    std::unique_ptr<Impl> impl_;
};