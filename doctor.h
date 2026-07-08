#ifndef DOCTOR_H
#define DOCTOR_H

#include <QString>
#include <QVector>
#include <unordered_map>

/*
 * Doctor
 * ------
 * Plain data structure representing one doctor record.
 */
struct Doctor {
    int id = 0;
    QString name;
    QString specialization;
    int experience = 0; // years
    QString phone;

    QString toLine() const;
    static Doctor fromLine(const QString &line, bool *ok = nullptr);
};

/*
 * DoctorManager
 * -------------
 * Data structures used:
 *  - unordered_map<int, Doctor>: O(1) average lookup/insert/delete by ID,
 *    and backs the instant Search module for "Doctor ID" lookups.
 *  - QVector<Doctor>: used to return ordered listings to the UI.
 */
class DoctorManager {
public:
    bool addDoctor(const Doctor &d, QString *error = nullptr);
    bool removeDoctor(int id, QString *error = nullptr);
    bool updateDoctor(int id, const Doctor &updated, QString *error = nullptr);

    bool findById(int id, Doctor &out) const;
    QVector<Doctor> findByName(const QString &name) const;
    QVector<Doctor> all() const;

    int count() const { return static_cast<int>(m_doctors.size()); }
    int nextAvailableId() const;

    void clear();
    void loadFromVector(const QVector<Doctor> &doctors);

private:
    std::unordered_map<int, Doctor> m_doctors;
};

#endif // DOCTOR_H
