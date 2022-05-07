#include <Windows.h>
#include <Windowsx.h>
#include <stdbool.h>
#include <stdio.h>

#define WIDTH     100
#define HEIGHT    50
#define CELL_SIZE 10

bool board[WIDTH][HEIGHT];
int generations = 0;

HBRUSH colors[2];

struct Point {
	int x;
	int y;
};

const UINT IDT_GENERATION = 0;

void NextGeneration()
{
	bool newBoard[WIDTH][HEIGHT] = {0}; // TODO 

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
	generations++;
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
	struct Point point = { .x = x, .y = y }; // TODO
	return point;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {

		case WM_CREATE:
			SetTimer(hwnd, IDT_GENERATION, 5000, NULL);	
			return 0;

		case WM_TIMER:
			NextGeneration();
			RECT rect;
			GetClientRect(hwnd, &rect);
			InvalidateRect(hwnd, &rect, false);
			return 0;
			
		case WM_PAINT:
			RECT updateRect;
			if (!GetUpdateRect(hwnd, &updateRect, false)) return 0;

			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);

			struct Point updateStart = ScreenToGamePoint(updateRect.left, updateRect.top);
			struct Point updateEnd   = ScreenToGamePoint(updateRect.right, updateRect.bottom);

			for (int x = updateStart.x; x < updateEnd.x; x++) {
				for (int y = updateStart.y; y < updateEnd.y; y++) {
					RECT rect = GameToScreenRect(x, y);
					FillRect(hdc, &rect, colors[board[x][y]]);
				}
			}

			EndPaint(hwnd, &ps);	
			return 0;

		case WM_LBUTTONDOWN:
			int x = GET_X_LPARAM(lParam);
			int y = GET_Y_LPARAM(lParam);
			struct Point point = ScreenToGamePoint(x, y);
			
			if (point.x >= 0 && point.x < WIDTH && point.y >= 0 && point.y < HEIGHT) {
				board[point.x][point.y] = !board[point.x][point.y];
				RECT rect = GameToScreenRect(point.x, point.y);
				InvalidateRect(hwnd, &rect, false);
			}

			return 0;

		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
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
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		WIDTH * CELL_SIZE, HEIGHT * CELL_SIZE,
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

