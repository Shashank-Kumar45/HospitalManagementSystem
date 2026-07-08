#include "mainwindow.h"
#include "login.h"

#include <QApplication>
#include <QListWidget>
#include <QStackedWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QDateEdit>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QMessageBox>
#include <QInputDialog>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QFile>
#include <QTextStream>
#include <QFileDialog>
#include <QPrinter>
#include <QPrintDialog>
#include <QPainter>
#include <QRegularExpression>
#include <QDate>
#include <QDateTime>
#include <QMap>
#include <QStyle>
#include <QCloseEvent>

MainWindow::MainWindow(FileManager *fileManager, const QString &loggedInUser, QWidget *parent)
    : QMainWindow(parent), m_fileManager(fileManager), m_loggedInUser(loggedInUser)
{
    setWindowTitle("Hospital Management System - Dashboard");
    resize(1200, 760);

    m_currentTheme = m_fileManager->loadTheme();
    loadAllData();
    buildUi();
    buildMenuToolStatusBars();
    applyTheme(m_currentTheme);

    refreshPatientTable(m_patients.all());
    refreshDoctorTable(m_doctors.all());
    refreshAppointmentTable();
    refreshEmergencyTable();
    refreshReports();

    setStatusMessage(QString("Welcome, %1!").arg(loggedInUser), 5000);
}

MainWindow::~MainWindow()
{
}

void MainWindow::loadAllData()
{
    m_patients.loadFromVector(m_fileManager->loadPatients());
    m_doctors.loadFromVector(m_fileManager->loadDoctors());
    m_appointments.loadFromVector(m_fileManager->loadAppointments());
    m_emergency.loadFromVector(m_fileManager->loadEmergency());
    rebuildPatientSearchIndex();
}

void MainWindow::persistPatients() { m_fileManager->savePatients(m_patients.all()); }
void MainWindow::persistDoctors() { m_fileManager->saveDoctors(m_doctors.all()); }
void MainWindow::persistAppointments() { m_fileManager->saveAppointments(m_appointments.all()); }
void MainWindow::persistEmergency() { m_fileManager->saveEmergency(m_emergency.snapshotSortedByPriority()); }

void MainWindow::setStatusMessage(const QString &msg, int timeoutMs)
{
    statusBar()->showMessage(msg, timeoutMs);
}

// ============================================================ UI SHELL ====
void MainWindow::buildUi()
{
    auto *central = new QWidget(this);
    auto *mainLayout = new QHBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_navPanel = new QListWidget(central);
    m_navPanel->setObjectName("navPanel");
    m_navPanel->setFixedWidth(220);
    m_navPanel->addItem("🧑‍⚕️  Patient Management");
    m_navPanel->addItem("🩺  Doctor Management");
    m_navPanel->addItem("📅  Appointments");
    m_navPanel->addItem("🚑  Emergency Queue");
    m_navPanel->addItem("📊  Reports");
    m_navPanel->addItem("🔍  Search");
    m_navPanel->addItem("⚙️  Settings");
    m_navPanel->addItem("🚪  Logout");
    connect(m_navPanel, &QListWidget::currentItemChanged, this, &MainWindow::onNavChanged);

    m_stack = new QStackedWidget(central);
    m_stack->addWidget(buildPatientsPage());
    m_stack->addWidget(buildDoctorsPage());
    m_stack->addWidget(buildAppointmentsPage());
    m_stack->addWidget(buildEmergencyPage());
    m_stack->addWidget(buildReportsPage());
    m_stack->addWidget(buildSearchPage());
    m_stack->addWidget(buildSettingsPage());

    mainLayout->addWidget(m_navPanel);
    mainLayout->addWidget(m_stack, 1);

    setCentralWidget(central);
    m_navPanel->setCurrentRow(0);
}

void MainWindow::buildMenuToolStatusBars()
{
    // Menu bar
    QMenu *fileMenu = menuBar()->addMenu("&File");
    QAction *logoutAction = fileMenu->addAction("Logout");
    connect(logoutAction, &QAction::triggered, this, &MainWindow::onLogout);
    fileMenu->addSeparator();
    QAction *exitAction = fileMenu->addAction("Exit");
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    QMenu *viewMenu = menuBar()->addMenu("&View");
    QAction *themeAction = viewMenu->addAction("Toggle Dark / Light Mode");
    themeAction->setShortcut(QKeySequence("Ctrl+T"));
    connect(themeAction, &QAction::triggered, this, &MainWindow::onToggleTheme);

    QMenu *helpMenu = menuBar()->addMenu("&Help");
    QAction *helpAction = helpMenu->addAction("Help Contents");
    helpAction->setShortcut(QKeySequence("F1"));
    connect(helpAction, &QAction::triggered, this, &MainWindow::onHelp);
    QAction *aboutAction = helpMenu->addAction("About");
    connect(aboutAction, &QAction::triggered, this, &MainWindow::onAbout);

    // Toolbar
    QToolBar *toolBar = addToolBar("Main Toolbar");
    toolBar->setMovable(false);
    QAction *newPatientAction = toolBar->addAction(style()->standardIcon(QStyle::SP_FileIcon), "New Patient");
    newPatientAction->setShortcut(QKeySequence("Ctrl+N"));
    connect(newPatientAction, &QAction::triggered, this, [this]() {
        m_navPanel->setCurrentRow(0);
        onClearPatientForm();
        m_pIdEdit->setFocus();
    });
    QAction *searchAction = toolBar->addAction(style()->standardIcon(QStyle::SP_FileDialogContentsView), "Search");
    searchAction->setShortcut(QKeySequence("Ctrl+F"));
    connect(searchAction, &QAction::triggered, this, [this]() { m_navPanel->setCurrentRow(5); });
    QAction *reportAction = toolBar->addAction(style()->standardIcon(QStyle::SP_FileDialogDetailedView), "Reports");
    connect(reportAction, &QAction::triggered, this, [this]() { m_navPanel->setCurrentRow(4); m_navPanel->setFocus(); refreshReports(); });
    toolBar->addSeparator();
    QAction *themeToolAction = toolBar->addAction(style()->standardIcon(QStyle::SP_DesktopIcon), "Toggle Theme");
    connect(themeToolAction, &QAction::triggered, this, &MainWindow::onToggleTheme);
    QAction *logoutToolAction = toolBar->addAction(style()->standardIcon(QStyle::SP_DialogCloseButton), "Logout");
    connect(logoutToolAction, &QAction::triggered, this, &MainWindow::onLogout);

    // Status bar
    statusBar()->showMessage("Ready");
}

void MainWindow::onNavChanged(QListWidgetItem *current)
{
    if (!current) return;
    int row = m_navPanel->row(current);
    if (row == 7) { // Logout
        onLogout();
        return;
    }
    m_stack->setCurrentIndex(row);
    if (row == 4) refreshReports();
}

void MainWindow::onLogout()
{
    auto reply = QMessageBox::question(this, "Confirm Logout",
                                        "Are you sure you want to logout?",
                                        QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        close();
        FileManager *fm = m_fileManager;
        LoginWindow *login = new LoginWindow(fm);
        if (login->exec() == QDialog::Accepted) {
            MainWindow *w = new MainWindow(fm, login->loggedInUser());
            w->show();
        } else {
            QApplication::quit();
        }
        login->deleteLater();
        this->deleteLater();
    }
}

void MainWindow::applyTheme(const QString &theme)
{
    QFile f(theme == "dark" ? ":/styles_dark.qss" : ":/styles.qss");
    if (f.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream ts(&f);
        qApp->setStyleSheet(ts.readAll());
    }
    m_currentTheme = theme;
}

void MainWindow::onToggleTheme()
{
    QString next = (m_currentTheme == "dark") ? "light" : "dark";
    applyTheme(next);
    m_fileManager->saveTheme(next);
    setStatusMessage(QString("Switched to %1 mode.").arg(next));
}

