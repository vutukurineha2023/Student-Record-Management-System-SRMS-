// srms_gui.cpp
// SRMS - Student Record Management System
// Login + SRM-themed professional colourful GUI (Win32 API) + in-memory backend

#include <windows.h>
#include <string>
#include <vector>
#include <sstream>
#include <cstdlib>
#include <iomanip>

using std::string;
using std::vector;
using std::ostringstream;
using std::setw;

// ======================== BACKEND (IN-MEMORY) ========================

struct Student {
    int roll;
    string name;
    string dept;
    int sem;
    float cgpa;
    char grade;
};

struct Query {
    int id;
    int roll;
    string name;
    string message;
    string status;   // e.g. "Pending"
};

static vector<Student> gStudents;
static vector<Query>   gQueries;
static int gNextQueryId = 1;

void backend_init() {
    // currently nothing
}

bool backend_addStudent(int roll,
                        const string& name,
                        const string& dept,
                        int sem,
                        float cgpa,
                        char grade) {
    if (roll <= 0 || name.empty() || dept.empty())
        return false;

    Student s;
    s.roll  = roll;
    s.name  = name;
    s.dept  = dept;
    s.sem   = sem;
    s.cgpa  = cgpa;
    s.grade = grade;

    gStudents.push_back(s);
    return true;
}

// update student (admin can correct details after checking queries)
bool backend_updateStudent(int roll,
                           const string& name,
                           const string& dept,
                           int sem,
                           float cgpa,
                           char grade) {
    for (auto &s : gStudents) {
        if (s.roll == roll) {
            s.name  = name;
            s.dept  = dept;
            s.sem   = sem;
            s.cgpa  = cgpa;
            s.grade = grade;
            return true;
        }
    }
    return false;  // not found
}

// delete student by roll
bool backend_deleteStudent(int roll) {
    for (auto it = gStudents.begin(); it != gStudents.end(); ++it) {
        if (it->roll == roll) {
            gStudents.erase(it);
            return true;
        }
    }
    return false;
}

int backend_addStudentNameOnly(const string& name) {
    int newRoll = (gStudents.empty() ? 1 : gStudents.back().roll + 1);

    Student s;
    s.roll  = newRoll;
    s.name  = name;
    s.dept  = "N/A";
    s.sem   = 0;
    s.cgpa  = 0.0f;
    s.grade = '-';

    gStudents.push_back(s);
    return newRoll;
}

int backend_searchNameOrAdd(const string& name, bool &wasAdded) {
    wasAdded = false;
    for (const auto& s : gStudents) {
        if (s.name == name) {
            return s.roll;     // existing student
        }
    }
    wasAdded = true;
    return backend_addStudentNameOnly(name);
}

string backend_getStudentByRoll(int roll) {
    for (const auto& s : gStudents) {
        if (s.roll == roll) {
            ostringstream oss;
            oss << "Roll   : " << s.roll  << "\r\n"
                << "Name   : " << s.name  << "\r\n"
                << "Dept   : " << s.dept  << "\r\n"
                << "Sem    : " << s.sem   << "\r\n"
                << "CGPA   : " << s.cgpa  << "\r\n"
                << "Grade  : " << s.grade;
            return oss.str();
        }
    }
    return "Student not found.";
}

string backend_getAllStudents() {
    ostringstream oss;
    // header
    oss << std::left
        << setw(5)  << "ROLL"  << " "
        << setw(18) << "NAME"
        << setw(10) << "DEPT"
        << setw(6)  << "SEM"
        << setw(7)  << "CGPA"
        << setw(7)  << "GRADE"
        << "STATUS" << "\r\n";

    oss << "---------------------------------------------------------------------\r\n";

    if (gStudents.empty()) {
        oss << "(No students added yet)\r\n";
    } else {
        for (const auto& s : gStudents) {
            string status;
            if (s.cgpa >= 8.0f)      status = "Excellent";
            else if (s.cgpa >= 7.0f) status = "Very Good";
            else if (s.cgpa >= 6.0f) status = "Good";
            else if (s.cgpa >= 5.0f) status = "Average";
            else                     status = "Needs Help";

            oss << std::left
                << setw(5)  << s.roll << " "
                << setw(18) << s.name
                << setw(10) << s.dept
                << setw(6)  << s.sem
                << setw(7)  << s.cgpa
                << setw(7)  << s.grade
                << status   << "\r\n";
        }
    }
    return oss.str();
}

