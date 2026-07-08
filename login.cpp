#include "login.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QKeyEvent>
#include <QPixmap>
#include <QIcon>
#include <QStyle>
#include <QApplication>

LoginWindow::LoginWindow(FileManager *fileManager, QWidget *parent)
    : QDialog(parent), m_fileManager(fileManager)
{
    setWindowTitle("Hospital Management System - Login");
    setFixedSize(420, 460);
    buildUi();
}

void LoginWindow::buildUi()
{
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);

    // Header banner
    auto *header = new QFrame(this);
    header->setStyleSheet("background-color:#1c2b39;");
    header->setFixedHeight(140);
    auto *headerLayout = new QVBoxLayout(header);
    auto *iconLabel = new QLabel(header);
    QIcon icon = QApplication::style()->standardIcon(QStyle::SP_ComputerIcon);
    iconLabel->setPixmap(icon.pixmap(48, 48));
    iconLabel->setAlignment(Qt::AlignCenter);
    auto *titleLabel = new QLabel("Hospital Management System", header);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("color:white; font-size:18px; font-weight:700;");
    auto *subtitleLabel = new QLabel("Offline Desktop Edition", header);
    subtitleLabel->setAlignment(Qt::AlignCenter);
    subtitleLabel->setStyleSheet("color:#9fb3c8; font-size:11px;");
    headerLayout->addStretch();
    headerLayout->addWidget(iconLabel);
    headerLayout->addWidget(titleLabel);
    headerLayout->addWidget(subtitleLabel);
    headerLayout->addStretch();
    outer->addWidget(header);

    auto *formContainer = new QWidget(this);
    auto *formLayout = new QVBoxLayout(formContainer);
    formLayout->setContentsMargins(36, 24, 36, 24);
    formLayout->setSpacing(14);

    auto *loginLabel = new QLabel("Sign in to continue", formContainer);
    loginLabel->setStyleSheet("font-size:15px; font-weight:600;");
    formLayout->addWidget(loginLabel);

    m_usernameEdit = new QLineEdit(formContainer);
    m_usernameEdit->setPlaceholderText("Username");
    m_usernameEdit->setText("admin");
    formLayout->addWidget(m_usernameEdit);

    auto *passwordRow = new QHBoxLayout();
    m_passwordEdit = new QLineEdit(formContainer);
    m_passwordEdit->setPlaceholderText("Password");
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_toggleVisibilityButton = new QPushButton("Show", formContainer);
    m_toggleVisibilityButton->setObjectName("secondaryButton");
    m_toggleVisibilityButton->setFixedWidth(60);
    connect(m_toggleVisibilityButton, &QPushButton::clicked, this, &LoginWindow::onTogglePasswordVisibility);
    passwordRow->addWidget(m_passwordEdit);
    passwordRow->addWidget(m_toggleVisibilityButton);
    formLayout->addLayout(passwordRow);

    m_errorLabel = new QLabel("", formContainer);
    m_errorLabel->setStyleSheet("color:#e74c3c; font-weight:600;");
    m_errorLabel->setWordWrap(true);
    formLayout->addWidget(m_errorLabel);

    m_loginButton = new QPushButton("Login", formContainer);
    m_loginButton->setMinimumHeight(38);
    connect(m_loginButton, &QPushButton::clicked, this, &LoginWindow::onLoginClicked);
    formLayout->addWidget(m_loginButton);

    auto *hint = new QLabel("Default credentials: admin / admin123", formContainer);
    hint->setStyleSheet("color:#7f8c9b; font-size:11px;");
    hint->setAlignment(Qt::AlignCenter);
    formLayout->addWidget(hint);

    formLayout->addStretch();
    outer->addWidget(formContainer);

    connect(m_usernameEdit, &QLineEdit::returnPressed, this, &LoginWindow::onLoginClicked);
    connect(m_passwordEdit, &QLineEdit::returnPressed, this, &LoginWindow::onLoginClicked);
}

void LoginWindow::onTogglePasswordVisibility()
{
    if (m_passwordEdit->echoMode() == QLineEdit::Password) {
        m_passwordEdit->setEchoMode(QLineEdit::Normal);
        m_toggleVisibilityButton->setText("Hide");
    } else {
        m_passwordEdit->setEchoMode(QLineEdit::Password);
        m_toggleVisibilityButton->setText("Show");
    }
}

void LoginWindow::onLoginClicked()
{
    const QString username = m_usernameEdit->text().trimmed();
    const QString password = m_passwordEdit->text();

    if (username.isEmpty() || password.isEmpty()) {
        m_errorLabel->setText("Please enter both username and password.");
        return;
    }

    if (m_fileManager->validateLogin(username, password)) {
        m_loggedInUser = username;
        accept();
    } else {
        m_errorLabel->setText("Invalid username or password. Please try again.");
        m_passwordEdit->clear();
        m_passwordEdit->setFocus();
    }
}
