#ifndef PATIENT_H
#define PATIENT_H

#include <QString>
#include <QVector>
#include <unordered_map>
#include <stack>
#include <memory>

/*
 * Patient
 * -------
 * Plain data structure representing one patient record.
 */
struct Patient {
    int id = 0;
    QString name;
    int age = 0;
    QString gender;
    QString phone;
    QString address;
    QString bloodGroup;
    QString disease;
    QString admissionDate; // stored as yyyy-MM-dd

    QString toLine() const;
    static Patient fromLine(const QString &line, bool *ok = nullptr);
};

/*
 * PatientNode
 * -----------
 * Node of the singly linked list used to store patients in memory.
 * A linked list is used (per project requirement) instead of a plain
 * array/vector so insertion/removal of a record does not require
 * shifting elements, and so traversal logic can be demonstrated.
 */
struct PatientNode {
    Patient data;
    std::unique_ptr<PatientNode> next;
    explicit PatientNode(const Patient &p) : data(p), next(nullptr) {}
};

/*
 * PatientList
 * -----------
 * Custom singly linked list managing Patient records.
 *
 * Data structures used here:
 *  - Linked List: primary storage of patient records (insert/delete/traverse).
 *  - unordered_map<int, PatientNode*>: O(1) average lookup of a patient by ID,
 *    used by the Search module and to enforce "no duplicate IDs".
 *  - std::stack<Patient>: keeps a history of deleted patients so the most
 *    recently deleted one can be restored via "Undo Delete".
 */
class PatientList {
public:
    PatientList() = default;

    bool addPatient(const Patient &p, QString *error = nullptr);
    bool removePatient(int id, QString *error = nullptr);
    bool updatePatient(int id, const Patient &updated, QString *error = nullptr);

    bool findById(int id, Patient &out) const;
    QVector<Patient> findByName(const QString &name) const; // partial, case-insensitive
    QVector<Patient> all() const;

    bool undoDelete(QString *error = nullptr); // pops stack and re-inserts
    bool hasUndo() const { return !m_deletedStack.empty(); }

    int count() const { return static_cast<int>(m_index.size()); }
    int nextAvailableId() const;

    void clear();
    void loadFromVector(const QVector<Patient> &patients); // used by FileManager on startup

private:
    std::unique_ptr<PatientNode> m_head;
    std::unordered_map<int, PatientNode *> m_index; // id -> raw pointer into the list
    std::stack<Patient> m_deletedStack;

    void rebuildIndex();
};

#endif // PATIENT_H