int backend_addQuery(int roll,
                     const string& name,
                     const string& message) {
    if (roll <= 0 || name.empty() || message.empty())
        return -1;

    Query q;
    q.id      = gNextQueryId++;
    q.roll    = roll;
    q.name    = name;
    q.message = message;
    q.status  = "Pending";

    gQueries.push_back(q);
    return q.id;
}

string backend_getAllQueries() {
    ostringstream oss;
    oss << std::left
        << setw(4)  << "ID"
        << setw(6)  << "ROLL"
        << setw(15) << "NAME"
        << setw(10) << "STATUS"
        << "MESSAGE" << "\r\n";

    oss << "---------------------------------------------------------------------\r\n";

    if (gQueries.empty()) {
        oss << "(No queries submitted yet)\r\n";
    } else {
        for (const auto& q : gQueries) {
            oss << std::left
                << setw(4)  << q.id
                << setw(6)  << q.roll
                << setw(15) << q.name
                << setw(10) << q.status
                << q.message << "\r\n";
        }
    }
    return oss.str();
}

// ======================== GUI PART (WIN32) ========================

LRESULT CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK LoginWndProc(HWND, UINT, WPARAM, LPARAM);

// Global window handles
HWND gMainHwnd  = NULL;
HWND gLoginHwnd = NULL;

// Main-window controls
HWND hAdminRoll, hAdminName, hAdminDept, hAdminSem, hAdminCgpa, hAdminGrade;
HWND hAdminOutput;
HWND hSearchNameEdit;
HWND hStudRoll, hStudName, hStudMsg, hStudOutput;
HWND hHeaderLabel, hAdminIdLabel;
HWND hLoginUserEdit, hLoginPassEdit;
HWND hAdminOutLabel, hQueryOutLabel;  // new labels above outputs

// Brushes & font
HBRUSH gMainBgBrush   = NULL;   // overall background
HBRUSH gOutputBgBrush = NULL;   // right side outputs
HBRUSH gQueryBgBrush  = NULL;   // query input box
HFONT  gHeaderFont    = NULL;

// Admin credentials
const char* ADMIN_USER       = "admin";
const char* ADMIN_PASS       = "1234";
const char* ADMIN_ID_DISPLAY = "Admin ID: SRMS-ADMIN-001";

void InfoBox(HWND hwnd, const char* msg, const char* title = "SRMS") {
    MessageBoxA(hwnd, msg, title, MB_OK | MB_ICONINFORMATION);
}

string GetEditText(HWND h) {
    int len = GetWindowTextLengthA(h);
    if (len <= 0) return "";
    string s(len, '\0');
    GetWindowTextA(h, &s[0], len + 1);
    return s;
}

// ======================== FRONTEND ACTIONS ========================

void GUI_AddStudent(HWND hwnd) {
    string rollStr  = GetEditText(hAdminRoll);
    string name     = GetEditText(hAdminName);
    string dept     = GetEditText(hAdminDept);
    string semStr   = GetEditText(hAdminSem);
    string cgpaStr  = GetEditText(hAdminCgpa);
    string gradeStr = GetEditText(hAdminGrade);

    if (rollStr.empty() || name.empty() || dept.empty() ||
        semStr.empty()  || cgpaStr.empty() || gradeStr.empty()) {
        InfoBox(hwnd, "For adding a student, please fill ALL fields.");
        return;
    }

    int   roll = std::atoi(rollStr.c_str());
    int   sem  = std::atoi(semStr.c_str());
    float cgpa = (float)std::atof(cgpaStr.c_str());
    char  grade = gradeStr[0];

    bool ok = backend_addStudent(roll, name, dept, sem, cgpa, grade);
    if (!ok) {
        InfoBox(hwnd, "Invalid student data.", "Error");
        return;
    }

    InfoBox(hwnd, "Student added successfully.");

    // clear boxes
    SetWindowTextA(hAdminRoll,  "");
    SetWindowTextA(hAdminName,  "");
    SetWindowTextA(hAdminDept,  "");
    SetWindowTextA(hAdminSem,   "");
    SetWindowTextA(hAdminCgpa,  "");
    SetWindowTextA(hAdminGrade, "");

    // auto-refresh table
    string out = backend_getAllStudents();
    SetWindowTextA(hAdminOutput, out.c_str());
}