void MainWindow::onAbout()
{
    QMessageBox::about(this, "About Hospital Management System",
        "<h3>Hospital Management System</h3>"
        "<p>Version 1.0 — Offline Desktop Edition</p>"
        "<p>Built with C++17 and Qt Widgets.</p>"
        "<p>All data is stored locally in plain text files. No internet "
        "connection or external database is required.</p>"
        "<p>&copy; 2026 Hospital Management System Project</p>");
}

void MainWindow::onHelp()
{
    QMessageBox::information(this, "Help",
        "<b>Getting started</b>"
        "<ul>"
        "<li>Use the left navigation panel to switch between modules.</li>"
        "<li>Patient and Doctor records can be added, edited, deleted and searched.</li>"
        "<li>Appointments follow First-Come-First-Serve order.</li>"
        "<li>The Emergency Queue always serves the most critical patient first.</li>"
        "<li>Use the Reports tab to view statistics and export/print a CSV report.</li>"
        "<li>Press Ctrl+T to toggle Dark/Light mode, Ctrl+N for a new patient, Ctrl+F to search.</li>"
        "</ul>");
}

bool MainWindow::isValidPhone(const QString &phone) const
{
    static QRegularExpression re("^[0-9+\\-\\s]{7,15}$");
    return re.match(phone).hasMatch();
}

// ======================================================== PATIENTS PAGE ====
QWidget *MainWindow::buildPatientsPage()
{
    auto *page = new QWidget();
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(20, 16, 20, 16);

    auto *title = new QLabel("Patient Management", page);
    title->setObjectName("pageTitle");
    layout->addWidget(title);

    auto *splitRow = new QHBoxLayout();

    // ---- Form group ----
    auto *formGroup = new QGroupBox("Patient Details", page);
    auto *form = new QFormLayout(formGroup);

    m_pIdEdit = new QLineEdit(formGroup);
    m_pIdEdit->setPlaceholderText(QString::number(m_patients.nextAvailableId()));
    m_pNameEdit = new QLineEdit(formGroup);
    m_pAgeSpin = new QSpinBox(formGroup);
    m_pAgeSpin->setRange(0, 130);
    m_pGenderCombo = new QComboBox(formGroup);
    m_pGenderCombo->addItems({"Male", "Female", "Other"});
    m_pPhoneEdit = new QLineEdit(formGroup);
    m_pPhoneEdit->setPlaceholderText("e.g. 9876543210");
    m_pAddressEdit = new QLineEdit(formGroup);
    m_pBloodCombo = new QComboBox(formGroup);
    m_pBloodCombo->addItems({"A+", "A-", "B+", "B-", "AB+", "AB-", "O+", "O-"});
    m_pDiseaseEdit = new QLineEdit(formGroup);
    m_pAdmissionDate = new QDateEdit(formGroup);
    m_pAdmissionDate->setDate(QDate::currentDate());
    m_pAdmissionDate->setCalendarPopup(true);

    form->addRow("Patient ID:", m_pIdEdit);
    form->addRow("Name:", m_pNameEdit);
    form->addRow("Age:", m_pAgeSpin);
    form->addRow("Gender:", m_pGenderCombo);
    form->addRow("Phone:", m_pPhoneEdit);
    form->addRow("Address:", m_pAddressEdit);
    form->addRow("Blood Group:", m_pBloodCombo);
    form->addRow("Disease:", m_pDiseaseEdit);
    form->addRow("Admission Date:", m_pAdmissionDate);

    auto *btnRow = new QHBoxLayout();
    m_pAddBtn = new QPushButton("Add", formGroup);
    m_pAddBtn->setObjectName("successButton");
    m_pUpdateBtn = new QPushButton("Update", formGroup);
    m_pDeleteBtn = new QPushButton("Delete", formGroup);
    m_pDeleteBtn->setObjectName("dangerButton");
    m_pClearBtn = new QPushButton("Clear", formGroup);
    m_pClearBtn->setObjectName("secondaryButton");
    btnRow->addWidget(m_pAddBtn);
    btnRow->addWidget(m_pUpdateBtn);
    btnRow->addWidget(m_pDeleteBtn);
    btnRow->addWidget(m_pClearBtn);
    form->addRow(btnRow);

    m_pUndoBtn = new QPushButton("Undo Last Delete", formGroup);
    m_pUndoBtn->setObjectName("secondaryButton");
    form->addRow(m_pUndoBtn);

    connect(m_pAddBtn, &QPushButton::clicked, this, &MainWindow::onAddPatient);
    connect(m_pUpdateBtn, &QPushButton::clicked, this, &MainWindow::onUpdatePatient);
    connect(m_pDeleteBtn, &QPushButton::clicked, this, &MainWindow::onDeletePatient);
    connect(m_pClearBtn, &QPushButton::clicked, this, &MainWindow::onClearPatientForm);
    connect(m_pUndoBtn, &QPushButton::clicked, this, &MainWindow::onUndoDeletePatient);

    splitRow->addWidget(formGroup, 0);

    // ---- Table group ----
    auto *tableGroup = new QGroupBox("All Patients", page);
    auto *tableLayout = new QVBoxLayout(tableGroup);

    m_pSearchEdit = new QLineEdit(tableGroup);
    m_pSearchEdit->setPlaceholderText("Search patients by name...");
    connect(m_pSearchEdit, &QLineEdit::textChanged, this, &MainWindow::onPatientSearchTextChanged);
    tableLayout->addWidget(m_pSearchEdit);

    m_patientTable = new QTableWidget(tableGroup);
    m_patientTable->setColumnCount(9);
    m_patientTable->setHorizontalHeaderLabels({"ID", "Name", "Age", "Gender", "Phone", "Address", "Blood Group", "Disease", "Admission Date"});
    m_patientTable->horizontalHeader()->setStretchLastSection(true);
    m_patientTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_patientTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_patientTable->setSortingEnabled(true);
    connect(m_patientTable, &QTableWidget::itemSelectionChanged, this, &MainWindow::onPatientTableSelectionChanged);
    tableLayout->addWidget(m_patientTable);

    splitRow->addWidget(tableGroup, 1);
    layout->addLayout(splitRow);

    return page;
}

bool MainWindow::validatePatientForm(QString *error) const
{
    if (m_pIdEdit->text().trimmed().isEmpty()) { if (error) *error = "Patient ID is required."; return false; }
    bool ok = false;
    m_pIdEdit->text().trimmed().toInt(&ok);
    if (!ok) { if (error) *error = "Patient ID must be a number."; return false; }
    if (m_pNameEdit->text().trimmed().isEmpty()) { if (error) *error = "Name is required."; return false; }
    if (m_pAgeSpin->value() <= 0) { if (error) *error = "Age must be greater than 0."; return false; }
    if (m_pPhoneEdit->text().trimmed().isEmpty()) { if (error) *error = "Phone number is required."; return false; }
    if (!isValidPhone(m_pPhoneEdit->text().trimmed())) { if (error) *error = "Phone number is invalid. Use 7-15 digits."; return false; }
    if (m_pAddressEdit->text().trimmed().isEmpty()) { if (error) *error = "Address is required."; return false; }
    return true;
}

void MainWindow::onAddPatient()
{
    QString err;
    if (!validatePatientForm(&err)) {
        QMessageBox::warning(this, "Validation Error", err);
        return;
    }
    Patient p;
    p.id = m_pIdEdit->text().trimmed().toInt();
    p.name = m_pNameEdit->text().trimmed();
    p.age = m_pAgeSpin->value();
    p.gender = m_pGenderCombo->currentText();
    p.phone = m_pPhoneEdit->text().trimmed();
    p.address = m_pAddressEdit->text().trimmed();
    p.bloodGroup = m_pBloodCombo->currentText();
    p.disease = m_pDiseaseEdit->text().trimmed();
    p.admissionDate = m_pAdmissionDate->date().toString("yyyy-MM-dd");

    if (!m_patients.addPatient(p, &err)) {
        QMessageBox::warning(this, "Could Not Add Patient", err);
        return;
    }
    persistPatients();
    rebuildPatientSearchIndex();
    refreshPatientTable(m_patients.all());
    refreshReports();
    onClearPatientForm();
    setStatusMessage(QString("Patient '%1' added successfully.").arg(p.name));
}

