#include "resource.h"
#include "framework.h"
#include "projekt 4.h"
#include <objidl.h>
#include <gdiplus.h>
using namespace Gdiplus;
#pragma comment (lib,"Gdiplus.lib")
#include <vector>
#include <ctime>
#include <cmath>
#include <string>


enum class Shape { Circle, Square, Triangle };
enum class ShapeCombination {
    SquareAndCircle,
    SquareAndTriangle,
    CircleAndTriangle
};

ShapeCombination allowedCombination = ShapeCombination::SquareAndCircle;

struct Item {
    Shape shape;
    float x, y;
    int   beltIndex = 0;
    int   segment = 0;
    static constexpr float size = 40.f;
};

Shape RandomShapeForCombination(ShapeCombination combo) {
    switch (combo) {
    case ShapeCombination::SquareAndCircle:
        return (rand() % 2 == 0) ? Shape::Circle : Shape::Square;
    case ShapeCombination::SquareAndTriangle:
        return (rand() % 2 == 0) ? Shape::Square : Shape::Triangle;
    case ShapeCombination::CircleAndTriangle:
        return (rand() % 2 == 0) ? Shape::Circle : Shape::Triangle;
    }
    return Shape::Square;
}
std::vector<Item> items;
const UINT_PTR    TIMER_ID = 1;
UINT  spawnInterval = 450;
float beltSpeed = 3.0f;


const float startX = 1500.f;
const float startY = 475.f;
const float endX = 900.f;
const float endY = 200.f;

const float dx = endX - startX;
const float dy = endY - startY;
const float length = sqrt(dx * dx + dy * dy);
const float dirX = dx / length;
const float dirY = dy / length;

const std::vector<std::vector<PointF>> belts = {
    { PointF(startX, startY), PointF(endX, endY) },
    { PointF(endX, endY), PointF(900.f, 350.f), PointF(450.f, 150.f) },
    { PointF(450.f, 270.f), PointF(0.f, 270.f) }
};

unsigned goodCount = 0, badCount = 0;
DWORD    lastSpawn = 0;

Shape filteredShape = Shape::Square;

bool MoveAlong(Item& it)
{
    if (it.beltIndex >= (int)belts.size())
        return true;

    const auto& path = belts[it.beltIndex];

    if (it.segment >= (int)path.size() - 1) {
        if (it.beltIndex + 1 < (int)belts.size()) {
            const auto& nextPath = belts[it.beltIndex + 1];
            if (!nextPath.empty()) {
                const PointF& nextStart = nextPath.front();

                it.x = path.back().X;
                it.y += beltSpeed;

                if (it.beltIndex == 0 && it.shape == filteredShape && it.y >= nextStart.Y) {
                    it.beltIndex = (int)belts.size();
                    badCount++;
                    return true;
                }

                if (abs(it.x - nextStart.X) < 1.f && abs(it.y - nextStart.Y) < beltSpeed + 1.f) {
                    it.x = nextStart.X;
                    it.y = nextStart.Y;
                    it.beltIndex++;
                    it.segment = 0;

                    if (it.beltIndex == 1 && it.shape != filteredShape)
                        goodCount++;
                }
                return false;
            }
        }
        else {
            it.beltIndex = (int)belts.size();
            return true;
        }
    }

    const PointF& A = path[it.segment];
    const PointF& B = path[it.segment + 1];

    float dx = B.X - A.X;
    float dy = B.Y - A.Y;
    float len = sqrt(dx * dx + dy * dy);
    if (len == 0) return false;

    it.x += (dx / len) * beltSpeed;
    it.y += (dy / len) * beltSpeed;

    float proj = (it.x - A.X) * dx + (it.y - A.Y) * dy;
    if (proj >= len * len) {
        it.x = B.X;
        it.y = B.Y;
        ++it.segment;
    }

    return false;
}

#define MAX_LOADSTRING 100
HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

