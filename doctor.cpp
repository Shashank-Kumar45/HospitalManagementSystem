#include "doctor.h"
#include <QStringList>
#include <algorithm>

QString Doctor::toLine() const
{
    auto esc = [](QString s) { return s.replace('|', '/'); };
    return QString("%1|%2|%3|%4|%5")
        .arg(id)
        .arg(esc(name))
        .arg(esc(specialization))
        .arg(experience)
        .arg(esc(phone));
}

Doctor Doctor::fromLine(const QString &line, bool *ok)
{
    Doctor d;
    QStringList parts = line.split('|');
    if (parts.size() < 5) {
        if (ok) *ok = false;
        return d;
    }
    bool idOk = false, expOk = false;
    d.id = parts[0].toInt(&idOk);
    d.name = parts[1];
    d.specialization = parts[2];
    d.experience = parts[3].toInt(&expOk);
    d.phone = parts[4];
    if (ok) *ok = idOk && expOk;
    return d;
}

bool DoctorManager::addDoctor(const Doctor &d, QString *error)
{
    if (m_doctors.find(d.id) != m_doctors.end()) {
        if (error) *error = QString("A doctor with ID %1 already exists.").arg(d.id);
        return false;
    }
    m_doctors[d.id] = d;
    return true;
}

bool DoctorManager::removeDoctor(int id, QString *error)
{
    auto it = m_doctors.find(id);
    if (it == m_doctors.end()) {
        if (error) *error = QString("Doctor with ID %1 was not found.").arg(id);
        return false;
    }
    m_doctors.erase(it);
    return true;
}

bool DoctorManager::updateDoctor(int id, const Doctor &updated, QString *error)
{
    auto it = m_doctors.find(id);
    if (it == m_doctors.end()) {
        if (error) *error = QString("Doctor with ID %1 was not found.").arg(id);
        return false;
    }
    if (updated.id != id && m_doctors.find(updated.id) != m_doctors.end()) {
        if (error) *error = QString("A doctor with ID %1 already exists.").arg(updated.id);
        return false;
    }
    m_doctors.erase(it);
    m_doctors[updated.id] = updated;
    return true;
}

bool DoctorManager::findById(int id, Doctor &out) const
{
    auto it = m_doctors.find(id);
    if (it == m_doctors.end()) return false;
    out = it->second;
    return true;
}

QVector<Doctor> DoctorManager::findByName(const QString &name) const
{
    QVector<Doctor> result;
    const QString needle = name.trimmed().toLower();
    for (const auto &kv : m_doctors) {
        if (kv.second.name.toLower().contains(needle)) result.push_back(kv.second);
    }
    return result;
}

QVector<Doctor> DoctorManager::all() const
{
    QVector<Doctor> result;
    for (const auto &kv : m_doctors) result.push_back(kv.second);
    std::sort(result.begin(), result.end(), [](const Doctor &a, const Doctor &b) {
        return a.id < b.id;
    });
    return result;
}

int DoctorManager::nextAvailableId() const
{
    int maxId = 0;
    for (const auto &kv : m_doctors) maxId = std::max(maxId, kv.first);
    return maxId + 1;
}

void DoctorManager::clear()
{
    m_doctors.clear();
}

void DoctorManager::loadFromVector(const QVector<Doctor> &doctors)
{
    clear();
    for (const auto &d : doctors) addDoctor(d);
}