void MainWindow::onUpdatePatient()
{
    if (m_editingPatientId < 0) {
        QMessageBox::information(this, "No Selection", "Please select a patient from the table to update.");
        return;
    }
    QString err;
    if (!validatePatientForm(&err)) {
        QMessageBox::warning(this, "Validation Error", err);
        return;
    }
    Patient p;
    p.id = m_pIdEdit->text().trimmed().toInt();
    p.name = m_pNameEdit->text().trimmed();
    p.age = m_pAgeSpin->value();
    p.gender = m_pGenderCombo->currentText();
    p.phone = m_pPhoneEdit->text().trimmed();
    p.address = m_pAddressEdit->text().trimmed();
    p.bloodGroup = m_pBloodCombo->currentText();
    p.disease = m_pDiseaseEdit->text().trimmed();
    p.admissionDate = m_pAdmissionDate->date().toString("yyyy-MM-dd");

    if (!m_patients.updatePatient(m_editingPatientId, p, &err)) {
        QMessageBox::warning(this, "Could Not Update Patient", err);
        return;
    }
    persistPatients();
    rebuildPatientSearchIndex();
    refreshPatientTable(m_patients.all());
    refreshReports();
    onClearPatientForm();
    setStatusMessage("Patient record updated successfully.");
}

void MainWindow::onDeletePatient()
{
    if (m_editingPatientId < 0) {
        QMessageBox::information(this, "No Selection", "Please select a patient from the table to delete.");
        return;
    }
    auto reply = QMessageBox::question(this, "Confirm Delete",
        QString("Are you sure you want to delete patient ID %1?").arg(m_editingPatientId),
        QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) return;

    QString err;
    if (!m_patients.removePatient(m_editingPatientId, &err)) {
        QMessageBox::warning(this, "Could Not Delete Patient", err);
        return;
    }
    persistPatients();
    rebuildPatientSearchIndex();
    refreshPatientTable(m_patients.all());
    refreshReports();
    onClearPatientForm();
    setStatusMessage("Patient deleted. Use 'Undo Last Delete' to restore.");
}

void MainWindow::onUndoDeletePatient()
{
    QString err;
    if (!m_patients.undoDelete(&err)) {
        QMessageBox::information(this, "Nothing to Undo", err);
        return;
    }
    persistPatients();
    rebuildPatientSearchIndex();
    refreshPatientTable(m_patients.all());
    refreshReports();
    setStatusMessage("Last deleted patient restored.");
}

void MainWindow::onClearPatientForm()
{
    m_pIdEdit->clear();
    m_pIdEdit->setPlaceholderText(QString::number(m_patients.nextAvailableId()));
    m_pNameEdit->clear();
    m_pAgeSpin->setValue(0);
    m_pGenderCombo->setCurrentIndex(0);
    m_pPhoneEdit->clear();
    m_pAddressEdit->clear();
    m_pBloodCombo->setCurrentIndex(0);
    m_pDiseaseEdit->clear();
    m_pAdmissionDate->setDate(QDate::currentDate());
    m_editingPatientId = -1;
    m_patientTable->clearSelection();
    m_pIdEdit->setEnabled(true);
}

void MainWindow::onPatientTableSelectionChanged()
{
    auto selected = m_patientTable->selectionModel()->selectedRows();
    if (selected.isEmpty()) { m_editingPatientId = -1; return; }
    int row = selected.first().row();
    int id = m_patientTable->item(row, 0)->text().toInt();
    Patient p;
    if (!m_patients.findById(id, p)) return;

    m_editingPatientId = id;
    m_pIdEdit->setText(QString::number(p.id));
    m_pIdEdit->setEnabled(false); // ID is the key; edit via delete+re-add if it must change
    m_pNameEdit->setText(p.name);
    m_pAgeSpin->setValue(p.age);
    m_pGenderCombo->setCurrentText(p.gender);
    m_pPhoneEdit->setText(p.phone);
    m_pAddressEdit->setText(p.address);
    m_pBloodCombo->setCurrentText(p.bloodGroup);
    m_pDiseaseEdit->setText(p.disease);
    QDate d = QDate::fromString(p.admissionDate, "yyyy-MM-dd");
    m_pAdmissionDate->setDate(d.isValid() ? d : QDate::currentDate());
}

void MainWindow::onPatientSearchTextChanged(const QString &text)
{
    if (text.trimmed().isEmpty()) {
        refreshPatientTable(m_patients.all());
        return;
    }
    refreshPatientTable(m_patients.findByName(text));
}

void MainWindow::refreshPatientTable(const QVector<Patient> &patients)
{
    m_patientTable->setSortingEnabled(false);
    m_patientTable->setRowCount(0);
    for (const auto &p : patients) {
        int row = m_patientTable->rowCount();
        m_patientTable->insertRow(row);
        m_patientTable->setItem(row, 0, new QTableWidgetItem(QString::number(p.id)));
        m_patientTable->setItem(row, 1, new QTableWidgetItem(p.name));
        m_patientTable->setItem(row, 2, new QTableWidgetItem(QString::number(p.age)));
        m_patientTable->setItem(row, 3, new QTableWidgetItem(p.gender));
        m_patientTable->setItem(row, 4, new QTableWidgetItem(p.phone));
        m_patientTable->setItem(row, 5, new QTableWidgetItem(p.address));
        m_patientTable->setItem(row, 6, new QTableWidgetItem(p.bloodGroup));
        m_patientTable->setItem(row, 7, new QTableWidgetItem(p.disease));
        m_patientTable->setItem(row, 8, new QTableWidgetItem(p.admissionDate));
    }
    m_patientTable->setSortingEnabled(true);
}

void MainWindow::rebuildPatientSearchIndex()
{
    m_searchIndex.clear();
    for (const auto &p : m_patients.all()) {
        m_searchIndex[p.name.toLower()] = p.id;
    }
}

