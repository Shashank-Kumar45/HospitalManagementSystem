#include "patient.h"
#include <QStringList>
#include <algorithm>

QString Patient::toLine() const
{
    // Pipe-delimited record. Pipes inside fields are not expected from the
    // validated UI, but we still escape them defensively.
    auto esc = [](QString s) { return s.replace('|', '/'); };
    return QString("%1|%2|%3|%4|%5|%6|%7|%8|%9")
        .arg(id)
        .arg(esc(name))
        .arg(age)
        .arg(esc(gender))
        .arg(esc(phone))
        .arg(esc(address))
        .arg(esc(bloodGroup))
        .arg(esc(disease))
        .arg(esc(admissionDate));
}

Patient Patient::fromLine(const QString &line, bool *ok)
{
    Patient p;
    QStringList parts = line.split('|');
    if (parts.size() < 9) {
        if (ok) *ok = false;
        return p;
    }
    bool idOk = false, ageOk = false;
    p.id = parts[0].toInt(&idOk);
    p.name = parts[1];
    p.age = parts[2].toInt(&ageOk);
    p.gender = parts[3];
    p.phone = parts[4];
    p.address = parts[5];
    p.bloodGroup = parts[6];
    p.disease = parts[7];
    p.admissionDate = parts[8];
    if (ok) *ok = idOk && ageOk;
    return p;
}

bool PatientList::addPatient(const Patient &p, QString *error)
{
    if (m_index.find(p.id) != m_index.end()) {
        if (error) *error = QString("A patient with ID %1 already exists.").arg(p.id);
        return false;
    }
    auto node = std::make_unique<PatientNode>(p);
    PatientNode *raw = node.get();

    if (!m_head) {
        m_head = std::move(node);
    } else {
        PatientNode *cur = m_head.get();
        while (cur->next) cur = cur->next.get();
        cur->next = std::move(node);
    }
    m_index[p.id] = raw;
    return true;
}

bool PatientList::removePatient(int id, QString *error)
{
    PatientNode *prev = nullptr;
    PatientNode *cur = m_head.get();
    while (cur) {
        if (cur->data.id == id) {
            m_deletedStack.push(cur->data); // remember for undo
            std::unique_ptr<PatientNode> ownedCur;
            if (prev) {
                ownedCur = std::move(prev->next);
                prev->next = std::move(cur->next);
            } else {
                ownedCur = std::move(m_head);
                m_head = std::move(cur->next);
            }
            m_index.erase(id);
            return true;
        }
        prev = cur;
        cur = cur->next.get();
    }
    if (error) *error = QString("Patient with ID %1 was not found.").arg(id);
    return false;
}

bool PatientList::updatePatient(int id, const Patient &updated, QString *error)
{
    auto it = m_index.find(id);
    if (it == m_index.end()) {
        if (error) *error = QString("Patient with ID %1 was not found.").arg(id);
        return false;
    }
    if (updated.id != id && m_index.find(updated.id) != m_index.end()) {
        if (error) *error = QString("A patient with ID %1 already exists.").arg(updated.id);
        return false;
    }
    PatientNode *node = it->second;
    node->data = updated;
    if (updated.id != id) {
        m_index.erase(it);
        m_index[updated.id] = node;
    }
    return true;
}

bool PatientList::findById(int id, Patient &out) const
{
    auto it = m_index.find(id);
    if (it == m_index.end()) return false;
    out = it->second->data;
    return true;
}

QVector<Patient> PatientList::findByName(const QString &name) const
{
    QVector<Patient> result;
    PatientNode *cur = m_head.get();
    const QString needle = name.trimmed().toLower();
    while (cur) {
        if (cur->data.name.toLower().contains(needle)) {
            result.push_back(cur->data);
        }
        cur = cur->next.get();
    }
    return result;
}

QVector<Patient> PatientList::all() const
{
    QVector<Patient> result;
    PatientNode *cur = m_head.get();
    while (cur) {
        result.push_back(cur->data);
        cur = cur->next.get();
    }
    return result;
}

bool PatientList::undoDelete(QString *error)
{
    if (m_deletedStack.empty()) {
        if (error) *error = "There is nothing to undo.";
        return false;
    }
    Patient p = m_deletedStack.top();
    m_deletedStack.pop();
    QString err;
    if (!addPatient(p, &err)) {
        // ID got reused in the meantime; restore under a fresh ID instead of losing the record.
        p.id = nextAvailableId();
        addPatient(p);
    }
    return true;
}

int PatientList::nextAvailableId() const
{
    int maxId = 0;
    for (const auto &kv : m_index) maxId = std::max(maxId, kv.first);
    return maxId + 1;
}

void PatientList::clear()
{
    m_head.reset();
    m_index.clear();
    while (!m_deletedStack.empty()) m_deletedStack.pop();
}

void PatientList::loadFromVector(const QVector<Patient> &patients)
{
    clear();
    for (const auto &p : patients) addPatient(p);
}
