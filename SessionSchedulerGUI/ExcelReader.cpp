#include "ExcelReader.h"
#include "SessionDay.h"
#include "Building.h"
#include "StudentFlow.h"
#include "Exam.h"
#include "GradedExam.h"
#include "FinalExam.h"
#include "Teacher.h"
#include "TimePreference.h"
#include "ScheduleEntry.h"
#include "Utils.h"
#include <xlnt/xlnt.hpp>
#include <algorithm>
#include <sstream>
#include <ctime>
#include <regex>
#include <filesystem>
#include <iostream>
#include <fstream>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

using namespace std;

namespace {
    bool fileExistsUtf8(const string& utf8Path) {
        if (utf8Path.empty()) return false;
        int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8Path.c_str(), -1, nullptr, 0);
        if (wlen <= 0) return false;
        wstring wpath(static_cast<size_t>(wlen), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, utf8Path.c_str(), -1, &wpath[0], wlen);
        if (!wpath.empty() && wpath.back() == L'\0') wpath.pop_back();
        DWORD attrs = GetFileAttributesW(wpath.c_str());
        return (attrs != INVALID_FILE_ATTRIBUTES) && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
    }

    void fileRemoveUtf8(const string& utf8Path) {
        if (utf8Path.empty()) return;
        int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8Path.c_str(), -1, nullptr, 0);
        if (wlen <= 0) return;
        wstring wpath(static_cast<size_t>(wlen), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, utf8Path.c_str(), -1, &wpath[0], wlen);
        if (!wpath.empty() && wpath.back() == L'\0') wpath.pop_back();
        DeleteFileW(wpath.c_str());
    }

    string trim(const string& str) {
        const auto first = str.find_first_not_of(" \t");
        if (string::npos == first) return "";
        const auto last = str.find_last_not_of(" \t");
        string result = str.substr(first, (last - first + 1));
        transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }

    bool isHeader(const string& value) {
        const vector<string> headerKeywords = {
            "группа", "дисциплина", "преподаватель", "аудитор", "тип",
            "дата", "время", "пожелание", "не ставить", "предпочтение", "дни"
        };

        string lowerStr = value;
        transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);

        for (const auto& keyword : headerKeywords) {
            if (lowerStr.find(keyword) != string::npos) {
                return true;
            }
        }
        return false;
    }

    vector<string> split(const string& s, char delimiter) {
        vector<string> tokens;
        string token;
        istringstream tokenStream(s);
        while (getline(tokenStream, token, delimiter)) {
            token = trim(token);
            if (!token.empty()) {
                tokens.push_back(token);
            }
        }
        return tokens;
    }

    string excelDateToString(double excelDate) {
        time_t rawtime = (time_t)((excelDate - 25569) * 86400);
        struct tm timeinfo;
        localtime_s(&timeinfo, &rawtime);
        char buffer[11];
        strftime(buffer, sizeof(buffer), "%d.%m.%Y", &timeinfo);
        return buffer;
    }

    bool isValidDateFormat(const string& dateStr) {
        regex dateRegex(R"((\d{2})\.(\d{2})\.(\d{4}))");
        return regex_match(dateStr, dateRegex);
    }

    time_t parseDateToTimeT(const string& dateStr) {
        tm tm = {};
        istringstream ss(dateStr);
        int day, month, year;
        char dot;
        ss >> day >> dot >> month >> dot >> year;
        tm.tm_year = year - 1900;
        tm.tm_mon = month - 1;
        tm.tm_mday = day;
        tm.tm_hour = 12; // Устанавливаем время на 12:00, чтобы избежать пограничных эффектов часового пояса
        tm.tm_min = 0;
        tm.tm_sec = 0;
        tm.tm_isdst = -1; // Отключаем автоматическую коррекцию летнего времени
        return mktime(&tm);
    }

    int getWeekday(const string& dateStr) {
        time_t time = parseDateToTimeT(dateStr);
        struct tm tm;
        localtime_s(&tm, &time);
        return tm.tm_wday;
    }

    vector<string> parseExcludedDates(const string& dateStr, const vector<SessionDay>& sessionDays) {
        vector<string> excludedDates;
        if (dateStr.empty()) {
            ofstream log("excel_log.txt", ios::app);
            log << "parseExcludedDates: Входная строка пуста\n";
            log.close();
            return excludedDates;
        }

        ofstream log("excel_log.txt", ios::app);
        log << "parseExcludedDates: Входная строка='" << dateStr << "'\n";

        auto parts = split(dateStr, ',');
        log << "parseExcludedDates: Разделено на " << parts.size() << " частей: ";
        for (const auto& part : parts) {
            log << "'" << part << "', ";
        }
        log << "\n";

        vector<string> weekdays = {
            "воскресенье", "понедельник", "вторник", "среда", "четверг", "пятница", "суббота"
        };

        for (auto& part : parts) {
            log << "parseExcludedDates: Обработка части='" << part << "'\n";
            auto dashPos = part.find('-');
            if (dashPos != string::npos) {
                string startStr = trim(part.substr(0, dashPos));
                string endStr = trim(part.substr(dashPos + 1));
                log << "parseExcludedDates: Диапазон дат: Начало='" << startStr << "', Конец='" << endStr << "'\n";

                if (!isValidDateFormat(startStr) || !isValidDateFormat(endStr)) {
                    log << "parseExcludedDates: Некорректный формат дат: Начало='" << startStr << "', Конец='" << endStr << "'\n";
                    continue;
                }

                time_t startTime = parseDateToTimeT(startStr);
                time_t endTime = parseDateToTimeT(endStr);
                log << "parseExcludedDates: Преобразовано: startTime=" << startTime << ", endTime=" << endTime << "\n";

                for (time_t t = startTime; t <= endTime; t += 86400) {
                    struct tm tm;
                    localtime_s(&tm, &t);
                    char buffer[11];
                    strftime(buffer, sizeof(buffer), "%d.%m.%Y", &tm);
                    excludedDates.push_back(buffer);
                    log << "parseExcludedDates: Добавлена дата в диапазоне: '" << buffer << "'\n";
                }
            }
            else {
                string lowerPart = part;
                transform(lowerPart.begin(), lowerPart.end(), lowerPart.begin(), ::tolower);
                auto weekdayIt = find(weekdays.begin(), weekdays.end(), lowerPart);
                if (weekdayIt != weekdays.end()) {
                    int targetWeekday = distance(weekdays.begin(), weekdayIt);
                    log << "parseExcludedDates: Найден день недели: '" << lowerPart << "', индекс=" << targetWeekday << "\n";
                    for (const auto& day : sessionDays) {
                        int weekday = getWeekday(day.date);
                        if (weekday == targetWeekday) {
                            excludedDates.push_back(day.date);
                            log << "parseExcludedDates: Добавлена дата для дня недели: '" << day.date << "'\n";
                        }
                    }
                }
                else {
                    if (isValidDateFormat(part)) {
                        excludedDates.push_back(part);
                        log << "parseExcludedDates: Добавлена одиночная дата: '" << part << "'\n";
                    }
                    else {
                        log << "parseExcludedDates: Некорректная дата или формат: '" << part << "'\n";
                    }
                }
            }
        }

        sort(excludedDates.begin(), excludedDates.end());
        excludedDates.erase(unique(excludedDates.begin(), excludedDates.end()), excludedDates.end());
        log << "parseExcludedDates: Итоговые исключенные даты: ";
        for (const auto& date : excludedDates) {
            log << "'" << date << "', ";
        }
        log << "\n";
        log.close();
        return excludedDates;
    }

    double parseTimeToFraction(const string& timeStr) {
        try {
            return stod(timeStr) / 24.0;
        }
        catch (...) {
            regex timeRegex(R"((\d{1,2}):(\d{2}))");
            smatch match;
            if (regex_match(timeStr, match, timeRegex)) {
                int hours = stoi(match[1]);
                int minutes = stoi(match[2]);
                return (hours + minutes / 60.0) / 24.0;
            }
            return 0.0;
        }
    }
}