// ======================================================== DOCTORS PAGE =====
QWidget *MainWindow::buildDoctorsPage()
{
    auto *page = new QWidget();
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(20, 16, 20, 16);

    auto *title = new QLabel("Doctor Management", page);
    title->setObjectName("pageTitle");
    layout->addWidget(title);

    auto *splitRow = new QHBoxLayout();

    auto *formGroup = new QGroupBox("Doctor Details", page);
    auto *form = new QFormLayout(formGroup);

    m_dIdEdit = new QLineEdit(formGroup);
    m_dIdEdit->setPlaceholderText(QString::number(m_doctors.nextAvailableId()));
    m_dNameEdit = new QLineEdit(formGroup);
    m_dSpecializationEdit = new QLineEdit(formGroup);
    m_dExperienceSpin = new QSpinBox(formGroup);
    m_dExperienceSpin->setRange(0, 60);
    m_dPhoneEdit = new QLineEdit(formGroup);

    form->addRow("Doctor ID:", m_dIdEdit);
    form->addRow("Name:", m_dNameEdit);
    form->addRow("Specialization:", m_dSpecializationEdit);
    form->addRow("Experience (yrs):", m_dExperienceSpin);
    form->addRow("Phone:", m_dPhoneEdit);

    auto *btnRow = new QHBoxLayout();
    m_dAddBtn = new QPushButton("Add", formGroup);
    m_dAddBtn->setObjectName("successButton");
    m_dUpdateBtn = new QPushButton("Update", formGroup);
    m_dDeleteBtn = new QPushButton("Delete", formGroup);
    m_dDeleteBtn->setObjectName("dangerButton");
    m_dClearBtn = new QPushButton("Clear", formGroup);
    m_dClearBtn->setObjectName("secondaryButton");
    btnRow->addWidget(m_dAddBtn);
    btnRow->addWidget(m_dUpdateBtn);
    btnRow->addWidget(m_dDeleteBtn);
    btnRow->addWidget(m_dClearBtn);
    form->addRow(btnRow);

    connect(m_dAddBtn, &QPushButton::clicked, this, &MainWindow::onAddDoctor);
    connect(m_dUpdateBtn, &QPushButton::clicked, this, &MainWindow::onUpdateDoctor);
    connect(m_dDeleteBtn, &QPushButton::clicked, this, &MainWindow::onDeleteDoctor);
    connect(m_dClearBtn, &QPushButton::clicked, this, &MainWindow::onClearDoctorForm);

    splitRow->addWidget(formGroup, 0);

    auto *tableGroup = new QGroupBox("All Doctors", page);
    auto *tableLayout = new QVBoxLayout(tableGroup);

    m_dSearchEdit = new QLineEdit(tableGroup);
    m_dSearchEdit->setPlaceholderText("Search doctors by name...");
    connect(m_dSearchEdit, &QLineEdit::textChanged, this, &MainWindow::onDoctorSearchTextChanged);
    tableLayout->addWidget(m_dSearchEdit);

    m_doctorTable = new QTableWidget(tableGroup);
    m_doctorTable->setColumnCount(5);
    m_doctorTable->setHorizontalHeaderLabels({"ID", "Name", "Specialization", "Experience", "Phone"});
    m_doctorTable->horizontalHeader()->setStretchLastSection(true);
    m_doctorTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_doctorTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_doctorTable->setSortingEnabled(true);
    connect(m_doctorTable, &QTableWidget::itemSelectionChanged, this, &MainWindow::onDoctorTableSelectionChanged);
    tableLayout->addWidget(m_doctorTable);

    splitRow->addWidget(tableGroup, 1);
    layout->addLayout(splitRow);

    return page;
}

bool MainWindow::validateDoctorForm(QString *error) const
{
    if (m_dIdEdit->text().trimmed().isEmpty()) { if (error) *error = "Doctor ID is required."; return false; }
    bool ok = false;
    m_dIdEdit->text().trimmed().toInt(&ok);
    if (!ok) { if (error) *error = "Doctor ID must be a number."; return false; }
    if (m_dNameEdit->text().trimmed().isEmpty()) { if (error) *error = "Name is required."; return false; }
    if (m_dSpecializationEdit->text().trimmed().isEmpty()) { if (error) *error = "Specialization is required."; return false; }
    if (m_dPhoneEdit->text().trimmed().isEmpty()) { if (error) *error = "Phone number is required."; return false; }
    if (!isValidPhone(m_dPhoneEdit->text().trimmed())) { if (error) *error = "Phone number is invalid. Use 7-15 digits."; return false; }
    return true;
}

void MainWindow::onAddDoctor()
{
    QString err;
    if (!validateDoctorForm(&err)) {
        QMessageBox::warning(this, "Validation Error", err);
        return;
    }
    Doctor d;
    d.id = m_dIdEdit->text().trimmed().toInt();
    d.name = m_dNameEdit->text().trimmed();
    d.specialization = m_dSpecializationEdit->text().trimmed();
    d.experience = m_dExperienceSpin->value();
    d.phone = m_dPhoneEdit->text().trimmed();

    if (!m_doctors.addDoctor(d, &err)) {
        QMessageBox::warning(this, "Could Not Add Doctor", err);
        return;
    }
    persistDoctors();
    refreshDoctorTable(m_doctors.all());
    refreshReports();
    onClearDoctorForm();
    setStatusMessage(QString("Doctor '%1' added successfully.").arg(d.name));
}

void MainWindow::onUpdateDoctor()
{
    if (m_editingDoctorId < 0) {
        QMessageBox::information(this, "No Selection", "Please select a doctor from the table to update.");
        return;
    }
    QString err;
    if (!validateDoctorForm(&err)) {
        QMessageBox::warning(this, "Validation Error", err);
        return;
    }
    Doctor d;
    d.id = m_dIdEdit->text().trimmed().toInt();
    d.name = m_dNameEdit->text().trimmed();
    d.specialization = m_dSpecializationEdit->text().trimmed();
    d.experience = m_dExperienceSpin->value();
    d.phone = m_dPhoneEdit->text().trimmed();

    if (!m_doctors.updateDoctor(m_editingDoctorId, d, &err)) {
        QMessageBox::warning(this, "Could Not Update Doctor", err);
        return;
    }
    persistDoctors();
    refreshDoctorTable(m_doctors.all());
    refreshReports();
    onClearDoctorForm();
    setStatusMessage("Doctor record updated successfully.");
}

void MainWindow::onDeleteDoctor()
{
    if (m_editingDoctorId < 0) {
        QMessageBox::information(this, "No Selection", "Please select a doctor from the table to delete.");
        return;
    }
    auto reply = QMessageBox::question(this, "Confirm Delete",
        QString("Are you sure you want to delete doctor ID %1?").arg(m_editingDoctorId),
        QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) return;

    QString err;
    if (!m_doctors.removeDoctor(m_editingDoctorId, &err)) {
        QMessageBox::warning(this, "Could Not Delete Doctor", err);
        return;
    }
    persistDoctors();
    refreshDoctorTable(m_doctors.all());
    refreshReports();
    onClearDoctorForm();
    setStatusMessage("Doctor deleted successfully.");
}

void MainWindow::onClearDoctorForm()
{
    m_dIdEdit->clear();
    m_dIdEdit->setPlaceholderText(QString::number(m_doctors.nextAvailableId()));
    m_dIdEdit->setEnabled(true);
    m_dNameEdit->clear();
    m_dSpecializationEdit->clear();
    m_dExperienceSpin->setValue(0);
    m_dPhoneEdit->clear();
    m_editingDoctorId = -1;
    m_doctorTable->clearSelection();
}

void MainWindow::onDoctorTableSelectionChanged()
{
    auto selected = m_doctorTable->selectionModel()->selectedRows();
    if (selected.isEmpty()) { m_editingDoctorId = -1; return; }
    int row = selected.first().row();
    int id = m_doctorTable->item(row, 0)->text().toInt();
    Doctor d;
    if (!m_doctors.findById(id, d)) return;

    m_editingDoctorId = id;
    m_dIdEdit->setText(QString::number(d.id));
    m_dIdEdit->setEnabled(false);
    m_dNameEdit->setText(d.name);
    m_dSpecializationEdit->setText(d.specialization);
    m_dExperienceSpin->setValue(d.experience);
    m_dPhoneEdit->setText(d.phone);
}

void MainWindow::onDoctorSearchTextChanged(const QString &text)
{
    if (text.trimmed().isEmpty()) {
        refreshDoctorTable(m_doctors.all());
        return;
    }
    refreshDoctorTable(m_doctors.findByName(text));
}

void MainWindow::refreshDoctorTable(const QVector<Doctor> &doctors)
{
    m_doctorTable->setSortingEnabled(false);
    m_doctorTable->setRowCount(0);
    for (const auto &d : doctors) {
        int row = m_doctorTable->rowCount();
        m_doctorTable->insertRow(row);
        m_doctorTable->setItem(row, 0, new QTableWidgetItem(QString::number(d.id)));
        m_doctorTable->setItem(row, 1, new QTableWidgetItem(d.name));
        m_doctorTable->setItem(row, 2, new QTableWidgetItem(d.specialization));
        m_doctorTable->setItem(row, 3, new QTableWidgetItem(QString::number(d.experience) + " yrs"));
        m_doctorTable->setItem(row, 4, new QTableWidgetItem(d.phone));
    }
    m_doctorTable->setSortingEnabled(true);
}