// update student details (used when admin wants to correct record)
void GUI_UpdateStudent(HWND hwnd) {
    string rollStr  = GetEditText(hAdminRoll);
    string name     = GetEditText(hAdminName);
    string dept     = GetEditText(hAdminDept);
    string semStr   = GetEditText(hAdminSem);
    string cgpaStr  = GetEditText(hAdminCgpa);
    string gradeStr = GetEditText(hAdminGrade);

    if (rollStr.empty()) {
        InfoBox(hwnd, "Please enter Roll number to update student.");
        return;
    }
    if (name.empty() || dept.empty() || semStr.empty() ||
        cgpaStr.empty() || gradeStr.empty()) {
        InfoBox(hwnd, "For updating, fill all fields with the NEW details.");
        return;
    }

    int   roll = std::atoi(rollStr.c_str());
    int   sem  = std::atoi(semStr.c_str());
    float cgpa = (float)std::atof(cgpaStr.c_str());
    char  grade = gradeStr[0];

    bool ok = backend_updateStudent(roll, name, dept, sem, cgpa, grade);
    if (!ok) {
        InfoBox(hwnd, "Student with this roll not found.", "Update Failed");
        return;
    }

    InfoBox(hwnd, "Student details updated successfully.");

    // refresh table so changes are visible
    string out = backend_getAllStudents();
    SetWindowTextA(hAdminOutput, out.c_str());
}

// delete student by roll
void GUI_DeleteStudent(HWND hwnd) {
    string rollStr = GetEditText(hAdminRoll);
    if (rollStr.empty()) {
        InfoBox(hwnd, "Enter the Roll number in Admin section to delete.");
        return;
    }
    int roll = std::atoi(rollStr.c_str());

    if (!backend_deleteStudent(roll)) {
        InfoBox(hwnd, "Student with this roll not found.", "Delete Failed");
        return;
    }

    InfoBox(hwnd, "Student deleted successfully.");

    SetWindowTextA(hAdminRoll, "");

    // refresh table
    string out = backend_getAllStudents();
    SetWindowTextA(hAdminOutput, out.c_str());
}

void GUI_ViewStudents() {
    string out = backend_getAllStudents();
    SetWindowTextA(hAdminOutput, out.c_str());
}

void GUI_SearchName(HWND hwnd) {
    string name = GetEditText(hSearchNameEdit);
    if (name.empty()) {
        InfoBox(hwnd, "Please enter a name to search.");
        return;
    }

    bool wasAdded = false;
    int roll = backend_searchNameOrAdd(name, wasAdded);
    string infoLine = backend_getStudentByRoll(roll);

    string msg;
    if (wasAdded) {
        msg = "Student with this name was NOT present.\r\n"
              "A new student has been added automatically.\r\n\r\nDetails:\r\n" + infoLine;
    } else {
        msg = "Student found:\r\n\r\n" + infoLine;
    }

    InfoBox(hwnd, msg.c_str(), "Search by Name");

    GUI_ViewStudents();
}

void GUI_AddQuery(HWND hwnd) {
    string rollStr = GetEditText(hStudRoll);
    string name    = GetEditText(hStudName);
    string qmsg    = GetEditText(hStudMsg);

    if (rollStr.empty() || name.empty() || qmsg.empty()) {
        InfoBox(hwnd, "Please fill Roll, Name and Query message.");
        return;
    }

    int roll = std::atoi(rollStr.c_str());
    int qid  = backend_addQuery(roll, name, qmsg);
    if (qid < 0) {
        InfoBox(hwnd, "Could not add query.", "Error");
        return;
    }

    string info = "Query submitted. Your Query ID: " + std::to_string(qid);
    InfoBox(hwnd, info.c_str(), "Query Submitted");

    SetWindowTextA(hStudRoll, "");
    SetWindowTextA(hStudName, "");
    SetWindowTextA(hStudMsg,  "");
}

void GUI_ViewQueries() {
    string out = backend_getAllQueries();
    SetWindowTextA(hStudOutput, out.c_str());
}

// IDs
#define ID_BTN_ADD_STU      1001
#define ID_BTN_VIEW_STU     1002
#define ID_BTN_ADD_Q        1003
#define ID_BTN_VIEW_Q       1004
#define ID_BTN_SEARCH_NAME  1005
#define ID_BTN_UPDATE_STU   1006
#define ID_BTN_DEL_STU      1007