class ExcelReader::Impl {
public:
    vector<SessionDay> readSessionDays(const string& filePath) {
        vector<SessionDay> days;
        try {
            xlnt::workbook wb;
            wb.load(filePath);
            if (!wb.contains("Дни сессии")) {
                ofstream log("excel_log.txt", ios::app);
                log << "readSessionDays: Лист 'Дни сессии' не найден\n";
                log.close();
                return days;
            }
            auto ws = wb.sheet_by_title("Дни сессии");

            bool headerPassed = false;
            for (auto row : ws.rows(false)) {
                if (row.empty()) continue;

                string dateStr = trim(row[0].to_string());
                if (dateStr.empty()) continue;

                ofstream log("excel_log.txt", ios::app);
                log << "readSessionDays: Сырые данные ячейки: '" << row[0].to_string() << "'\n";

                if (!headerPassed && isHeader(dateStr)) {
                    headerPassed = true;
                    log << "readSessionDays: Пропущен заголовок: '" << dateStr << "'\n";
                    log.close();
                    continue;
                }

                if (row[0].data_type() == xlnt::cell_type::number) {
                    try {
                        double excelDate = row[0].value<double>();
                        dateStr = excelDateToString(excelDate);
                        log << "readSessionDays: Числовой формат Excel, преобразовано в: '" << dateStr << "'\n";
                    }
                    catch (const std::exception& e) {
                        log << "readSessionDays: Ошибка преобразования числовой даты: " << e.what() << "\n";
                        log.close();
                        continue;
                    }
                }
                else {
                    if (!isValidDateFormat(dateStr)) {
                        log << "readSessionDays: Некорректный формат даты: '" << dateStr << "'\n";
                        log.close();
                        continue;
                    }
                    log << "readSessionDays: Строковый формат даты: '" << dateStr << "'\n";
                }

                bool isHoliday = false;
                if (row.length() > 1 && row[1].has_value()) {
                    isHoliday = trim(row[1].to_string()) == "*";
                }

                days.emplace_back(dateStr, isHoliday);
                log << "readSessionDays: Добавлена дата: Дата='" << dateStr << "', Праздник=" << (isHoliday ? "Да" : "Нет") << "\n";
                log.close();
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Ошибка при чтении дней сессии: " << e.what() << std::endl;
            ofstream log("excel_log.txt", ios::app);
            log << "readSessionDays: Ошибка при чтении дней сессии: " << e.what() << "\n";
            log.close();
        }
        return days;
    }

    vector<Building> readBuildings(const string& filePath) {
        vector<Building> buildings;
        try {
            xlnt::workbook wb;
            wb.load(filePath);
            auto ws = wb.sheet_by_title("Аудитории");

            vector<string> groupNames;
            bool headerRead = false;
            for (auto row : ws.rows(false)) {
                if (!headerRead) {
                    for (auto cell : row) {
                        string groupName = trim(cell.to_string());
                        if (!groupName.empty() && !isHeader(groupName)) {
                            groupNames.push_back(groupName);
                            buildings.emplace_back(groupName);
                        }
                    }
                    headerRead = true;
                    continue;
                }

                for (size_t col = 0; col < groupNames.size() && col < row.length(); ++col) {
                    string classroomName = trim(row[col].to_string());
                    if (classroomName.empty()) continue;

                    ClassroomType type = (groupNames[col] == "3-ОА") ? ClassroomType::REGULAR : ClassroomType::COMPUTER;

                    auto it = find_if(buildings.begin(), buildings.end(),
                        [&groupNames, col](const Building& b) { return b.getCode() == groupNames[col]; });
                    if (it != buildings.end()) {
                        it->addClassroom(classroomName, type, groupNames[col]);
                        ofstream log("excel_log.txt", ios::app);
                        log << "readBuildings: Добавлена аудитория: " << classroomName << ", Группа=" << groupNames[col]
                            << ", Тип=" << (type == ClassroomType::COMPUTER ? "COMPUTER" : "REGULAR") << "\n";
                            log.close();
                    }
                }
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Ошибка при чтении аудиторий: " << e.what() << std::endl;
            ofstream log("excel_log.txt", ios::app);
            log << "readBuildings: Ошибка при чтении аудиторий: " << e.what() << "\n";
            log.close();
        }
        return buildings;
    }

    vector<StudentFlow> readFlows(const string& filePath) {
        vector<StudentFlow> flows;
        try {
            xlnt::workbook wb;
            wb.load(filePath);
            auto ws = wb.sheet_by_title("Поток");

            bool headerPassed = false;
            for (auto row : ws.rows(false)) {
                if (row.empty()) continue;

                string flowName = trim(row[0].to_string());
                if (flowName.empty()) continue;

                if (!headerPassed && isHeader(flowName)) {
                    headerPassed = true;
                    continue;
                }

                auto it = find_if(flows.begin(), flows.end(),
                    [&flowName](const StudentFlow& f) { return f.name == flowName; });
                if (it == flows.end()) {
                    flows.emplace_back(flowName);
                    it = flows.end() - 1;
                }

                if (row.length() > 1) {
                    string groupsStr = trim(row[1].to_string());
                    if (!groupsStr.empty()) {
                        vector<string> groups = split(groupsStr, ',');
                        for (const auto& group : groups) {
                            string trimmedGroup = trim(group);
                            if (!trimmedGroup.empty()) {
                                it->addGroup(StudentGroup(trimmedGroup));
                                ofstream log("excel_log.txt", ios::app);
                                log << "readFlows: Добавлена группа '" << trimmedGroup << "' в поток '" << flowName << "'\n";
                                log.close();
                            }
                        }
                    }
                }
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Ошибка при чтении потоков: " << e.what() << std::endl;
            ofstream log("excel_log.txt", ios::app);
            log << "readFlows: Ошибка при чтении потоков: " << e.what() << "\n";
            log.close();
        }
        return flows;
    }

    void markFixedTasks(vector<Exam>& exams, vector<GradedExam>& gradedExams, vector<FinalExam>& finalExams,
        const vector<ScheduleEntry>& existingSchedule) {
        ofstream log("excel_log.txt", ios::app);
        log << "markFixedTasks: Пометка фиксированных задач на основе листа Итог\n";

        for (const auto& entry : existingSchedule) {
            if (entry.type == "Конс") {
                bool consultFound = false;
                for (auto& exam : finalExams) {
                    if (exam.groupOrFlow == entry.group &&
                        exam.subject == entry.subject &&
                        exam.teacher.name == entry.teacher) {
                        exam.consultationFixed = true;
                        consultFound = true;
                        log << "markFixedTasks: Консультация из Итог зафиксирована (повторно планироваться не будет): Группа/Поток='"
                            << exam.groupOrFlow << "', Дисциплина='" << exam.subject
                            << "', Преподаватель='" << exam.teacher.name << "', Дата='" << entry.date << "'\n";
                    }
                }
                if (!consultFound) {
                    log << "markFixedTasks: Консультация из Итог не сопоставлена ни с одним экзаменом: Группа='" << entry.group
                        << "', Дисциплина='" << entry.subject << "', Преподаватель='" << entry.teacher
                        << "', Дата='" << entry.date << "'\n";
                }
                continue;
            }

            bool found = false;
            bool isExcluded = false;
            string excludedReason;

            // Проверка для Exam (З_КР)
            for (auto& exam : exams) {
                string examType = exam.type == ExamType::CREDIT ? "З" : "КР";
                if (exam.group == entry.group &&
                    exam.subject == entry.subject &&
                    exam.teacher.name == entry.teacher &&
                    examType == entry.type) {
                    // Проверяем, входит ли дата в excludedDates
                    if (std::find(exam.excludedDates.begin(), exam.excludedDates.end(), entry.date) != exam.excludedDates.end()) {
                        isExcluded = true;
                        excludedReason = "Дата '" + entry.date + "' входит в исключённые даты";
                        log << "markFixedTasks: Задача не помечена как фиксированная (З_КР): Группа='" << exam.group
                            << "', Дисциплина='" << exam.subject << "', Преподаватель='" << exam.teacher.name
                            << "', Тип='" << examType << "', Дата='" << entry.date << "', Причина='" << excludedReason << "'\n";
                        continue;
                    }
                    exam.isFixed = true;
                    found = true;
                    log << "markFixedTasks: Помечена фиксированная задача (З_КР): Группа='" << exam.group
                        << "', Дисциплина='" << exam.subject << "', Преподаватель='" << exam.teacher.name
                        << "', Тип='" << examType << "', Дата='" << entry.date << "'\n";
                }
            }

            // Проверка для GradedExam (ЗаО_КП)
            for (auto& exam : gradedExams) {
                string examType = exam.type == GradedExamType::GRADED_CREDIT ? "ЗаО" : "КП";
                if (exam.groupOrFlow == entry.group &&
                    exam.subject == entry.subject &&
                    exam.teacher.name == entry.teacher &&
                    examType == entry.type) {
                    if (std::find(exam.excludedDates.begin(), exam.excludedDates.end(), entry.date) != exam.excludedDates.end()) {
                        isExcluded = true;
                        excludedReason = "Дата '" + entry.date + "' входит в исключённые даты";
                        log << "markFixedTasks: Задача не помечена как фиксированная (ЗаО_КП): Группа/Поток='" << exam.groupOrFlow
                            << "', Дисциплина='" << exam.subject << "', Преподаватель='" << exam.teacher.name
                            << "', Тип='" << examType << "', Дата='" << entry.date << "', Причина='" << excludedReason << "'\n";
                        continue;
                    }
                    exam.isFixed = true;
                    found = true;
                    log << "markFixedTasks: Помечена фиксированная задача (ЗаО_КП): Группа/Поток='" << exam.groupOrFlow
                        << "', Дисциплина='" << exam.subject << "', Преподаватель='" << exam.teacher.name
                        << "', Тип='" << examType << "', Дата='" << entry.date << "'\n";
                }
            }

            // Проверка для FinalExam (Э)
            for (auto& exam : finalExams) {
                if (exam.groupOrFlow == entry.group &&
                    exam.subject == entry.subject &&
                    exam.teacher.name == entry.teacher &&
                    entry.type == "Э") {
                    if (std::find(exam.excludedDates.begin(), exam.excludedDates.end(), entry.date) != exam.excludedDates.end()) {
                        isExcluded = true;
                        excludedReason = "Дата '" + entry.date + "' входит в исключённые даты";
                        log << "markFixedTasks: Задача не помечена как фиксированная (Экзамен): Группа/Поток='" << exam.groupOrFlow
                            << "', Дисциплина='" << exam.subject << "', Преподаватель='" << exam.teacher.name
                            << "', Тип='Э', Дата='" << entry.date << "', Причина='" << excludedReason << "'\n";
                        continue;
                    }
                    exam.isFixed = true;
                    found = true;
                    log << "markFixedTasks: Помечена фиксированная задача (Экзамен): Группа/Поток='" << exam.groupOrFlow
                        << "', Дисциплина='" << exam.subject << "', Преподаватель='" << exam.teacher.name
                        << "', Тип='Э', Дата='" << entry.date << "'\n";
                }
            }

            if (!found) {
                log << "markFixedTasks: Задача из Итог не найдена: Группа='" << entry.group
                    << "', Дисциплина='" << entry.subject << "', Преподаватель='" << entry.teacher
                    << "', Тип='" << entry.type << "', Дата='" << entry.date << "'\n";
            }
            else if (isExcluded) {
                log << "markFixedTasks: Задача исключена из фиксированных: Группа='" << entry.group
                    << "', Дисциплина='" << entry.subject << "', Преподаватель='" << entry.teacher
                    << "', Тип='" << entry.type << "', Дата='" << entry.date << "', Причина='" << excludedReason << "'\n";
            }
        }
        log.close();
    }

    vector<Exam> readExams(const string& filePath) {
        vector<Exam> exams;
        try {
            xlnt::workbook wb;
            wb.load(filePath);
            auto ws = wb.sheet_by_title("З_КР");

            bool headerPassed = false;
            auto sessionDays = readSessionDays(filePath);
            for (auto row : ws.rows(false)) {
                vector<string> rowData;
                for (auto cell : row) {
                    rowData.push_back(trim(cell.to_string()));
                }

                if (rowData.empty()) continue;

                if (!headerPassed && isHeader(rowData[0])) {
                    headerPassed = true;
                    continue;
                }

                if (rowData.size() < 5 || rowData[0].empty()) continue;

                ExamType type = (rowData[4] == "З") ? ExamType::CREDIT : ExamType::COURSEWORK;
                vector<string> classrooms = rowData.size() > 3 ? split(rowData[3], ',') : vector<string>();
                vector<string> excludedDates = rowData.size() > 5 ? parseExcludedDates(rowData[5], sessionDays) : vector<string>();

                ofstream log("excel_log.txt", ios::app);
                log << "readExams: Чтение З_КР: Группа='" << rowData[0] << "', Дисциплина='"
                    << (rowData.size() > 1 ? rowData[1] : "") << "', Преподаватель='"
                    << (rowData.size() > 2 ? rowData[2] : "") << "', Аудитории={";
                for (const auto& room : classrooms) log << room << ",";
                log << "}, Исключенные даты={";
                for (const auto& date : excludedDates) log << date << ",";
                log << "}\n";
                log.close();

                exams.emplace_back(
                    rowData[0],
                    rowData.size() > 1 ? rowData[1] : "",
                    Teacher(rowData.size() > 2 ? rowData[2] : ""),
                    classrooms,
                    type,
                    TimePreference(rowData.size() > 6 ? rowData[6] : ""),
                    excludedDates
                );
            }

            auto existingSchedule = readExistingSchedule(filePath);
            vector<GradedExam> dummyGradedExams;
            vector<FinalExam> dummyFinalExams;
            markFixedTasks(exams, dummyGradedExams, dummyFinalExams, existingSchedule);
        }
        catch (const std::exception& e) {
            std::cerr << "Ошибка при чтении З_КР: " << e.what() << std::endl;
            ofstream log("excel_log.txt", ios::app);
            log << "readExams: Ошибка при чтении З_КР: " << e.what() << "\n";
            log.close();
        }
        return exams;
    }

    vector<GradedExam> readGradedExams(const string& filePath) {
        vector<GradedExam> exams;
        try {
            xlnt::workbook wb;
            wb.load(filePath);
            auto ws = wb.sheet_by_title("ЗаО_КП");

            bool headerPassed = false;
            auto sessionDays = readSessionDays(filePath);
            for (auto row : ws.rows(false)) {
                vector<string> rowData;
                for (auto cell : row) {
                    rowData.push_back(trim(cell.to_string()));
                }

                if (rowData.empty()) continue;

                if (!headerPassed && isHeader(rowData[0])) {
                    headerPassed = true;
                    continue;
                }

                if (rowData.size() < 5 || rowData[0].empty()) continue;

                GradedExamType type = (rowData[4] == "ЗаО") ? GradedExamType::GRADED_CREDIT : GradedExamType::COURSE_PROJECT;
                vector<string> classrooms = rowData.size() > 3 ? split(rowData[3], ',') : vector<string>();
                vector<string> excludedDates = rowData.size() > 5 ? parseExcludedDates(rowData[5], sessionDays) : vector<string>();

                ofstream log("excel_log.txt", ios::app);
                log << "readGradedExams: Чтение ЗаО_КП: Группа/Поток='" << rowData[0] << "', Дисциплина='"
                    << rowData[1] << "', Преподаватель='" << rowData[2] << "', Аудитории={";
                for (const auto& room : classrooms) log << room << ",";
                log << "}, Исключенные даты={";
                for (const auto& date : excludedDates) log << date << ",";
                log << "}\n";
                log.close();

                exams.emplace_back(
                    rowData[0],
                    rowData[1],
                    Teacher(rowData[2]),
                    classrooms,
                    type,
                    TimePreference(rowData.size() > 6 ? rowData[6] : ""),
                    excludedDates
                );
            }

            auto existingSchedule = readExistingSchedule(filePath);
            vector<Exam> dummyExams;
            vector<FinalExam> dummyFinalExams;
            markFixedTasks(dummyExams, exams, dummyFinalExams, existingSchedule);
        }
        catch (const std::exception& e) {
            std::cerr << "Ошибка при чтении ЗаО_КП: " << e.what() << std::endl;
            ofstream log("excel_log.txt", ios::app);
            log << "readGradedExams: Ошибка при чтении ЗаО_КП: " << e.what() << "\n";
            log.close();
        }
        return exams;
    }

    vector<FinalExam> readFinalExams(const string& filePath) {
        vector<FinalExam> exams;
        try {
            xlnt::workbook wb;
            wb.load(filePath);
            auto ws = wb.sheet_by_title("Экзамен");

            bool headerPassed = false;
            auto sessionDays = readSessionDays(filePath);
            for (auto row : ws.rows(false)) {
                vector<string> rowData;
                for (auto cell : row) {
                    rowData.push_back(trim(cell.to_string()));
                }

                if (rowData.empty()) continue;

                if (!headerPassed && isHeader(rowData[0])) {
                    headerPassed = true;
                    continue;
                }

                if (rowData.size() < 4 || rowData[0].empty()) continue;

                vector<string> classrooms = rowData.size() > 4 ? split(rowData[4], ',') : vector<string>();
                vector<string> excludedDates = rowData.size() > 5 ? parseExcludedDates(rowData[5], sessionDays) : vector<string>();

                ofstream log("excel_log.txt", ios::app);
                log << "readFinalExams: Чтение Экзамен: Группа/Поток='" << rowData[0] << "', Дисциплина='"
                    << (rowData.size() > 1 ? rowData[1] : "") << "', Преподаватель='"
                    << (rowData.size() > 2 ? rowData[2] : "") << "', Аудитории={";
                for (const auto& room : classrooms) log << room << ",";
                log << "}, Исключенные даты={";
                for (const auto& date : excludedDates) log << date << ",";
                log << "}\n";
                log.close();

                exams.emplace_back(
                    rowData[0],
                    rowData.size() > 1 ? rowData[1] : "",
                    Teacher(rowData.size() > 2 ? rowData[2] : ""),
                    rowData.size() > 3 ? rowData[3] : "",
                    classrooms,
                    excludedDates,
                    TimePreference(rowData.size() > 6 ? rowData[6] : "")
                );
            }

            auto existingSchedule = readExistingSchedule(filePath);
            vector<Exam> dummyExams;
            vector<GradedExam> dummyGradedExams;
            markFixedTasks(dummyExams, dummyGradedExams, exams, existingSchedule);
        }
        catch (const std::exception& e) {
            std::cerr << "Ошибка при чтении экзаменов: " << e.what() << std::endl;
            ofstream log("excel_log.txt", ios::app);
            log << "readFinalExams: Ошибка при чтении экзаменов: " << e.what() << "\n";
            log.close();
        }
        return exams;
    }

    vector<ScheduleEntry> readExistingSchedule(const string& filePath) {
        vector<ScheduleEntry> schedule;
        try {
            xlnt::workbook wb;
            wb.load(filePath);
            auto ws = wb.sheet_by_title("Итог");

            bool headerPassed = false;
            for (auto row : ws.rows(false)) {
                vector<string> rowData;
                for (auto cell : row) {
                    rowData.push_back(trim(cell.to_string()));
                }

                if (rowData.empty()) continue;

                if (!headerPassed && isHeader(rowData[0])) {
                    headerPassed = true;
                    continue;
                }

                if (rowData.size() < 8) continue;

                if (rowData[0].empty()) continue;

                string dateStr = rowData[3];
                ofstream log("excel_log.txt", ios::app);
                log << "readExistingSchedule: Сырые данные даты: '" << row[3].to_string() << "'\n";

                if (dateStr.empty()) {
                    log << "readExistingSchedule: Пустая дата, строка пропущена\n";
                    log.close();
                    continue;
                }

                if (row[3].data_type() == xlnt::cell_type::number) {
                    try {
                        double excelDate = row[3].value<double>();
                        dateStr = excelDateToString(excelDate);
                        log << "readExistingSchedule: Числовой формат Excel, преобразовано в: '" << dateStr << "'\n";
                    }
                    catch (const std::exception& e) {
                        log << "readExistingSchedule: Ошибка преобразования числовой даты: " << e.what() << "\n";
                        log.close();
                        continue;
                    }
                }
                else if (!isValidDateFormat(dateStr)) {
                    log << "readExistingSchedule: Некорректный формат даты: '" << dateStr << "'\n";
                    log.close();
                    continue;
                }
                else {
                    log << "readExistingSchedule: Строковый формат даты: '" << dateStr << "'\n";
                }
          
                double startTime = 0.0, endTime = 0.0;
                try {
                    double rawStart = (row[4].data_type() == xlnt::cell_type::number)
                        ? row[4].value<double>()
                        : parseTimeToFraction(rowData[4]);
                    double rawEnd = (row[5].data_type() == xlnt::cell_type::number)
                        ? row[5].value<double>()
                        : parseTimeToFraction(rowData[5]);

                    // Если значение >= 1, значит это дата+время, берём только дробную часть (время)
                    startTime = (rawStart >= 1.0) ? std::fmod(rawStart, 1.0) : rawStart;
                    endTime = (rawEnd >= 1.0) ? std::fmod(rawEnd, 1.0) : rawEnd;

                    log << "readExistingSchedule: Время: Начало='" << rowData[4] << "' -> " << Utils::formatTime(startTime)
                        << ", Конец='" << rowData[5] << "' -> " << Utils::formatTime(endTime) << "\n";
                }
                catch (const std::exception& e) {
                    log << "readExistingSchedule: Ошибка преобразования времени: " << e.what() << "\n";
                    log.close();
                    continue;
                }

                ScheduleEntry entry({
                    rowData[0],
                    rowData[1],
                    rowData[2],
                    dateStr,
                    startTime,
                    endTime,
                    rowData[6],
                    rowData[7]
                    });

                schedule.push_back(entry);
                log << "readExistingSchedule: Добавлена запись: Группа='" << entry.group << "', Дисциплина='" << entry.subject
                    << "', Аудитория='" << entry.classroom << "', Дата='" << entry.date
                    << "', Начало=" << Utils::formatTime(entry.startTime)
                    << ", Конец=" << Utils::formatTime(entry.endTime)
                    << ", Тип='" << entry.type << "', Преподаватель='" << entry.teacher << "'\n";
                log.close();
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Ошибка при чтении существующего расписания: " << e.what() << std::endl;
            ofstream log("excel_log.txt", ios::app);
            log << "readExistingSchedule: Ошибка при чтении Итог: " << e.what() << "\n";
            log.close();
        }
        return schedule;
    }

    void writeDistributedExams(const string& inputFilePath, const string& outputFilePath, const vector<ScheduleEntry>& schedule) {
        ofstream log("excel_log.txt", ios::app);
        log << "writeDistributedExams: Запись в лист Распределенные_аттестации: Входной файл='" << inputFilePath << "', Выходной файл='" << outputFilePath << "', Записей=" << schedule.size() << "\n";

        try {
            xlnt::workbook wb;
            if (fileExistsUtf8(inputFilePath)) {
                wb.load(inputFilePath);
                log << "writeDistributedExams: Входной файл успешно загружен\n";
            }
            else {
                log << "writeDistributedExams: Входной файл не существует, создаётся новый\n";
            }

            xlnt::worksheet ws;
            if (wb.contains("Распределенные_аттестации")) {
                wb.remove_sheet(wb.sheet_by_title("Распределенные_аттестации"));
                log << "writeDistributedExams: Существующий лист 'Распределенные_аттестации' удалён\n";
            }
            ws = wb.create_sheet();
            ws.title("Распределенные_аттестации");
            log << "writeDistributedExams: Лист 'Распределенные_аттестации' успешно создан\n";

            ws.cell("A1").value("Группа");
            ws.cell("B1").value("Дисциплина");
            ws.cell("C1").value("Аудитория");
            ws.cell("D1").value("Дата");
            ws.cell("E1").value("Начало");
            ws.cell("F1").value("Конец");
            ws.cell("G1").value("Тип");
            ws.cell("H1").value("Преподаватель");
            ws.cell("I1").value("Пожелания выполнены");

            size_t row = 2;
            for (const auto& entry : schedule) {
                log << "writeDistributedExams: Записываем строку " << row << ": Группа='" << entry.group << "', Дисциплина='"
                    << entry.subject << "', Аудитория='" << entry.classroom << "', Дата='"
                    << entry.date << "', Начало=" << Utils::formatTime(entry.startTime)
                    << ", Конец=" << Utils::formatTime(entry.endTime) << ", Тип='" << entry.type
                    << "', Преподаватель='" << entry.teacher << "'\n";

                ws.cell("A" + to_string(row)).value(entry.group);
                ws.cell("B" + to_string(row)).value(entry.subject);
                ws.cell("C" + to_string(row)).value(entry.classroom);
                ws.cell("D" + to_string(row)).value(entry.date);
                ws.cell("E" + to_string(row)).value(Utils::formatTime(entry.startTime));
                ws.cell("F" + to_string(row)).value(Utils::formatTime(entry.endTime));
                ws.cell("G" + to_string(row)).value(entry.type);
                ws.cell("H" + to_string(row)).value(entry.teacher);
                ws.cell("I" + to_string(row)).value("Да");
                ++row;
            }

            wb.save(outputFilePath);
            log << "writeDistributedExams: Лист Распределенные_аттестации успешно сохранён в " << outputFilePath << "\n";
        }
        catch (const std::exception& e) {
            std::cerr << "Ошибка при записи распределенных аттестаций: " << e.what() << "\n";
            log << "writeDistributedExams: Ошибка при записи в Распределенные_аттестации: " << e.what() << "\n";
            throw;
        }
        log.close();
    }

    void writeUnscheduledTasks(const string& inputFilePath, const string& outputFilePath,
        const vector<pair<ScheduleEntry, string>>& unscheduledTasks) {
        ofstream log("excel_log.txt", ios::app);
        log << "writeUnscheduledTasks: Запись в лист Несоставленные_аттестации: Входной файл='" << inputFilePath << "', Выходной файл='" << outputFilePath << "', Записей=" << unscheduledTasks.size() << "\n";

        try {
            xlnt::workbook wb;
            if (fileExistsUtf8(inputFilePath)) {
                wb.load(inputFilePath);
                log << "writeUnscheduledTasks: Входной файл успешно загружен\n";
            }
            else {
                log << "writeUnscheduledTasks: Входной файл не существует, создаётся новый\n";
            }

            xlnt::worksheet ws;
            if (wb.contains("Несоставленные_аттестации")) {
                wb.remove_sheet(wb.sheet_by_title("Несоставленные_аттестации"));
                log << "writeUnscheduledTasks: Существующий лист 'Несоставленные_аттестации' удалён\n";
            }
            ws = wb.create_sheet();
            ws.title("Несоставленные_аттестации");
            log << "writeUnscheduledTasks: Лист 'Несоставленные_аттестации' успешно создан\n";

            ws.cell("A1").value("Группа");
            ws.cell("B1").value("Дисциплина");
            ws.cell("C1").value("Тип");
            ws.cell("D1").value("Преподаватель");
            ws.cell("E1").value("Причина");

            size_t row = 2;
            for (const auto& task : unscheduledTasks) {
                const auto& entry = task.first;
                const auto& reason = task.second;
                log << "writeUnscheduledTasks: Записываем строку " << row << ": Группа='" << entry.group << "', Дисциплина='"
                    << entry.subject << "', Тип='" << entry.type << "', Преподаватель='"
                    << entry.teacher << "', Причина='" << reason << "'\n";

                ws.cell("A" + to_string(row)).value(entry.group);
                ws.cell("B" + to_string(row)).value(entry.subject);
                ws.cell("C" + to_string(row)).value(entry.type);
                ws.cell("D" + to_string(row)).value(entry.teacher);
                ws.cell("E" + to_string(row)).value(reason);
                ++row;
            }

            if (fileExistsUtf8(outputFilePath)) {
                fileRemoveUtf8(outputFilePath);
                log << "writeUnscheduledTasks: Существующий выходной файл удалён: " << outputFilePath << "\n";
            }

            log << "writeUnscheduledTasks: Попытка сохранения файла...\n";
            wb.save(outputFilePath);
            log << "writeUnscheduledTasks: Лист Несоставленные_аттестации успешно сохранён в " << outputFilePath << "\n";
        }
        catch (const std::exception& e) {
            std::cerr << "Ошибка при записи несоставленных задач: " << e.what() << "\n";
            log << "writeUnscheduledTasks: Ошибка при записи в Несоставленные_аттестации: " << e.what() << "\n";
            throw;
        }
        log.close();
    }

    void writeAllResults(const string& inputFilePath, const string& outputFilePath,
        const vector<ScheduleEntry>& schedule,
        const vector<pair<ScheduleEntry, string>>& unscheduledTasks) {
        ofstream log("excel_log.txt", ios::app);
        log << "writeAllResults: Запись результатов: Входной файл='" << inputFilePath << "', Выходной файл='" << outputFilePath
            << "', Распределено записей=" << schedule.size() << ", Несоставлено записей=" << unscheduledTasks.size() << "\n";
        try {
            xlnt::workbook wb;
            if (fileExistsUtf8(inputFilePath)) {
                wb.load(inputFilePath);
                log << "writeAllResults: Входной файл успешно загружен\n";
            }
            else {
                log << "writeAllResults: Входной файл не существует, создаётся новый\n";
            }

            // Считываем зафиксированные записи из листа "Итог", чтобы перенести их в "Распределенные_аттестации"
            vector<ScheduleEntry> existingSchedule = readExistingSchedule(inputFilePath);

            // --- Запись в лист "Распределенные_аттестации" ---
            xlnt::worksheet ws_distributed;
            if (wb.contains("Распределенные_аттестации")) {
                wb.remove_sheet(wb.sheet_by_title("Распределенные_аттестации"));
                log << "writeAllResults: Существующий лист 'Распределенные_аттестации' удалён\n";
            }
            ws_distributed = wb.create_sheet();
            ws_distributed.title("Распределенные_аттестации");
            log << "writeAllResults: Лист 'Распределенные_аттестации' успешно создан\n";

            ws_distributed.cell("A1").value("Группа");
            ws_distributed.cell("B1").value("Дисциплина");
            ws_distributed.cell("C1").value("Аудитория");
            ws_distributed.cell("D1").value("Дата");
            ws_distributed.cell("E1").value("Начало");
            ws_distributed.cell("F1").value("Конец");
            ws_distributed.cell("G1").value("Тип");
            ws_distributed.cell("H1").value("Преподаватель");
            ws_distributed.cell("I1").value("Пожелания выполнены");

            size_t row = 2;

            // 1. Сначала записываем зафиксированные записи (бывший лист "Итог")
            for (const auto& entry : existingSchedule) {
                log << "writeAllResults: Записываем строку " << row << " в Распределенные_аттестации (из Итог): Группа='" << entry.group
                    << "', Дисциплина='" << entry.subject << "', Аудитория='" << entry.classroom
                    << "', Дата='" << entry.date << "', Начало=" << Utils::formatTime(entry.startTime)
                    << ", Конец=" << Utils::formatTime(entry.endTime) << ", Тип='" << entry.type
                    << "', Преподаватель='" << entry.teacher << "'\n";
                ws_distributed.cell("A" + to_string(row)).value(entry.group);
                ws_distributed.cell("B" + to_string(row)).value(entry.subject);
                ws_distributed.cell("C" + to_string(row)).value(entry.classroom);
                ws_distributed.cell("D" + to_string(row)).value(entry.date);
                ws_distributed.cell("E" + to_string(row)).value(Utils::formatTime(entry.startTime));
                ws_distributed.cell("F" + to_string(row)).value(Utils::formatTime(entry.endTime));
                ws_distributed.cell("G" + to_string(row)).value(entry.type);
                ws_distributed.cell("H" + to_string(row)).value(entry.teacher);
                ws_distributed.cell("I" + to_string(row)).value("Да");
                ++row;
            }

            // 2. Затем записываем новые распределённые записи
            for (const auto& entry : schedule) {
                log << "writeAllResults: Записываем строку " << row << " в Распределенные_аттестации (новые): Группа='" << entry.group
                    << "', Дисциплина='" << entry.subject << "', Аудитория='" << entry.classroom
                    << "', Дата='" << entry.date << "', Начало=" << Utils::formatTime(entry.startTime)
                    << ", Конец=" << Utils::formatTime(entry.endTime) << ", Тип='" << entry.type
                    << "', Преподаватель='" << entry.teacher << "'\n";
                ws_distributed.cell("A" + to_string(row)).value(entry.group);
                ws_distributed.cell("B" + to_string(row)).value(entry.subject);
                ws_distributed.cell("C" + to_string(row)).value(entry.classroom);
                ws_distributed.cell("D" + to_string(row)).value(entry.date);
                ws_distributed.cell("E" + to_string(row)).value(Utils::formatTime(entry.startTime));
                ws_distributed.cell("F" + to_string(row)).value(Utils::formatTime(entry.endTime));
                ws_distributed.cell("G" + to_string(row)).value(entry.type);
                ws_distributed.cell("H" + to_string(row)).value(entry.teacher);
                ws_distributed.cell("I" + to_string(row)).value("Да");
                ++row;
            }

            // --- Запись в лист "Несоставленные_аттестации" ---
            xlnt::worksheet ws_unscheduled;
            if (wb.contains("Несоставленные_аттестации")) {
                wb.remove_sheet(wb.sheet_by_title("Несоставленные_аттестации"));
                log << "writeAllResults: Существующий лист 'Несоставленные_аттестации' удалён\n";
            }
            ws_unscheduled = wb.create_sheet();
            ws_unscheduled.title("Несоставленные_аттестации");
            log << "writeAllResults: Лист 'Несоставленные_аттестации' успешно создан\n";

            ws_unscheduled.cell("A1").value("Группа");
            ws_unscheduled.cell("B1").value("Дисциплина");
            ws_unscheduled.cell("C1").value("Тип");
            ws_unscheduled.cell("D1").value("Преподаватель");
            ws_unscheduled.cell("E1").value("Причина");

            row = 2;
            for (const auto& task : unscheduledTasks) {
                const auto& entry = task.first;
                const auto& reason = task.second;
                log << "writeAllResults: Записываем строку " << row << " в Несоставленные_аттестации: Группа='" << entry.group
                    << "', Дисциплина='" << entry.subject << "', Тип='" << entry.type
                    << "', Преподаватель='" << entry.teacher << "', Причина='" << reason << "'\n";
                ws_unscheduled.cell("A" + to_string(row)).value(entry.group);
                ws_unscheduled.cell("B" + to_string(row)).value(entry.subject);
                ws_unscheduled.cell("C" + to_string(row)).value(entry.type);
                ws_unscheduled.cell("D" + to_string(row)).value(entry.teacher);
                ws_unscheduled.cell("E" + to_string(row)).value(reason);
                ++row;
            }

            wb.save(outputFilePath);
            log << "writeAllResults: Результаты успешно сохранены в " << outputFilePath << "\n";
        }
        catch (const std::exception& e) {
            std::cerr << "Ошибка при записи результатов: " << e.what() << "\n";
            log << "writeAllResults: Ошибка при записи результатов: " << e.what() << "\n";
            throw;
        }
        log.close();
    }
};

ExcelReader::ExcelReader() : impl_(std::make_unique<Impl>()) {}
ExcelReader::~ExcelReader() = default;

vector<SessionDay> ExcelReader::readSessionDays(const string& filePath) {
    return impl_->readSessionDays(filePath);
}

vector<Building> ExcelReader::readBuildings(const string& filePath) {
    return impl_->readBuildings(filePath);
}

vector<StudentFlow> ExcelReader::readFlows(const string& filePath) {
    return impl_->readFlows(filePath);
}

vector<Exam> ExcelReader::readExams(const string& filePath) {
    return impl_->readExams(filePath);
}

vector<GradedExam> ExcelReader::readGradedExams(const string& filePath) {
    return impl_->readGradedExams(filePath);
}

vector<FinalExam> ExcelReader::readFinalExams(const string& filePath) {
    return impl_->readFinalExams(filePath);
}

vector<ScheduleEntry> ExcelReader::readExistingSchedule(const string& filePath) {
    return impl_->readExistingSchedule(filePath);
}

void ExcelReader::writeDistributedExams(const string& inputFilePath, const string& outputFilePath, const vector<ScheduleEntry>& schedule) {
    impl_->writeDistributedExams(inputFilePath, outputFilePath, schedule);
}

void ExcelReader::writeUnscheduledTasks(const string& inputFilePath, const string& outputFilePath,
    const vector<pair<ScheduleEntry, string>>& unscheduledTasks) {
    impl_->writeUnscheduledTasks(inputFilePath, outputFilePath, unscheduledTasks);
}

void ExcelReader::writeAllResults(const string& inputFilePath, const string& outputFilePath,
    const vector<ScheduleEntry>& schedule,
    const vector<pair<ScheduleEntry, string>>& unscheduledTasks) {
    impl_->writeAllResults(inputFilePath, outputFilePath, schedule, unscheduledTasks);
}