// ===================================================== APPOINTMENTS PAGE ===
QWidget *MainWindow::buildAppointmentsPage()
{
    auto *page = new QWidget();
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(20, 16, 20, 16);

    auto *title = new QLabel("Appointment Management (First-Come-First-Serve Queue)", page);
    title->setObjectName("pageTitle");
    layout->addWidget(title);

    auto *splitRow = new QHBoxLayout();

    auto *formGroup = new QGroupBox("Book Appointment", page);
    auto *form = new QFormLayout(formGroup);

    m_aPatientIdEdit = new QLineEdit(formGroup);
    m_aPatientIdEdit->setPlaceholderText("Existing Patient ID");
    m_aDoctorIdEdit = new QLineEdit(formGroup);
    m_aDoctorIdEdit->setPlaceholderText("Existing Doctor ID");
    m_aDateEdit = new QDateEdit(formGroup);
    m_aDateEdit->setDate(QDate::currentDate());
    m_aDateEdit->setCalendarPopup(true);
    m_aTimeEdit = new QLineEdit(formGroup);
    m_aTimeEdit->setPlaceholderText("e.g. 10:30 AM");

    form->addRow("Patient ID:", m_aPatientIdEdit);
    form->addRow("Doctor ID:", m_aDoctorIdEdit);
    form->addRow("Date:", m_aDateEdit);
    form->addRow("Time Slot:", m_aTimeEdit);

    m_aBookBtn = new QPushButton("Book Appointment", formGroup);
    m_aBookBtn->setObjectName("successButton");
    connect(m_aBookBtn, &QPushButton::clicked, this, &MainWindow::onBookAppointment);
    form->addRow(m_aBookBtn);

    auto *cancelRow = new QHBoxLayout();
    m_aCancelIdEdit = new QLineEdit(formGroup);
    m_aCancelIdEdit->setPlaceholderText("Appointment ID to cancel");
    m_aCancelBtn = new QPushButton("Cancel Appointment", formGroup);
    m_aCancelBtn->setObjectName("dangerButton");
    connect(m_aCancelBtn, &QPushButton::clicked, this, &MainWindow::onCancelAppointment);
    cancelRow->addWidget(m_aCancelIdEdit);
    cancelRow->addWidget(m_aCancelBtn);
    form->addRow(cancelRow);

    m_aNextLabel = new QLabel("Next in queue: -", formGroup);
    m_aNextLabel->setStyleSheet("font-weight:600;");
    form->addRow(m_aNextLabel);

    m_aServeNextBtn = new QPushButton("Serve Next Appointment", formGroup);
    connect(m_aServeNextBtn, &QPushButton::clicked, this, &MainWindow::onServeNextAppointment);
    form->addRow(m_aServeNextBtn);

    splitRow->addWidget(formGroup, 0);

    auto *tableGroup = new QGroupBox("Appointment Queue (front = next served)", page);
    auto *tableLayout = new QVBoxLayout(tableGroup);
    m_appointmentTable = new QTableWidget(tableGroup);
    m_appointmentTable->setColumnCount(7);
    m_appointmentTable->setHorizontalHeaderLabels({"Appt ID", "Patient ID", "Patient Name", "Doctor ID", "Doctor Name", "Date", "Time"});
    m_appointmentTable->horizontalHeader()->setStretchLastSection(true);
    m_appointmentTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_appointmentTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableLayout->addWidget(m_appointmentTable);
    splitRow->addWidget(tableGroup, 1);

    layout->addLayout(splitRow);
    return page;
}

void MainWindow::onBookAppointment()
{
    bool pidOk = false, didOk = false;
    int patientId = m_aPatientIdEdit->text().trimmed().toInt(&pidOk);
    int doctorId = m_aDoctorIdEdit->text().trimmed().toInt(&didOk);

    if (!pidOk || !didOk) {
        QMessageBox::warning(this, "Validation Error", "Patient ID and Doctor ID must be valid numbers.");
        return;
    }
    if (m_aTimeEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Please enter a time slot.");
        return;
    }
    Patient p;
    if (!m_patients.findById(patientId, p)) {
        QMessageBox::warning(this, "Patient Not Found", QString("No patient exists with ID %1.").arg(patientId));
        return;
    }
    Doctor d;
    if (!m_doctors.findById(doctorId, d)) {
        QMessageBox::warning(this, "Doctor Not Found", QString("No doctor exists with ID %1.").arg(doctorId));
        return;
    }

    Appointment a;
    a.appointmentId = m_appointments.nextAvailableId();
    a.patientId = patientId;
    a.patientName = p.name;
    a.doctorId = doctorId;
    a.doctorName = d.name;
    a.date = m_aDateEdit->date().toString("yyyy-MM-dd");
    a.timeSlot = m_aTimeEdit->text().trimmed();
    a.status = "Scheduled";

    QString err;
    if (!m_appointments.bookAppointment(a, &err)) {
        QMessageBox::warning(this, "Could Not Book Appointment", err);
        return;
    }
    persistAppointments();
    refreshAppointmentTable();
    refreshReports();
    m_aPatientIdEdit->clear();
    m_aDoctorIdEdit->clear();
    m_aTimeEdit->clear();
    setStatusMessage(QString("Appointment #%1 booked for %2.").arg(a.appointmentId).arg(p.name));
}

void MainWindow::onCancelAppointment()
{
    bool ok = false;
    int id = m_aCancelIdEdit->text().trimmed().toInt(&ok);
    if (!ok) {
        QMessageBox::warning(this, "Validation Error", "Please enter a valid Appointment ID.");
        return;
    }
    QString err;
    if (!m_appointments.cancelAppointment(id, &err)) {
        QMessageBox::warning(this, "Could Not Cancel", err);
        return;
    }
    persistAppointments();
    refreshAppointmentTable();
    refreshReports();
    m_aCancelIdEdit->clear();
    setStatusMessage(QString("Appointment #%1 cancelled.").arg(id));
}

void MainWindow::onServeNextAppointment()
{
    Appointment a;
    if (!m_appointments.serveNext(a)) {
        QMessageBox::information(this, "Queue Empty", "There are no appointments in the queue.");
        return;
    }
    persistAppointments();
    refreshAppointmentTable();
    refreshReports();
    setStatusMessage(QString("Served appointment #%1 for %2 with Dr. %3.").arg(a.appointmentId).arg(a.patientName).arg(a.doctorName));
}

void MainWindow::refreshAppointmentTable()
{
    m_appointmentTable->setRowCount(0);
    const auto appts = m_appointments.all();
    for (const auto &a : appts) {
        int row = m_appointmentTable->rowCount();
        m_appointmentTable->insertRow(row);
        m_appointmentTable->setItem(row, 0, new QTableWidgetItem(QString::number(a.appointmentId)));
        m_appointmentTable->setItem(row, 1, new QTableWidgetItem(QString::number(a.patientId)));
        m_appointmentTable->setItem(row, 2, new QTableWidgetItem(a.patientName));
        m_appointmentTable->setItem(row, 3, new QTableWidgetItem(QString::number(a.doctorId)));
        m_appointmentTable->setItem(row, 4, new QTableWidgetItem(a.doctorName));
        m_appointmentTable->setItem(row, 5, new QTableWidgetItem(a.date));
        m_appointmentTable->setItem(row, 6, new QTableWidgetItem(a.timeSlot));
    }
    Appointment next;
    if (m_appointments.peekNext(next)) {
        m_aNextLabel->setText(QString("Next in queue: #%1 - %2 with Dr. %3").arg(next.appointmentId).arg(next.patientName).arg(next.doctorName));
    } else {
        m_aNextLabel->setText("Next in queue: -");
    }
}

