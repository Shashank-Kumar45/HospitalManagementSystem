# Hospital Management System

A complete, **offline** desktop Hospital Management System built with **C++17** and **Qt Widgets**.
No external/cloud database is used — all data is persisted to local text files
(`patients.txt`, `doctors.txt`, `appointments.txt`, `users.txt`).

## Abstract

The Hospital Management System (HMS) is a standalone desktop application that helps a small
clinic or hospital manage patients, doctors, appointments and emergency cases without needing
an internet connection or a database server. All records are stored in plain text files in the
application directory, loaded into in-memory data structures on startup and saved automatically
after every change.

## Objective

* Provide a single, self-contained executable for day-to-day hospital administration.
* Demonstrate practical use of classic data structures (linked list, queue, priority queue,
  stack, hash map) inside a real, GUI-driven application.
* Require no setup beyond the Qt runtime — no servers, drivers, or network access.

## Features

* Secure login screen (default `admin` / `admin123`), passwords changeable from Settings.
* **Patient Management** — add / edit / delete / search / view, with **Undo Delete**.
* **Doctor Management** — add / edit / delete / search / view.
* **Appointment Management** — FCFS booking queue, cancel, view, filter by status.
* **Emergency Queue** — priority-based (1 = Critical, 2 = Serious, 3 = Normal), always serves
  the highest-priority patient next.
* **Instant Search** across patients and doctors using hash maps (O(1) average lookup).
* **Reports** — total patients/doctors, today's appointments, emergency case count,
  male/female counts, average age; exportable to CSV and printable.
* Dark Mode / Light Mode toggle (persisted between sessions).
* Keyboard shortcuts, sortable/filterable tables, status bar, toolbar, menu bar.
* About / Help dialogs.
* Field validation everywhere: empty fields, duplicate IDs, phone format, age range, etc.
* Automatic load on startup and save after every modification; missing files are created
  safely with no crash.

## Data Structures Used

| Data Structure        | Where it is used                                              |
|------------------------|----------------------------------------------------------------|
| **Linked List**        | `PatientList` — primary in-memory store of patient records.   |
| **Queue** (`std::deque` used as FCFS queue) | `AppointmentQueue` — appointments are served First Come First Served. |
| **Priority Queue** (`std::priority_queue`) | `EmergencyQueue` — emergency patients sorted by severity (1 highest). |
| **Stack** (`std::stack`) | Undo-delete history for patients.                             |
| **unordered_map**       | O(1) ID-based lookup for patients/doctors; backbone of the Search module. |
| **Vector** (`QVector`)  | Returned snapshots/results for tables, search results, reports. |

## Algorithms

* **Linear/partial-match search** — case-insensitive substring matching by name.
* **Hash-based exact search** — O(1) average lookup by numeric ID via `unordered_map`.
* **FCFS scheduling** — appointments processed in arrival order via queue semantics.
* **Priority scheduling** — emergency patients ordered by a min-heap on priority level
  (ties broken by arrival order).
* **Aggregate statistics** — single-pass computation of counts/averages for the Reports module.

## Project Structure

```
HospitalManagementSystem/
├── HospitalManagementSystem.pro   # qmake project file (open this in Qt Creator)
├── main.cpp
├── login.h / login.cpp
├── mainwindow.h / mainwindow.cpp
├── patient.h / patient.cpp
├── doctor.h / doctor.cpp
├── appointment.h / appointment.cpp
├── emergency.h / emergency.cpp
├── filemanager.h / filemanager.cpp
├── styles.qss                     # light theme
├── styles_dark.qss                # dark theme
├── resources.qrc
├── patients.txt
├── doctors.txt
├── appointments.txt
├── users.txt
└── README.md
```

## Installation / How to Run

### Requirements
* Qt 5.15+ or Qt 6 (Widgets module) with a C++17 compiler (MSVC, MinGW, or GCC/Clang).
* Qt Creator (recommended) or qmake + make from the command line.

### Using Qt Creator
1. Open Qt Creator → **File → Open File or Project** → select `HospitalManagementSystem.pro`.
2. Choose a Kit (e.g. Desktop Qt 6.x or 5.15).
3. Click **Build** (hammer icon), then **Run** (green play icon).
4. Log in with `admin` / `admin123`.

### Command line
```bash
cd HospitalManagementSystem
qmake HospitalManagementSystem.pro
make           # or mingw32-make / nmake on Windows
./HospitalManagementSystem
```

On first run, the four `.txt` files in the project directory are read; if any are missing or
empty they are created automatically with no data, so the app always starts cleanly.

## Screenshots

*(placeholders — replace with actual screenshots once built)*

* `docs/screenshots/login.png`
* `docs/screenshots/dashboard.png`
* `docs/screenshots/patients.png`
* `docs/screenshots/emergency.png`
* `docs/screenshots/reports.png`

## Future Scope

* Optional encrypted local storage for patient records.
* Multi-user roles (doctor/receptionist/admin) with different permissions.
* Billing and pharmacy/inventory modules.
* PDF report generation in addition to CSV export.
* Appointment reminders via local notifications.

## License

This project is provided as-is for educational/demonstration purposes.