#define ID_LOGIN_USER_EDIT  2001
#define ID_LOGIN_PASS_EDIT  2002
#define ID_LOGIN_OK         2003
#define ID_LOGIN_CANCEL     2004

// ======================== WinMain (GUI entry) ========================

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    backend_init();

    // SRM-style colour theme
    gMainBgBrush   = CreateSolidBrush(RGB(240, 244, 248)); // light grey-blue
    gOutputBgBrush = CreateSolidBrush(RGB(255, 255, 255)); // white panels
    gQueryBgBrush  = CreateSolidBrush(RGB(235, 244, 255)); // very light blue

    const char MAIN_CLASS[]  = "SRMS_MAIN_CLASS";
    const char LOGIN_CLASS[] = "SRMS_LOGIN_CLASS";

    // Main window class
    WNDCLASSA wc{};
    wc.lpfnWndProc   = MainWndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = MAIN_CLASS;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = gMainBgBrush;
    if (!RegisterClassA(&wc)) return 0;

    // Login window class
    WNDCLASSA wcLogin{};
    wcLogin.lpfnWndProc   = LoginWndProc;
    wcLogin.hInstance     = hInstance;
    wcLogin.lpszClassName = LOGIN_CLASS;
    wcLogin.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wcLogin.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    if (!RegisterClassA(&wcLogin)) return 0;

    // Main window (a bit taller)
    gMainHwnd = CreateWindowExA(
        0, MAIN_CLASS,
        "SRMS - Student Record Management System (GUI + Backend)",
        WS_OVERLAPPED | WS_MINIMIZEBOX | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT,
        900, 700,
        NULL, NULL, hInstance, NULL
    );
    if (!gMainHwnd) return 0;

    ShowWindow(gMainHwnd, SW_HIDE);
    UpdateWindow(gMainHwnd);

    // Login window (shown first)
    gLoginHwnd = CreateWindowExA(
        0, LOGIN_CLASS,
        "Admin Login - SRMS",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT,
        400, 220,
        NULL, NULL, hInstance, NULL
    );
    if (!gLoginHwnd) return 0;

    ShowWindow(gLoginHwnd, nCmdShow);
    UpdateWindow(gLoginHwnd);

    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return (int)msg.wParam;
}

// ======================== Main Window Proc ========================