// ======================================================= EMERGENCY PAGE ====
QWidget *MainWindow::buildEmergencyPage()
{
    auto *page = new QWidget();
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(20, 16, 20, 16);

    auto *title = new QLabel("Emergency Patient Queue (Priority Queue)", page);
    title->setObjectName("pageTitle");
    layout->addWidget(title);

    auto *splitRow = new QHBoxLayout();

    auto *formGroup = new QGroupBox("Add Emergency Patient", page);
    auto *form = new QFormLayout(formGroup);

    m_eNameEdit = new QLineEdit(formGroup);
    m_eAgeSpin = new QSpinBox(formGroup);
    m_eAgeSpin->setRange(0, 130);
    m_eGenderCombo = new QComboBox(formGroup);
    m_eGenderCombo->addItems({"Male", "Female", "Other"});
    m_eConditionEdit = new QLineEdit(formGroup);
    m_eConditionEdit->setPlaceholderText("e.g. Chest pain, Accident, Burns");
    m_ePriorityCombo = new QComboBox(formGroup);
    m_ePriorityCombo->addItems({"1 - Critical", "2 - Serious", "3 - Normal"});

    form->addRow("Name:", m_eNameEdit);
    form->addRow("Age:", m_eAgeSpin);
    form->addRow("Gender:", m_eGenderCombo);
    form->addRow("Condition:", m_eConditionEdit);
    form->addRow("Priority:", m_ePriorityCombo);

    m_eAddBtn = new QPushButton("Add to Emergency Queue", formGroup);
    m_eAddBtn->setObjectName("dangerButton");
    connect(m_eAddBtn, &QPushButton::clicked, this, &MainWindow::onAddEmergencyPatient);
    form->addRow(m_eAddBtn);

    m_eNextLabel = new QLabel("Next to treat: -", formGroup);
    m_eNextLabel->setStyleSheet("font-weight:600;");
    form->addRow(m_eNextLabel);

    m_eRemoveNextBtn = new QPushButton("Remove / Treat Next Patient", formGroup);
    m_eRemoveNextBtn->setObjectName("successButton");
    connect(m_eRemoveNextBtn, &QPushButton::clicked, this, &MainWindow::onRemoveNextEmergencyPatient);
    form->addRow(m_eRemoveNextBtn);

    splitRow->addWidget(formGroup, 0);

    auto *tableGroup = new QGroupBox("Queue (ordered by priority, most urgent first)", page);
    auto *tableLayout = new QVBoxLayout(tableGroup);
    m_emergencyTable = new QTableWidget(tableGroup);
    m_emergencyTable->setColumnCount(6);
    m_emergencyTable->setHorizontalHeaderLabels({"ID", "Name", "Age", "Gender", "Condition", "Priority"});
    m_emergencyTable->horizontalHeader()->setStretchLastSection(true);
    m_emergencyTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_emergencyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableLayout->addWidget(m_emergencyTable);
    splitRow->addWidget(tableGroup, 1);

    layout->addLayout(splitRow);
    return page;
}

void MainWindow::onAddEmergencyPatient()
{
    if (m_eNameEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Name is required.");
        return;
    }
    if (m_eAgeSpin->value() <= 0) {
        QMessageBox::warning(this, "Validation Error", "Age must be greater than 0.");
        return;
    }
    if (m_eConditionEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Condition is required.");
        return;
    }

    EmergencyPatient p;
    p.id = m_emergency.nextAvailableId();
    p.name = m_eNameEdit->text().trimmed();
    p.age = m_eAgeSpin->value();
    p.gender = m_eGenderCombo->currentText();
    p.condition = m_eConditionEdit->text().trimmed();
    p.priorityLevel = m_ePriorityCombo->currentIndex() + 1; // 1,2,3

    m_emergency.addPatient(p);
    persistEmergency();
    refreshEmergencyTable();
    refreshReports();

    m_eNameEdit->clear();
    m_eAgeSpin->setValue(0);
    m_eConditionEdit->clear();
    m_ePriorityCombo->setCurrentIndex(2);
    setStatusMessage(QString("Emergency patient '%1' added with priority %2.").arg(p.name).arg(p.priorityLabel()));
}

void MainWindow::onRemoveNextEmergencyPatient()
{
    EmergencyPatient p;
    if (!m_emergency.removeNext(p)) {
        QMessageBox::information(this, "Queue Empty", "There are no patients in the emergency queue.");
        return;
    }
    persistEmergency();
    refreshEmergencyTable();
    refreshReports();
    QMessageBox::information(this, "Patient Being Treated",
        QString("Now treating: %1 (Age %2, %3 priority) - %4")
            .arg(p.name).arg(p.age).arg(p.priorityLabel()).arg(p.condition));
    setStatusMessage(QString("Treating emergency patient '%1'.").arg(p.name));
}

void MainWindow::refreshEmergencyTable()
{
    m_emergencyTable->setRowCount(0);
    const auto patients = m_emergency.snapshotSortedByPriority();
    for (const auto &p : patients) {
        int row = m_emergencyTable->rowCount();
        m_emergencyTable->insertRow(row);
        m_emergencyTable->setItem(row, 0, new QTableWidgetItem(QString::number(p.id)));
        m_emergencyTable->setItem(row, 1, new QTableWidgetItem(p.name));
        m_emergencyTable->setItem(row, 2, new QTableWidgetItem(QString::number(p.age)));
        m_emergencyTable->setItem(row, 3, new QTableWidgetItem(p.gender));
        m_emergencyTable->setItem(row, 4, new QTableWidgetItem(p.condition));
        auto *priorityItem = new QTableWidgetItem(p.priorityLabel());
        if (p.priorityLevel == 1) priorityItem->setForeground(Qt::red);
        else if (p.priorityLevel == 2) priorityItem->setForeground(QColor("#e67e22"));
        m_emergencyTable->setItem(row, 5, priorityItem);
    }
    EmergencyPatient next;
    if (m_emergency.peekNext(next)) {
        m_eNextLabel->setText(QString("Next to treat: %1 (%2)").arg(next.name).arg(next.priorityLabel()));
    } else {
        m_eNextLabel->setText("Next to treat: -");
    }
}

// ========================================================= REPORTS PAGE ====
QWidget *MainWindow::buildReportsPage()
{
    auto *page = new QWidget();
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(20, 16, 20, 16);

    auto *title = new QLabel("Reports & Statistics", page);
    title->setObjectName("pageTitle");
    layout->addWidget(title);

    auto *grid = new QGridLayout();
    auto makeCard = [page](const QString &labelText, QLabel *&valueLabel) {
        auto *box = new QGroupBox(labelText, page);
        auto *v = new QVBoxLayout(box);
        valueLabel = new QLabel("0", box);
        valueLabel->setStyleSheet("font-size:26px; font-weight:700;");
        valueLabel->setAlignment(Qt::AlignCenter);
        v->addWidget(valueLabel);
        return box;
    };

    grid->addWidget(makeCard("Total Patients", m_rTotalPatients), 0, 0);
    grid->addWidget(makeCard("Total Doctors", m_rTotalDoctors), 0, 1);
    grid->addWidget(makeCard("Today's Appointments", m_rTodayAppointments), 0, 2);
    grid->addWidget(makeCard("Emergency Cases", m_rEmergencyCases), 0, 3);
    grid->addWidget(makeCard("Male Patients", m_rMaleCount), 1, 0);
    grid->addWidget(makeCard("Female Patients", m_rFemaleCount), 1, 1);
    grid->addWidget(makeCard("Average Age", m_rAverageAge), 1, 2);

    layout->addLayout(grid);

    auto *summaryGroup = new QGroupBox("Detailed Summary", page);
    auto *summaryLayout = new QVBoxLayout(summaryGroup);
    m_rSummaryText = new QTextEdit(summaryGroup);
    m_rSummaryText->setReadOnly(true);
    summaryLayout->addWidget(m_rSummaryText);

    auto *btnRow = new QHBoxLayout();
    auto *refreshBtn = new QPushButton("Refresh", summaryGroup);
    connect(refreshBtn, &QPushButton::clicked, this, &MainWindow::refreshReports);
    auto *exportBtn = new QPushButton("Export to CSV", summaryGroup);
    exportBtn->setObjectName("successButton");
    connect(exportBtn, &QPushButton::clicked, this, &MainWindow::onExportReportCsv);
    auto *printBtn = new QPushButton("Print Report", summaryGroup);
    connect(printBtn, &QPushButton::clicked, this, &MainWindow::onPrintReport);
    btnRow->addWidget(refreshBtn);
    btnRow->addWidget(exportBtn);
    btnRow->addWidget(printBtn);
    summaryLayout->addLayout(btnRow);

    layout->addWidget(summaryGroup, 1);
    return page;
}