VOID OnPaint(HDC hdc)
{
    Graphics graphics(hdc);
    SolidBrush background(Color(255, 255, 255, 255));
    graphics.FillRectangle(&background, 0, 0, 2000, 1200);

    SolidBrush fillBrush(Color(255, 0, 200, 255));
    FontFamily fontFamily(L"Arial");

    Point points1[4] = {
        Point(900,200), Point(900,225), Point(1500,475), Point(1500,450),
    };
    graphics.FillPolygon(&fillBrush, points1, 4);

    Point points[4] = {
        Point(1000, 400), Point(1000, 425), Point(450, 175), Point(450, 150)
    };
    graphics.FillPolygon(&fillBrush, points, 4);
    graphics.FillRectangle(&fillBrush, 0, 300, 500, 25);

    SolidBrush circleBrush(Color(255, 30, 170, 255));
    SolidBrush squareBrush(Color(255, 250, 50, 50));
    Pen         outline(Color::Black);

    for (const Item& it : items) {
        RectF r(it.x, it.y, Item::size, Item::size);
        if (it.shape == Shape::Circle) {
            graphics.FillEllipse(&circleBrush, r);
            graphics.DrawEllipse(&outline, r);
        }
        else if (it.shape == Shape::Square) {
            graphics.FillRectangle(&squareBrush, r);
            graphics.DrawRectangle(&outline, r);
        }
        else if (it.shape == Shape::Triangle) {
            PointF triangle[3] = {
                PointF(r.X + r.Width / 2, r.Y),                    
                PointF(r.X + r.Width,     r.Y + r.Height),             
                PointF(r.X,               r.Y + r.Height)             
            };

            SolidBrush triangleBrush(Color(255, 50, 200, 50));
            graphics.FillPolygon(&triangleBrush, triangle, 3);
            graphics.DrawPolygon(&outline, triangle, 3);
        }
    }


    PointF end = belts[0].back();
    const float boxSize = 60.0f;

    RectF filterBox(end.X - boxSize / 2, end.Y - boxSize / 2, boxSize, boxSize);
    SolidBrush filterBrush(Color(255, 255, 230, 150));
    graphics.FillRectangle(&filterBrush, filterBox);
    graphics.DrawRectangle(&outline, filterBox);

    Font filterFont(L"Arial", 12);
    StringFormat sf;
    sf.SetAlignment(StringAlignmentCenter);
    sf.SetLineAlignment(StringAlignmentCenter);
    SolidBrush filterTextBrush(Color::Black);
    graphics.DrawString(L"Filtr", -1, &filterFont, filterBox, &sf, &filterTextBrush);

    std::wstring filterText;
    switch (filteredShape) {
    case Shape::Square:
        filterText = L"Filtruje: Kwadraty (aby zmienić kliknij: opcje)";
        break;
    case Shape::Circle:
        filterText = L"Filtruje: Koła (aby zmienić kliknij: opcje)";
        break;
    case Shape::Triangle:
        filterText = L"Filtruje: Trójkąty (aby zmienić kliknij: opcje)";
        break;
    }
    PointF topLeftPos(200.f, 200.f);
    graphics.DrawString(filterText.c_str(), -1, &filterFont, topLeftPos, &sf, &filterTextBrush);

}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{

    MSG  msg;

    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;

    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_PROJEKT4, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    if (!InitInstance(hInstance, nCmdShow))
        return FALSE;

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_PROJEKT4));

    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    GdiplusShutdown(gdiplusToken);
    return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PROJEKT4));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    wcex.lpszMenuName = MAKEINTRESOURCE(IDC_PROJEKT4);

    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance;
    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0,
        nullptr, nullptr, hInstance, nullptr);
    if (!hWnd) return FALSE;

    HMENU hMenu = LoadMenu(hInstance, MAKEINTRESOURCE(IDC_PROJEKT4));
    if (hMenu) SetMenu(hWnd, hMenu);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        srand((unsigned)time(NULL));
        SetTimer(hWnd, TIMER_ID, 10, NULL);
        lastSpawn = GetTickCount();
        return 0;

    case WM_TIMER:
    {
        DWORD now = GetTickCount();
        if (now - lastSpawn >= spawnInterval) {
            lastSpawn = now;
            Item it;
            it.shape = RandomShapeForCombination(allowedCombination);
            it.x = startX;
            it.y = startY - Item::size;
            items.push_back(it);
        }

        for (auto& it : items)
            MoveAlong(it);

        items.erase(std::remove_if(items.begin(), items.end(),
            [](Item& it) { return it.beltIndex >= (int)belts.size(); }), items.end());

        InvalidateRect(hWnd, nullptr, FALSE);
    }
    return 0;
  

    case WM_INITMENUPOPUP:
    {
        HMENU hMenu = (HMENU)wParam;

        CheckMenuItem(hMenu, IDM_FILTER_SQUARE,
            (filteredShape == Shape::Square) ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(hMenu, IDM_FILTER_CIRCLE,
            (filteredShape == Shape::Circle) ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(hMenu, IDM_FILTER_TRIANGLE,
            (filteredShape == Shape::Triangle) ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(hMenu, IDM_COMBO_SQUARE_CIRCLE,
            (allowedCombination == ShapeCombination::SquareAndCircle) ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(hMenu, IDM_COMBO_SQUARE_TRIANGLE,
            (allowedCombination == ShapeCombination::SquareAndTriangle) ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(hMenu, IDM_COMBO_CIRCLE_TRIANGLE,
            (allowedCombination == ShapeCombination::CircleAndTriangle) ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(hMenu, IDM_SPEED_SLOW, (beltSpeed == 1.5f) ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(hMenu, IDM_SPEED_NORMAL, (beltSpeed == 3.0f) ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(hMenu, IDM_SPEED_FAST, (beltSpeed == 6.0f) ? MF_CHECKED : MF_UNCHECKED);

        CheckMenuItem(hMenu, IDM_SPAWN_SLOW, (spawnInterval == 1000) ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(hMenu, IDM_SPAWN_NORMAL, (spawnInterval == 450) ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(hMenu, IDM_SPAWN_FAST, (spawnInterval == 200) ? MF_CHECKED : MF_UNCHECKED);

        return 0;
    }

    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        switch (wmId)
        {
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        case IDM_FILTER_SQUARE:
            filteredShape = Shape::Square;
            InvalidateRect(hWnd, nullptr, TRUE);
            break;
        case IDM_FILTER_CIRCLE:
            filteredShape = Shape::Circle;
            InvalidateRect(hWnd, nullptr, TRUE);
            break;
        case IDM_FILTER_TRIANGLE:
            filteredShape = Shape::Triangle;
            InvalidateRect(hWnd, nullptr, TRUE);
            break;
        case IDM_COMBO_SQUARE_CIRCLE:
            allowedCombination = ShapeCombination::SquareAndCircle;
            InvalidateRect(hWnd, nullptr, TRUE);
            break;
        case IDM_COMBO_SQUARE_TRIANGLE:
            allowedCombination = ShapeCombination::SquareAndTriangle;
            InvalidateRect(hWnd, nullptr, TRUE);
            break;
        case IDM_COMBO_CIRCLE_TRIANGLE:
            allowedCombination = ShapeCombination::CircleAndTriangle;
            InvalidateRect(hWnd, nullptr, TRUE);
            break;
        case IDM_SPEED_SLOW:
            beltSpeed = 1.5f;
            break;
        case IDM_SPEED_NORMAL:
            beltSpeed = 3.0f;
            break;
        case IDM_SPEED_FAST:
            beltSpeed = 6.0f;
            break;
        case IDM_SPAWN_SLOW:
            spawnInterval = 1000;
            break;
        case IDM_SPAWN_NORMAL:
            spawnInterval = 450;
            break;
        case IDM_SPAWN_FAST:
            spawnInterval = 200;
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    }

    break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        HDC memDC = CreateCompatibleDC(hdc);
        RECT clientRect;
        GetClientRect(hWnd, &clientRect);
        int width = clientRect.right - clientRect.left;
        int height = clientRect.bottom - clientRect.top;

        HBITMAP memBitmap = CreateCompatibleBitmap(hdc, width, height);
        HGDIOBJ oldBitmap = SelectObject(memDC, memBitmap);

        Graphics graphics(memDC);
        OnPaint(memDC);

        BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);
        SelectObject(memDC, oldBitmap);
        DeleteObject(memBitmap);
        DeleteDC(memDC);
        EndPaint(hWnd, &ps);
    }
    return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}


INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}