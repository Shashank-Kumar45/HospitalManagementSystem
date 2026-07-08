#include "filemanager.h"
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QFileInfo>

FileManager::FileManager(const QString &baseDir)
{
    m_baseDir = baseDir.isEmpty() ? QDir::currentPath() : baseDir;
}

QString FileManager::path(const QString &fileName) const
{
    return QDir(m_baseDir).filePath(fileName);
}

void FileManager::ensureFileExists(const QString &path)
{
    QFileInfo info(path);
    if (!info.dir().exists()) {
        QDir().mkpath(info.dir().absolutePath());
    }
    if (!QFile::exists(path)) {
        QFile f(path);
        if (f.open(QIODevice::WriteOnly)){
            f.close();
        }
        
    }
}

template <typename T>
static QVector<T> loadLines(const QString &filePath, T (*fromLine)(const QString &, bool *))
{
    QVector<T> result;
    FileManager::ensureFileExists(filePath);
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return result;
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.trimmed().isEmpty()) continue;
        bool ok = false;
        T item = fromLine(line, &ok);
        if (ok) result.push_back(item);
    }
    file.close();
    return result;
}

template <typename T>
static bool saveLines(const QString &filePath, const QVector<T> &items)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) return false;
    QTextStream out(&file);
    for (const auto &item : items) {
        out << item.toLine() << "\n";
    }
    file.close();
    return true;
}

// ----------------------------- Patients -----------------------------
QVector<Patient> FileManager::loadPatients() const
{
    return loadLines<Patient>(path("patients.txt"), &Patient::fromLine);
}

bool FileManager::savePatients(const QVector<Patient> &patients) const
{
    return saveLines<Patient>(path("patients.txt"), patients);
}

// ----------------------------- Doctors -------------------------------
QVector<Doctor> FileManager::loadDoctors() const
{
    return loadLines<Doctor>(path("doctors.txt"), &Doctor::fromLine);
}

bool FileManager::saveDoctors(const QVector<Doctor> &doctors) const
{
    return saveLines<Doctor>(path("doctors.txt"), doctors);
}

// --------------------------- Appointments ----------------------------
QVector<Appointment> FileManager::loadAppointments() const
{
    return loadLines<Appointment>(path("appointments.txt"), &Appointment::fromLine);
}

bool FileManager::saveAppointments(const QVector<Appointment> &appts) const
{
    return saveLines<Appointment>(path("appointments.txt"), appts);
}

// ----------------------------- Emergency -----------------------------
static EmergencyPatient emergencyFromLine(const QString &line, bool *ok)
{
    EmergencyPatient p;
    QStringList parts = line.split('|');
    if (parts.size() < 6) { if (ok) *ok = false; return p; }
    bool idOk = false, ageOk = false, prOk = false;
    p.id = parts[0].toInt(&idOk);
    p.name = parts[1];
    p.age = parts[2].toInt(&ageOk);
    p.gender = parts[3];
    p.condition = parts[4];
    p.priorityLevel = parts[5].toInt(&prOk);
    if (ok) *ok = idOk && ageOk && prOk;
    return p;
}

struct EmergencyLineWrapper {
    EmergencyPatient p;
    QString toLine() const {
        auto esc = [](QString s) { return s.replace('|', '/'); };
        return QString("%1|%2|%3|%4|%5|%6")
            .arg(p.id).arg(esc(p.name)).arg(p.age).arg(esc(p.gender)).arg(esc(p.condition)).arg(p.priorityLevel);
    }
};

QVector<EmergencyPatient> FileManager::loadEmergency() const
{
    QVector<EmergencyPatient> result;
    QString filePath = path("emergency.txt");
    ensureFileExists(filePath);
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return result;
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.trimmed().isEmpty()) continue;
        bool ok = false;
        EmergencyPatient p = emergencyFromLine(line, &ok);
        if (ok) result.push_back(p);
    }
    return result;
}

bool FileManager::saveEmergency(const QVector<EmergencyPatient> &patients) const
{
    QFile file(path("emergency.txt"));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) return false;
    QTextStream out(&file);
    for (const auto &p : patients) {
        EmergencyLineWrapper w{p};
        out << w.toLine() << "\n";
    }
    return true;
}

// ------------------------------- Users --------------------------------
QVector<UserAccount> FileManager::loadUsers() const
{
    QVector<UserAccount> result;
    QString filePath = path("users.txt");
    ensureFileExists(filePath);
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return result;
    QTextStream in(&file);
    bool any = false;
    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.trimmed().isEmpty()) continue;
        QStringList parts = line.split('|');
        if (parts.size() < 2) continue;
        UserAccount u;
        u.username = parts[0];
        u.password = parts[1];
        u.fullName = parts.size() > 2 ? parts[2] : parts[0];
        result.push_back(u);
        any = true;
    }
    file.close();

    if (!any) {
        // Safety net: file existed but was empty/corrupt - recreate the default admin account.
        UserAccount admin{"admin", "admin123", "Administrator"};
        result.push_back(admin);
        saveUsers(result);
    }
    return result;
}

bool FileManager::saveUsers(const QVector<UserAccount> &users) const
{
    QFile file(path("users.txt"));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) return false;
    QTextStream out(&file);
    for (const auto &u : users) {
        out << u.username << "|" << u.password << "|" << u.fullName << "\n";
    }
    return true;
}

bool FileManager::validateLogin(const QString &username, const QString &password) const
{
    const auto users = loadUsers();
    for (const auto &u : users) {
        if (u.username == username && u.password == password) return true;
    }
    return false;
}

// ------------------------------ Settings -------------------------------
QString FileManager::loadTheme() const
{
    QString filePath = path("settings.txt");
    ensureFileExists(filePath);
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return "light";
    QTextStream in(&file);
    QString theme = in.readLine().trimmed();
    file.close();
    return theme.isEmpty() ? "light" : theme;
}

bool FileManager::saveTheme(const QString &theme) const
{
    QFile file(path("settings.txt"));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) return false;
    QTextStream out(&file);
    out << theme << "\n";
    return true;
}