void MainWindow::refreshReports()
{
    const auto patients = m_patients.all();
    const auto doctors = m_doctors.all();
    const QString today = QDate::currentDate().toString("yyyy-MM-dd");
    const auto todaysAppts = m_appointments.todays(today);
    const int emergencyCount = m_emergency.count();

    int maleCount = 0, femaleCount = 0;
    long long ageSum = 0;
    for (const auto &p : patients) {
        if (p.gender.compare("Male", Qt::CaseInsensitive) == 0) maleCount++;
        else if (p.gender.compare("Female", Qt::CaseInsensitive) == 0) femaleCount++;
        ageSum += p.age;
    }
    double avgAge = patients.isEmpty() ? 0.0 : static_cast<double>(ageSum) / patients.size();

    m_rTotalPatients->setText(QString::number(patients.size()));
    m_rTotalDoctors->setText(QString::number(doctors.size()));
    m_rTodayAppointments->setText(QString::number(todaysAppts.size()));
    m_rEmergencyCases->setText(QString::number(emergencyCount));
    m_rMaleCount->setText(QString::number(maleCount));
    m_rFemaleCount->setText(QString::number(femaleCount));
    m_rAverageAge->setText(QString::number(avgAge, 'f', 1));

    QString summary;
    summary += QString("Hospital Management System - Report generated on %1\n")
                   .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    summary += QString("=====================================================\n\n");
    summary += QString("Total Patients        : %1\n").arg(patients.size());
    summary += QString("Total Doctors          : %1\n").arg(doctors.size());
    summary += QString("Appointments in Queue  : %1\n").arg(m_appointments.count());
    summary += QString("Today's Appointments    : %1\n").arg(todaysAppts.size());
    summary += QString("Emergency Cases Waiting : %1\n").arg(emergencyCount);
    summary += QString("Male / Female Patients  : %1 / %2\n").arg(maleCount).arg(femaleCount);
    summary += QString("Average Patient Age     : %1\n\n").arg(avgAge, 0, 'f', 1);

    summary += "Patients by Blood Group:\n";
    QMap<QString, int> bloodCounts;
    for (const auto &p : patients) bloodCounts[p.bloodGroup]++;
    for (auto it = bloodCounts.begin(); it != bloodCounts.end(); ++it) {
        summary += QString("  %1 : %2\n").arg(it.key().isEmpty() ? "Unspecified" : it.key()).arg(it.value());
    }

    summary += "\nDoctors by Specialization:\n";
    QMap<QString, int> specCounts;
    for (const auto &d : doctors) specCounts[d.specialization]++;
    for (auto it = specCounts.begin(); it != specCounts.end(); ++it) {
        summary += QString("  %1 : %2\n").arg(it.key().isEmpty() ? "Unspecified" : it.key()).arg(it.value());
    }

    m_rSummaryText->setPlainText(summary);
}

void MainWindow::onExportReportCsv()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Export Report", "hospital_report.csv", "CSV Files (*.csv)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Export Failed", "Could not write to the selected file.");
        return;
    }
    QTextStream out(&file);
    out << "Metric,Value\n";
    out << "Total Patients," << m_rTotalPatients->text() << "\n";
    out << "Total Doctors," << m_rTotalDoctors->text() << "\n";
    out << "Today's Appointments," << m_rTodayAppointments->text() << "\n";
    out << "Emergency Cases," << m_rEmergencyCases->text() << "\n";
    out << "Male Patients," << m_rMaleCount->text() << "\n";
    out << "Female Patients," << m_rFemaleCount->text() << "\n";
    out << "Average Age," << m_rAverageAge->text() << "\n";
    out << "\nID,Name,Age,Gender,Phone,Address,BloodGroup,Disease,AdmissionDate\n";
    for (const auto &p : m_patients.all()) {
        out << p.id << "," << p.name << "," << p.age << "," << p.gender << "," << p.phone << ","
            << p.address << "," << p.bloodGroup << "," << p.disease << "," << p.admissionDate << "\n";
    }
    file.close();
    QMessageBox::information(this, "Export Complete", QString("Report exported to:\n%1").arg(fileName));
}

void MainWindow::onPrintReport()
{
    QPrinter printer(QPrinter::HighResolution);
    QPrintDialog dialog(&printer, this);
    if (dialog.exec() != QDialog::Accepted) return;

    QPainter painter(&printer);
    QFont font("Courier New", 10);
    painter.setFont(font);
    QStringList lines = m_rSummaryText->toPlainText().split('\n');
    int y = 40;
    const int lineHeight = 16;
    for (const QString &line : lines) {
        painter.drawText(40, y, line);
        y += lineHeight;
        if (y > printer.pageRect(QPrinter::DevicePixel).height() - 40) {
            printer.newPage();
            y = 40;
        }
    }
}

// ========================================================== SEARCH PAGE ====
QWidget *MainWindow::buildSearchPage()
{
    auto *page = new QWidget();
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(20, 16, 20, 16);

    auto *title = new QLabel("Instant Search", page);
    title->setObjectName("pageTitle");
    layout->addWidget(title);

    auto *searchRow = new QHBoxLayout();
    m_searchTypeCombo = new QComboBox(page);
    m_searchTypeCombo->addItems({"Patient ID", "Patient Name", "Doctor ID", "Doctor Name"});
    m_searchGlobalEdit = new QLineEdit(page);
    m_searchGlobalEdit->setPlaceholderText("Type to search...");
    connect(m_searchGlobalEdit, &QLineEdit::returnPressed, this, &MainWindow::onGlobalSearch);
    auto *searchBtn = new QPushButton("Search", page);
    connect(searchBtn, &QPushButton::clicked, this, &MainWindow::onGlobalSearch);

    searchRow->addWidget(m_searchTypeCombo);
    searchRow->addWidget(m_searchGlobalEdit, 1);
    searchRow->addWidget(searchBtn);
    layout->addLayout(searchRow);

    m_searchResultTable = new QTableWidget(page);
    m_searchResultTable->setColumnCount(1);
    m_searchResultTable->setHorizontalHeaderLabels({"Result"});
    m_searchResultTable->horizontalHeader()->setStretchLastSection(true);
    m_searchResultTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(m_searchResultTable, 1);

    return page;
}

