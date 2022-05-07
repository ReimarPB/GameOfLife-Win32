#include <Windows.h>
#include <Windowsx.h>
#include <strsafe.h>
#include <stdbool.h>
#include <stdio.h>

#define MIN(a, b) (a < b ? a : b)
#define MAX(a, b) (a > b ? a : b)
#define WIDTH     100
#define HEIGHT    50
#define CELL_SIZE 10
#define GEN_TIME  300

bool board[WIDTH][HEIGHT];
bool paused = true;
int generation = 0;

HBRUSH colors[2];

struct Point {
	int x;
	int y;
};

const UINT IDT_GENERATION = 0;

void NextGeneration()
{
	bool newBoard[WIDTH][HEIGHT];

	for (int x = 0; x < WIDTH; x++) {
		for (int y = 0; y < HEIGHT; y++) {
			
			struct Point neighbors[] = {
				{ x - 1, y - 1 },
				{ x, y - 1 },
				{ x - 1, y },
				{ x + 1, y - 1 },
				{ x - 1, y + 1 },
				{ x + 1, y },
				{ x, y + 1 },
				{ x + 1, y + 1 }
			};
			
			int neighborCount = 0;
			for (int i = 0; i < sizeof(neighbors) / sizeof(neighbors[0]); i++) {
				if (
					neighbors[i].x >= 0 && neighbors[i].x < WIDTH &&
					neighbors[i].y >= 0 && neighbors[i].y < HEIGHT &&
					board[neighbors[i].x][neighbors[i].y] == 1
				) {
					neighborCount++;
				}
			}

			if (
				(board[x][y] == 1 && neighborCount == 2) ||
				neighborCount == 3
			) {
				newBoard[x][y] = 1;
			} else {
				newBoard[x][y] = 0;
			}

		}
	}

	memcpy(board, newBoard, sizeof(board));
	generation++;
}

RECT GameToScreenRect(int x, int y)
{
	RECT rect = {
		.left   = x * CELL_SIZE,
		.top    = y * CELL_SIZE,
		.right  = x * CELL_SIZE + CELL_SIZE,
		.bottom = y * CELL_SIZE + CELL_SIZE
	};
	return rect;
}

struct Point ScreenToGamePoint(int x, int y)
{
	x -= (x % CELL_SIZE);
	y -= (y % CELL_SIZE);
	x /= CELL_SIZE;
	y /= CELL_SIZE;
	struct Point point = { x, y };
	return point;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {

		case WM_CREATE: {
			// Get text metrics
			HDC hdc = GetDC(hwnd);
			TEXTMETRIC metrics;
			GetTextMetrics(hdc, &metrics);
			ReleaseDC(hwnd, hdc);

			// Calculate window size
			RECT rect;
			rect.left = 0;
			rect.top = 0;
			rect.right = WIDTH * CELL_SIZE;
			rect.bottom = HEIGHT * CELL_SIZE + metrics.tmHeight;
			AdjustWindowRect(&rect, WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX, false);

			// Set the position
			SetWindowPos(
				hwnd, NULL, 0, 0,
				rect.right - rect.left, rect.bottom - rect.top,
				SWP_NOMOVE | SWP_NOZORDER
			);
		}

		case WM_TIMER: {
			NextGeneration();
			RECT rect;
			GetClientRect(hwnd, &rect);
			InvalidateRect(hwnd, &rect, false);
			return 0;
		}

		case WM_PAINT: {
			RECT updateRect;
			if (!GetUpdateRect(hwnd, &updateRect, false)) return 0;

			RECT clientRect;
			GetClientRect(hwnd, &clientRect);

			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);

			// Create memory DC for double buffering
			HDC hdcMem = CreateCompatibleDC(hdc);
			HBITMAP hbmMem = CreateCompatibleBitmap(hdc, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);
			HDC hbmOld = SelectObject(hdcMem, hbmMem);

			struct Point updateStart = ScreenToGamePoint(updateRect.left, updateRect.top);
			struct Point updateEnd   = ScreenToGamePoint(updateRect.right, updateRect.bottom);

			// Draw cells
			for (int x = MIN(updateStart.x, 0); x < MAX(updateEnd.x, WIDTH); x++) {
				for (int y = MIN(updateStart.y, 0); y < MAX(updateEnd.y, HEIGHT); y++) {
					RECT rect = GameToScreenRect(x, y);
					FillRect(hdcMem, &rect, colors[board[x][y]]);
				}
			}
			
			// Draw status line
			char generationStatus[16];
			StringCbPrintf(generationStatus, sizeof(generationStatus), TEXT("Generation %d"), generation);
			SetTextColor(hdcMem, RGB(255, 255, 255));
			SetBkColor(hdcMem, RGB(0, 0, 0));
			TextOut(hdcMem, 0, HEIGHT * CELL_SIZE, generationStatus, strlen(generationStatus));

			// Copy buffer and clean up
	        BitBlt(hdc, clientRect.left, clientRect.top, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top, hdcMem, clientRect.left, clientRect.top, SRCCOPY);
			SelectObject(hdcMem, hbmOld);
			DeleteObject(hbmMem);
			EndPaint(hwnd, &ps);	
			return 0;
		}

		case WM_LBUTTONDOWN: {
			int x = GET_X_LPARAM(lParam);
			int y = GET_Y_LPARAM(lParam);
			struct Point point = ScreenToGamePoint(x, y);
			
			if (point.x >= 0 && point.x < WIDTH && point.y >= 0 && point.y < HEIGHT) {
				board[point.x][point.y] = !board[point.x][point.y];
				RECT rect = GameToScreenRect(point.x, point.y);
				InvalidateRect(hwnd, &rect, false);
			}
			return 0;
		}

		case WM_KEYDOWN: {
			switch (wParam) {

				case VK_SPACE:
					if (paused) SetTimer(hwnd, IDT_GENERATION, GEN_TIME, NULL);
					else KillTimer(hwnd, IDT_GENERATION);
					paused = !paused;
					break;

			}
			return 0;
		}

		case WM_DESTROY: {
			PostQuitMessage(0);
			return 0;
		}
	}

	return DefWindowProc(hwnd, message, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	LPCSTR name = TEXT("Game of life");

	colors[0] = GetStockObject(BLACK_BRUSH);
	colors[1] = GetStockObject(WHITE_BRUSH);

	WNDCLASS wndclass = {
		.style = CS_HREDRAW | CS_VREDRAW,
		.lpfnWndProc = WndProc,
		.cbClsExtra = 0,
		.cbWndExtra = 0,
		.hInstance = hInstance,
		.hIcon = LoadIcon(NULL, IDI_APPLICATION),
		.hCursor = LoadCursor(NULL, IDC_ARROW),
		.hbrBackground = colors[0],
		.lpszMenuName = NULL,
		.lpszClassName = name
	};

	RegisterClass(&wndclass);

	HWND hwnd = CreateWindow(
		name, name,
		WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME ^ WS_MAXIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT, 0, 0,
		NULL, NULL, hInstance, NULL
	);

	ShowWindow(hwnd, iCmdShow);
	UpdateWindow(hwnd);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

