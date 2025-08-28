#define UNICODE
#include <windows.h>
#include <stdio.h> // For wsprintf

// --- Configuration ---
#define ZOOM_FACTOR 2.0f
#define UPDATE_INTERVAL_MS 30 // Refresh rate for the zoomed view (approx 33 FPS)

// --- Global Variables ---
LPCWSTR g_szSelectionClassName = L"ScreenSelectionClass";
LPCWSTR g_szZoomClassName = L"ZoomWindowClass";

RECT g_selectionRect;       // The rectangle the user draws
BOOL g_isSelecting = FALSE; // Flag to check if the user is currently dragging the mouse
HWND g_hSelectionWnd = NULL;
HWND g_hZoomWnd = NULL;

// --- Function Declarations ---
LRESULT CALLBACK SelectionWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ZoomWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void CreateZoomWindow(HINSTANCE hInstance);
void NormalizeRect(RECT* r);

// --- Entry Point ---
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSEX wc_select = {0}, wc_zoom = {0};
    MSG Msg;

    // 1. Register the Selection Window Class
    wc_select.cbSize        = sizeof(WNDCLASSEX);
    wc_select.lpfnWndProc   = SelectionWndProc;
    wc_select.hInstance     = hInstance;
    wc_select.hCursor       = LoadCursor(NULL, IDC_CROSS);
    wc_select.lpszClassName = g_szSelectionClassName;
    if (!RegisterClassEx(&wc_select)) return 0;

    // 2. Register the Zoom Window Class
    wc_zoom.cbSize        = sizeof(WNDCLASSEX);
    wc_zoom.lpfnWndProc   = ZoomWndProc;
    wc_zoom.hInstance     = hInstance;
    wc_zoom.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc_zoom.hbrBackground = (HBRUSH)(COLOR_WINDOW);
    wc_zoom.lpszClassName = g_szZoomClassName;
    wc_zoom.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    if (!RegisterClassEx(&wc_zoom)) return 0;
    
    // Get screen dimensions
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // 3. Create the Selection Window
    g_hSelectionWnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED,
        g_szSelectionClassName, L"Select an area", WS_POPUP,
        0, 0, screenWidth, screenHeight, NULL, NULL, hInstance, NULL);

    if (g_hSelectionWnd == NULL) return 0;

    SetLayeredWindowAttributes(g_hSelectionWnd, 0, 128, LWA_ALPHA);
    ShowWindow(g_hSelectionWnd, nCmdShow);
    UpdateWindow(g_hSelectionWnd);

    // 4. The Message Loop
    while(GetMessage(&Msg, NULL, 0, 0) > 0) {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
    return Msg.wParam;
}

// --- Window Procedure for the Selection Overlay ---
LRESULT CALLBACK SelectionWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch(msg) {
        case WM_LBUTTONDOWN:
            g_isSelecting = TRUE;
            SetCapture(hwnd);
            g_selectionRect.left = LOWORD(lParam);
            g_selectionRect.top = HIWORD(lParam);
            g_selectionRect.right = LOWORD(lParam);
            g_selectionRect.bottom = HIWORD(lParam);
            return 0;

        case WM_MOUSEMOVE:
            if (g_isSelecting) {
                g_selectionRect.right = LOWORD(lParam);
                g_selectionRect.bottom = HIWORD(lParam);
                InvalidateRect(hwnd, NULL, TRUE);
            }
            return 0;

        case WM_LBUTTONUP:
            g_isSelecting = FALSE;
            ReleaseCapture();
            ShowWindow(hwnd, SW_HIDE);

            NormalizeRect(&g_selectionRect);

            if (g_selectionRect.right - g_selectionRect.left > 5 &&
                g_selectionRect.bottom - g_selectionRect.top > 5) {
                CreateZoomWindow(GetModuleHandle(NULL));
            } else {
                PostQuitMessage(0);
            }
            return 0;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            // Redraw transparent background (needed for InvalidateRect)
            RECT clientRect;
            GetClientRect(hwnd, &clientRect);
            HBRUSH transparentBrush = CreateSolidBrush(RGB(0,0,0)); // Color doesn't matter with layered windows
            FillRect(hdc, &clientRect, transparentBrush);
            DeleteObject(transparentBrush);

            if (g_isSelecting) {
                HPEN hPen = CreatePen(PS_DASH, 1, RGB(255, 255, 255));
                SelectObject(hdc, hPen);
                SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
                Rectangle(hdc, g_selectionRect.left, g_selectionRect.top, g_selectionRect.right, g_selectionRect.bottom);
                DeleteObject(hPen);
            }
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) PostQuitMessage(0);
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

// --- Window Procedure for the Final Zoomed Window ---
LRESULT CALLBACK ZoomWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            SetTimer(hwnd, 1, UPDATE_INTERVAL_MS, NULL);
            return 0;


        case WM_TIMER:
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            int sourceWidth = g_selectionRect.right - g_selectionRect.left;
            int sourceHeight = g_selectionRect.bottom - g_selectionRect.top;

            HDC hdcScreen = GetDC(NULL);

            // Use a better stretching mode for smoother results
            SetStretchBltMode(hdc, HALFTONE);
            StretchBlt(hdc, 0, 0, (int)(sourceWidth * ZOOM_FACTOR), (int)(sourceHeight * ZOOM_FACTOR),
                       hdcScreen, g_selectionRect.left, g_selectionRect.top, sourceWidth, sourceHeight,
                       SRCCOPY);

            ReleaseDC(NULL, hdcScreen);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) {
                DestroyWindow(hwnd); // Proper way to close, triggers WM_DESTROY
            }
            return 0;

        case WM_DESTROY:
            KillTimer(hwnd, 1);
            PostQuitMessage(0);
            return 0;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

// --- Helper Functions ---
void CreateZoomWindow(HINSTANCE hInstance) {
    int sourceWidth = g_selectionRect.right - g_selectionRect.left;
    int sourceHeight = g_selectionRect.bottom - g_selectionRect.top;
    int zoomWidth = (int)(sourceWidth * ZOOM_FACTOR);
    int zoomHeight = (int)(sourceHeight * ZOOM_FACTOR);

    // Create a more descriptive window title
    WCHAR windowTitle[100];
    wsprintfW(windowTitle, L"Live Zoom of (%d, %d) - Size %dx%d", g_selectionRect.left, g_selectionRect.top, sourceWidth, sourceHeight);

    g_hZoomWnd = CreateWindowEx(
        WS_EX_TOPMOST, g_szZoomClassName, windowTitle,
        WS_OVERLAPPEDWINDOW | WS_VISIBLE, // Added WS_VISIBLE
        CW_USEDEFAULT, CW_USEDEFAULT,
        zoomWidth, zoomHeight,
        NULL, NULL, hInstance, NULL);
}

void NormalizeRect(RECT* r) {
    if (r->left > r->right) {
        int temp = r->left; r->left = r->right; r->right = temp;
    }
    if (r->top > r->bottom) {
        int temp = r->top; r->top = r->bottom; r->bottom = temp;
    }
}