// helper to draw a SRM-style vertical blue gradient in header
void DrawHeaderGradient(HDC hdc, int width, int height) {
    for (int y = 0; y < height; ++y) {
        double t = (double)y / (double)(height - 1);
        // deep royal blue -> lighter SRM blue
        int r = (int)(  0 + t * ( 30 -   0));  // 0   -> 30
        int g = (int)( 51 + t * (120 -  51));  // 51  -> 120
        int b = (int)(120 + t * (200 - 120));  // 120 -> 200

        HBRUSH br = CreateSolidBrush(RGB(r, g, b));
        RECT rcLine{0, y, width, y + 1};
        FillRect(hdc, &rcLine, br);
        DeleteObject(br);
    }
}

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        gHeaderFont = CreateFontA(
            24, 0, 0, 0, FW_BOLD,
            FALSE, FALSE, FALSE,
            DEFAULT_CHARSET,
            OUT_OUTLINE_PRECIS,
            CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY,
            VARIABLE_PITCH,
            "Segoe UI"
        );

        // MAIN HEADING
        hHeaderLabel = CreateWindowA(
            "STATIC",
            "SRMS - Student Record Management System",
            WS_CHILD | WS_VISIBLE | SS_CENTER,
            0, 10, 880, 30,
            hwnd, NULL, NULL, NULL
        );
        if (gHeaderFont)
            SendMessageA(hHeaderLabel, WM_SETFONT, (WPARAM)gHeaderFont, TRUE);

        // Admin ID (in golden colour)
        hAdminIdLabel = CreateWindowA(
            "STATIC",
            ADMIN_ID_DISPLAY,
            WS_CHILD | WS_VISIBLE | SS_RIGHT,
            600, 40, 280, 20,
            hwnd, NULL, NULL, NULL
        );
        if (gHeaderFont)
            SendMessageA(hAdminIdLabel, WM_SETFONT, (WPARAM)gHeaderFont, TRUE);

        // Admin panel
        CreateWindowA("BUTTON", "Admin - Student Records",
                      WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                      20, 70, 400, 260, hwnd, NULL, NULL, NULL);

        int xL = 40, xE = 140, y = 100, h = 22, gap = 28;

        CreateWindowA("STATIC", "Roll:", WS_CHILD | WS_VISIBLE,
                      xL, y, 90, h, hwnd, NULL, NULL, NULL);
        hAdminRoll = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                                   xE, y, 230, h, hwnd, NULL, NULL, NULL);
        y += gap;

        CreateWindowA("STATIC", "Name:", WS_CHILD | WS_VISIBLE,
                      xL, y, 90, h, hwnd, NULL, NULL, NULL);
        hAdminName = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                                   xE, y, 230, h, hwnd, NULL, NULL, NULL);
        y += gap;

        CreateWindowA("STATIC", "Department:", WS_CHILD | WS_VISIBLE,
                      xL, y, 90, h, hwnd, NULL, NULL, NULL);
        hAdminDept = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                                   xE, y, 230, h, hwnd, NULL, NULL, NULL);
        y += gap;

        CreateWindowA("STATIC", "Semester:", WS_CHILD | WS_VISIBLE,
                      xL, y, 90, h, hwnd, NULL, NULL, NULL);
        hAdminSem = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                                  xE, y, 80, h, hwnd, NULL, NULL, NULL);
        y += gap;

        CreateWindowA("STATIC", "CGPA:", WS_CHILD | WS_VISIBLE,
                      xL, y, 90, h, hwnd, NULL, NULL, NULL);
        hAdminCgpa = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                                   xE, y, 80, h, hwnd, NULL, NULL, NULL);

        CreateWindowA("STATIC", "Grade:", WS_CHILD | WS_VISIBLE,
                      xE + 100, y, 60, h, hwnd, NULL, NULL, NULL);
        hAdminGrade = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                                    xE + 160, y, 60, h, hwnd, NULL, NULL, NULL);

        // Admin buttons: Add, Update, Delete, View All
        CreateWindowA("BUTTON", "Add",
                      WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                      30, 270, 80, 28, hwnd, (HMENU)ID_BTN_ADD_STU, NULL, NULL);

        CreateWindowA("BUTTON", "Update",
                      WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                      120, 270, 80, 28, hwnd, (HMENU)ID_BTN_UPDATE_STU, NULL, NULL);

        CreateWindowA("BUTTON", "Delete",
                      WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                      210, 270, 80, 28, hwnd, (HMENU)ID_BTN_DEL_STU, NULL, NULL);

        CreateWindowA("BUTTON", "View All",
                      WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                      300, 270, 80, 28, hwnd, (HMENU)ID_BTN_VIEW_STU, NULL, NULL);

        // Search-by-name panel
        CreateWindowA("BUTTON", "Search / Auto-add by Name",
                      WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                      20, 340, 400, 90, hwnd, NULL, NULL, NULL);

        CreateWindowA("STATIC", "Name:", WS_CHILD | WS_VISIBLE,
                      40, 365, 90, 22, hwnd, NULL, NULL, NULL);
        hSearchNameEdit = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                                        140, 365, 230, 22, hwnd, NULL, NULL, NULL);

        CreateWindowA("BUTTON", "Search / Add",
                      WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                      140, 392, 120, 26, hwnd, (HMENU)ID_BTN_SEARCH_NAME, NULL, NULL);

        // Student - Queries (final layout: both buttons visible)
        CreateWindowA("BUTTON", "Student - Queries",
                      WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                      20, 430, 400, 200, hwnd, NULL, NULL, NULL);

        int sy = 455;          // start a little below the group title
        int h2 = 22;

        CreateWindowA("STATIC", "Roll:", WS_CHILD | WS_VISIBLE,
                      xL, sy, 90, h2, hwnd, NULL, NULL, NULL);
        hStudRoll = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                                  xE, sy, 230, h2, hwnd, NULL, NULL, NULL);
        sy += 28;

        CreateWindowA("STATIC", "Name:", WS_CHILD | WS_VISIBLE,
                      xL, sy, 90, h2, hwnd, NULL, NULL, NULL);
        hStudName = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                                  xE, sy, 230, h2, hwnd, NULL, NULL, NULL);
        sy += 28;

        CreateWindowA("STATIC", "Query:", WS_CHILD | WS_VISIBLE,
                      xL, sy, 90, h2, hwnd, NULL, NULL, NULL);

        // Query box (height 60 so we have clear space for buttons)
        hStudMsg = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER |
                                 ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL,
                                 xE, sy, 230, 60, hwnd, NULL, NULL, NULL);

        // Place BOTH buttons on same line, clearly visible
        int btnY = sy + 65;   // just below the query box

        CreateWindowA("BUTTON", "Submit Query",
                      WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                      40, btnY, 120, 26, hwnd, (HMENU)ID_BTN_ADD_Q, NULL, NULL);

        CreateWindowA("BUTTON", "View All Queries",
                      WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                      190, btnY, 160, 26, hwnd, (HMENU)ID_BTN_VIEW_Q, NULL, NULL);

        // Right-side titles (new)
        hAdminOutLabel = CreateWindowA("STATIC", "Student Records",
                      WS_CHILD | WS_VISIBLE | SS_LEFT,
                      440, 65, 200, 18, hwnd, NULL, NULL, NULL);

        hQueryOutLabel = CreateWindowA("STATIC", "Student Queries",
                      WS_CHILD | WS_VISIBLE | SS_LEFT,
                      440, 345, 200, 18, hwnd, NULL, NULL, NULL);

        // Right-side outputs (white cards with 3D edge, read-only)
        hAdminOutput = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                      WS_CHILD | WS_VISIBLE |
                      ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | ES_READONLY,
                      440, 80, 430, 250, hwnd, NULL, NULL, NULL);

        hStudOutput = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                      WS_CHILD | WS_VISIBLE  |
                      ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | ES_READONLY,
                      440, 360, 430, 250, hwnd, NULL, NULL, NULL);

        return 0;
    }

    case WM_COMMAND: {
        switch (LOWORD(wParam)) {
        case ID_BTN_ADD_STU:     GUI_AddStudent(hwnd);    break;
        case ID_BTN_UPDATE_STU:  GUI_UpdateStudent(hwnd); break;
        case ID_BTN_DEL_STU:     GUI_DeleteStudent(hwnd); break;
        case ID_BTN_VIEW_STU:    GUI_ViewStudents();      break;
        case ID_BTN_SEARCH_NAME: GUI_SearchName(hwnd);    break;
        case ID_BTN_ADD_Q:       GUI_AddQuery(hwnd);      break;
        case ID_BTN_VIEW_Q:      GUI_ViewQueries();   break;
        }
        return 0;
    }

    case WM_ERASEBKGND: {
        // SRM blue gradient header + light body
        HDC hdc = (HDC)wParam;
        RECT rc;
        GetClientRect(hwnd, &rc);

        int headerHeight = 60;
        DrawHeaderGradient(hdc, rc.right - rc.left, headerHeight);

        RECT rcBody = rc;
        rcBody.top = headerHeight;
        FillRect(hdc, &rcBody, gMainBgBrush);
        return 1;
    }

    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        HWND hCtrl = (HWND)lParam;

        if (hCtrl == hHeaderLabel) {
            SetTextColor(hdc, RGB(255, 255, 255));      // white heading
            SetBkMode(hdc, TRANSPARENT);
            return (LRESULT)GetStockObject(NULL_BRUSH);
        }

        if (hCtrl == hAdminIdLabel) {
            SetTextColor(hdc, RGB(218, 165, 32));       // golden admin ID
            SetBkMode(hdc, TRANSPARENT);
            return (LRESULT)GetStockObject(NULL_BRUSH);
        }

        if (hCtrl == hAdminOutLabel || hCtrl == hQueryOutLabel) {
            SetTextColor(hdc, RGB(0, 60, 120));         // dark blue section titles
            SetBkMode(hdc, TRANSPARENT);
            return (LRESULT)GetStockObject(NULL_BRUSH);
        }

        SetBkMode(hdc, TRANSPARENT);
        return (LRESULT)gMainBgBrush;
    }

    case WM_CTLCOLOREDIT: {
        HDC  hdc   = (HDC)wParam;
        HWND hCtrl = (HWND)lParam;

        if (hCtrl == hAdminOutput || hCtrl == hStudOutput) {
            SetBkColor(hdc, RGB(255, 255, 255));        // white cards
            SetTextColor(hdc, RGB(0, 0, 0));
            return (LRESULT)gOutputBgBrush;
        }

        if (hCtrl == hStudMsg) {
            SetBkColor(hdc, RGB(235, 244, 255));        // light blue
            SetTextColor(hdc, RGB(0, 40, 90));
            return (LRESULT)gQueryBgBrush;
        }

        SetBkColor(hdc, RGB(255, 255, 255));
        SetTextColor(hdc, RGB(0, 0, 0));
        return (LRESULT)GetStockObject(WHITE_BRUSH);
    }

    case WM_CTLCOLORBTN: {
        HDC hdc = (HDC)wParam;
        SetBkMode(hdc, TRANSPARENT);
        return (LRESULT)gMainBgBrush;
    }

    case WM_DESTROY:
        if (gHeaderFont)    { DeleteObject(gHeaderFont);    gHeaderFont    = NULL; }
        if (gMainBgBrush)   { DeleteObject(gMainBgBrush);   gMainBgBrush   = NULL; }
        if (gOutputBgBrush) { DeleteObject(gOutputBgBrush); gOutputBgBrush = NULL; }
        if (gQueryBgBrush)  { DeleteObject(gQueryBgBrush);  gQueryBgBrush  = NULL; }
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