void MainWindow::onGlobalSearch()
{
    const QString query = m_searchGlobalEdit->text().trimmed();
    if (query.isEmpty()) {
        QMessageBox::information(this, "Search", "Please enter a search term.");
        return;
    }
    const QString mode = m_searchTypeCombo->currentText();
    m_searchResultTable->setRowCount(0);

    if (mode == "Patient ID") {
        bool ok = false;
        int id = query.toInt(&ok);
        Patient p;
        m_searchResultTable->setColumnCount(9);
        m_searchResultTable->setHorizontalHeaderLabels({"ID", "Name", "Age", "Gender", "Phone", "Address", "Blood Group", "Disease", "Admission Date"});
        if (ok && m_patients.findById(id, p)) {
            m_searchResultTable->insertRow(0);
            m_searchResultTable->setItem(0, 0, new QTableWidgetItem(QString::number(p.id)));
            m_searchResultTable->setItem(0, 1, new QTableWidgetItem(p.name));
            m_searchResultTable->setItem(0, 2, new QTableWidgetItem(QString::number(p.age)));
            m_searchResultTable->setItem(0, 3, new QTableWidgetItem(p.gender));
            m_searchResultTable->setItem(0, 4, new QTableWidgetItem(p.phone));
            m_searchResultTable->setItem(0, 5, new QTableWidgetItem(p.address));
            m_searchResultTable->setItem(0, 6, new QTableWidgetItem(p.bloodGroup));
            m_searchResultTable->setItem(0, 7, new QTableWidgetItem(p.disease));
            m_searchResultTable->setItem(0, 8, new QTableWidgetItem(p.admissionDate));
        } else {
            setStatusMessage("No patient found with that ID.");
        }
    } else if (mode == "Patient Name") {
        m_searchResultTable->setColumnCount(9);
        m_searchResultTable->setHorizontalHeaderLabels({"ID", "Name", "Age", "Gender", "Phone", "Address", "Blood Group", "Disease", "Admission Date"});
        // unordered_map-backed instant lookup for exact-name fast path, falls back to partial scan.
        auto it = m_searchIndex.find(query.toLower());
        QVector<Patient> matches;
        if (it != m_searchIndex.end()) {
            Patient p;
            if (m_patients.findById(it->second, p)) matches.push_back(p);
        }
        for (const auto &p : m_patients.findByName(query)) {
            bool exists = false;
            for (const auto &m : matches) if (m.id == p.id) { exists = true; break; }
            if (!exists) matches.push_back(p);
        }
        for (const auto &p : matches) {
            int row = m_searchResultTable->rowCount();
            m_searchResultTable->insertRow(row);
            m_searchResultTable->setItem(row, 0, new QTableWidgetItem(QString::number(p.id)));
            m_searchResultTable->setItem(row, 1, new QTableWidgetItem(p.name));
            m_searchResultTable->setItem(row, 2, new QTableWidgetItem(QString::number(p.age)));
            m_searchResultTable->setItem(row, 3, new QTableWidgetItem(p.gender));
            m_searchResultTable->setItem(row, 4, new QTableWidgetItem(p.phone));
            m_searchResultTable->setItem(row, 5, new QTableWidgetItem(p.address));
            m_searchResultTable->setItem(row, 6, new QTableWidgetItem(p.bloodGroup));
            m_searchResultTable->setItem(row, 7, new QTableWidgetItem(p.disease));
            m_searchResultTable->setItem(row, 8, new QTableWidgetItem(p.admissionDate));
        }
    } else if (mode == "Doctor ID") {
        bool ok = false;
        int id = query.toInt(&ok);
        Doctor d;
        m_searchResultTable->setColumnCount(5);
        m_searchResultTable->setHorizontalHeaderLabels({"ID", "Name", "Specialization", "Experience", "Phone"});
        if (ok && m_doctors.findById(id, d)) {
            m_searchResultTable->insertRow(0);
            m_searchResultTable->setItem(0, 0, new QTableWidgetItem(QString::number(d.id)));
            m_searchResultTable->setItem(0, 1, new QTableWidgetItem(d.name));
            m_searchResultTable->setItem(0, 2, new QTableWidgetItem(d.specialization));
            m_searchResultTable->setItem(0, 3, new QTableWidgetItem(QString::number(d.experience) + " yrs"));
            m_searchResultTable->setItem(0, 4, new QTableWidgetItem(d.phone));
        } else {
            setStatusMessage("No doctor found with that ID.");
        }
    } else { // Doctor Name
        m_searchResultTable->setColumnCount(5);
        m_searchResultTable->setHorizontalHeaderLabels({"ID", "Name", "Specialization", "Experience", "Phone"});
        for (const auto &d : m_doctors.findByName(query)) {
            int row = m_searchResultTable->rowCount();
            m_searchResultTable->insertRow(row);
            m_searchResultTable->setItem(row, 0, new QTableWidgetItem(QString::number(d.id)));
            m_searchResultTable->setItem(row, 1, new QTableWidgetItem(d.name));
            m_searchResultTable->setItem(row, 2, new QTableWidgetItem(d.specialization));
            m_searchResultTable->setItem(row, 3, new QTableWidgetItem(QString::number(d.experience) + " yrs"));
            m_searchResultTable->setItem(row, 4, new QTableWidgetItem(d.phone));
        }
    }
}

// ======================================================== SETTINGS PAGE ====
QWidget *MainWindow::buildSettingsPage()
{
    auto *page = new QWidget();
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(20, 16, 20, 16);

    auto *title = new QLabel("Settings", page);
    title->setObjectName("pageTitle");
    layout->addWidget(title);

    auto *themeGroup = new QGroupBox("Appearance", page);
    auto *themeLayout = new QVBoxLayout(themeGroup);
    m_settingsThemeBtn = new QPushButton("Toggle Dark / Light Mode", themeGroup);
    connect(m_settingsThemeBtn, &QPushButton::clicked, this, &MainWindow::onToggleTheme);
    themeLayout->addWidget(m_settingsThemeBtn);
    layout->addWidget(themeGroup);

    auto *passGroup = new QGroupBox("Change Password", page);
    auto *passForm = new QFormLayout(passGroup);
    m_settingsOldPass = new QLineEdit(passGroup);
    m_settingsOldPass->setEchoMode(QLineEdit::Password);
    m_settingsNewPass = new QLineEdit(passGroup);
    m_settingsNewPass->setEchoMode(QLineEdit::Password);
    passForm->addRow("Current Password:", m_settingsOldPass);
    passForm->addRow("New Password:", m_settingsNewPass);
    auto *changePassBtn = new QPushButton("Change Password", passGroup);
    connect(changePassBtn, &QPushButton::clicked, this, &MainWindow::onChangePassword);
    passForm->addRow(changePassBtn);
    layout->addWidget(passGroup);

    auto *aboutGroup = new QGroupBox("About / Help", page);
    auto *aboutLayout = new QHBoxLayout(aboutGroup);
    auto *aboutBtn = new QPushButton("About", aboutGroup);
    connect(aboutBtn, &QPushButton::clicked, this, &MainWindow::onAbout);
    auto *helpBtn = new QPushButton("Help", aboutGroup);
    connect(helpBtn, &QPushButton::clicked, this, &MainWindow::onHelp);
    aboutLayout->addWidget(aboutBtn);
    aboutLayout->addWidget(helpBtn);
    layout->addWidget(aboutGroup);

    layout->addStretch();
    return page;
}

void MainWindow::onChangePassword()
{
    QString oldPass = m_settingsOldPass->text();
    QString newPass = m_settingsNewPass->text();
    if (oldPass.isEmpty() || newPass.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Please fill in both password fields.");
        return;
    }
    if (newPass.length() < 4) {
        QMessageBox::warning(this, "Validation Error", "New password must be at least 4 characters.");
        return;
    }
    if (!m_fileManager->validateLogin(m_loggedInUser, oldPass)) {
        QMessageBox::warning(this, "Incorrect Password", "Your current password is incorrect.");
        return;
    }
    auto users = m_fileManager->loadUsers();
    for (auto &u : users) {
        if (u.username == m_loggedInUser) u.password = newPass;
    }
    m_fileManager->saveUsers(users);
    m_settingsOldPass->clear();
    m_settingsNewPass->clear();
    QMessageBox::information(this, "Password Changed", "Your password has been updated successfully.");
}
