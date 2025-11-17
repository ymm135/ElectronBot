#include <windows.h>
#include <stdint.h>
#include <string>
#include "botdriver_lib.h"

static HWND hEdit;
static HWND hBtnConn;
static HWND hBtnList;
static HWND hBtnXfer;
static void* hBD;

static void Append(HWND h, const std::wstring& t) {
  int len = GetWindowTextLengthW(h);
  std::wstring buf;
  buf.resize(len);
  GetWindowTextW(h, &buf[0], len + 1);
  buf += t;
  SetWindowTextW(h, buf.c_str());
}

static void DoConnect(HWND hWnd) {
  if (!hBD) hBD = BD_Create();
  BD_Init(hBD);
  int ok = BD_Open(hBD, 0x1001, 0x8023);
  if (ok) BD_ProbeEndpoints(hBD);
  Append(hEdit, ok ? L"Connected\r\n" : L"Connect failed\r\n");
}

static void DoList(HWND hWnd) {
  char* s = BD_ListDevices();
  if (!s) { Append(hEdit, L"List failed\r\n"); return; }
  int wlen = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);
  std::wstring w;
  w.resize(wlen);
  MultiByteToWideChar(CP_UTF8, 0, s, -1, &w[0], wlen);
  BD_Free(s);
  Append(hEdit, w);
  Append(hEdit, L"\r\n");
}

static void DoXfer(HWND hWnd) {
  if (!hBD) { Append(hEdit, L"Not connected\r\n"); return; }
  uint8_t tx[5];
  tx[0] = 0x01;
  float f = 1.0f;
  std::memcpy(&tx[1], &f, 4);
  int w = 0;
  int r = 0;
  uint8_t rx[5] = {0};
  int okW = BD_Write(hBD, tx, 5, 1000, &w);
  int okR = BD_Read(hBD, rx, 5, 1000, &r);
  std::wstring s = L"TX " + std::to_wstring(w) + L" bytes, RX " + std::to_wstring(r) + L" bytes\r\n";
  Append(hEdit, s);
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {
    case WM_CREATE: {
      hEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD|WS_VISIBLE|ES_MULTILINE|ES_AUTOVSCROLL|ES_READONLY|WS_VSCROLL, 10, 10, 560, 320, hWnd, (HMENU)100, GetModuleHandleW(NULL), NULL);
      hBtnConn = CreateWindowExW(0, L"BUTTON", L"Connect", WS_CHILD|WS_VISIBLE, 10, 340, 100, 30, hWnd, (HMENU)101, GetModuleHandleW(NULL), NULL);
      hBtnList = CreateWindowExW(0, L"BUTTON", L"List", WS_CHILD|WS_VISIBLE, 120, 340, 100, 30, hWnd, (HMENU)102, GetModuleHandleW(NULL), NULL);
      hBtnXfer = CreateWindowExW(0, L"BUTTON", L"Xfer", WS_CHILD|WS_VISIBLE, 230, 340, 100, 30, hWnd, (HMENU)103, GetModuleHandleW(NULL), NULL);
      break;
    }
    case WM_COMMAND: {
      int id = LOWORD(wParam);
      if (id == 101) DoConnect(hWnd);
      else if (id == 102) DoList(hWnd);
      else if (id == 103) DoXfer(hWnd);
      break;
    }
    case WM_DESTROY: {
      if (hBD) {
        BD_Close(hBD);
        BD_Exit(hBD);
        BD_Destroy(hBD);
        hBD = NULL;
      }
      PostQuitMessage(0);
      break;
    }
    default: return DefWindowProcW(hWnd, msg, wParam, lParam);
  }
  return 0;
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int nCmd) {
  WNDCLASSW wc{};
  wc.lpfnWndProc = WndProc;
  wc.hInstance = hInst;
  wc.lpszClassName = L"BotDriverUI";
  RegisterClassW(&wc);
  HWND hWnd = CreateWindowExW(0, L"BotDriverUI", L"BotDriver UI", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 600, 420, NULL, NULL, hInst, NULL);
  ShowWindow(hWnd, nCmd);
  MSG msg;
  while (GetMessageW(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }
  return 0;
}