// ======================== Login Window Proc ========================

LRESULT CALLBACK LoginWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        CreateWindowA("STATIC", "Admin Login",
                      WS_CHILD | WS_VISIBLE | SS_CENTER,
                      0, 10, 380, 24, hwnd, NULL, NULL, NULL);

        CreateWindowA("STATIC", "Username:", WS_CHILD | WS_VISIBLE,
                      40, 60, 80, 20, hwnd, NULL, NULL, NULL);
        hLoginUserEdit = CreateWindowA("EDIT", "",
                      WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                      140, 60, 200, 20, hwnd, (HMENU)ID_LOGIN_USER_EDIT, NULL, NULL);

        CreateWindowA("STATIC", "Password:", WS_CHILD | WS_VISIBLE,
                      40, 95, 80, 20, hwnd, NULL, NULL, NULL);
        hLoginPassEdit = CreateWindowA("EDIT", "",
                      WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_PASSWORD,
                      140, 95, 200, 20, hwnd, (HMENU)ID_LOGIN_PASS_EDIT, NULL, NULL);

        CreateWindowA("BUTTON", "Login",
                      WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                      90, 140, 80, 26, hwnd, (HMENU)ID_LOGIN_OK, NULL, NULL);

        CreateWindowA("BUTTON", "Cancel",
                      WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                      210, 140, 80, 26, hwnd, (HMENU)ID_LOGIN_CANCEL, NULL, NULL);

        return 0;
    }

    case WM_COMMAND: {
        switch (LOWORD(wParam)) {
        case ID_LOGIN_OK: {
            string user = GetEditText(hLoginUserEdit);
            string pass = GetEditText(hLoginPassEdit);

            if (user == ADMIN_USER && pass == ADMIN_PASS) {
                MessageBoxA(hwnd, "Login successful!", "SRMS", MB_OK | MB_ICONINFORMATION);

                if (gMainHwnd) {
                    ShowWindow(gMainHwnd, SW_SHOW);
                    UpdateWindow(gMainHwnd);
                    SetForegroundWindow(gMainHwnd);
                }

                DestroyWindow(hwnd);   // close login window
            } else {
                MessageBoxA(hwnd, "Invalid credentials. Try again.", "Login Failed",
                            MB_OK | MB_ICONERROR);
                SetWindowTextA(hLoginPassEdit, "");
            }
            return 0;
        }

        case ID_LOGIN_CANCEL:
            if (gMainHwnd) {
                DestroyWindow(gMainHwnd);
            }
            DestroyWindow(hwnd);
            return 0;
        }
        return 0;
    }

    case WM_DESTROY:
        return 0;
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

// ======================== Entry point (normal main) ========================

int main() {
    // hide console window, only GUI visible
    HWND hConsole = GetConsoleWindow();
    if (hConsole) {
        ShowWindow(hConsole, SW_HIDE);
    }

    HINSTANCE hInst = GetModuleHandleA(NULL);
    return WinMain(hInst, NULL, GetCommandLineA(), SW_SHOWNORMAL);
}
