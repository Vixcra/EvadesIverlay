#ifndef UNICODE
#define UNICODE
#endif

#define _WIN32_WINNT 0x0500
#include <windows.h>
#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))
#include <gdiplus.h>
#undef min
#undef max
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <deque>
#include <string>
#include <vector>

#pragma comment(lib, "Gdiplus.lib")

using namespace Gdiplus;

const int WINDOW_WIDTH = 350;
const int WINDOW_HEIGHT = 250;
const float KEY_SIZE = 32.5f;
const float KEY_SPACING = 4.0f;
const int TRAIL_LENGTH = 50;
const int MAX_SNOWFLAKES = 30;

const Color COL_BG(255, 0, 255, 0);
const Color COL_KEY_BG_DEFAULT(255, 30, 35, 45); // More opaque (was 220)
const Color COL_KEY_BG_ACTIVE(255, 120, 90, 255);
const Color COL_KEY_BORDER_DEFAULT(255, 70, 75, 90);
const Color COL_KEY_BORDER_ACTIVE(255, 200, 120, 255);
const Color COL_KEY_TEXT_DEFAULT(255, 200, 200, 210);
const Color COL_KEY_TEXT_ACTIVE(255, 255, 255, 255);

const Color COL_TRAIL_START(255, 25, 25, 112);
const Color COL_TRAIL_MID(255, 65, 105, 225);
const Color COL_TRAIL_END(255, 135, 206, 250);

struct KeyButton {
  std::wstring label;
  RectF rect;
  std::vector<int> vKeys;
  bool isPressed;
  float pressAnimation;
  float glowIntensity;

  KeyButton(float x, float y, float w, float h, std::wstring text,
            std::vector<int> keys)
      : label(text), vKeys(keys), isPressed(false), pressAnimation(0.0f),
        glowIntensity(0.0f) {
    rect = RectF(x, y, w, h);
  }
};

struct TrailPoint {
  PointF pos;
  float alpha;
  DWORD timestamp;
};

struct Snowflake {
  PointF pos;
  float size;
  float alpha;
  float rotation;
  float fallSpeed;
  DWORD spawnTime;
  bool active;
};

std::vector<KeyButton> g_Buttons;
std::deque<TrailPoint> g_Trail;
std::vector<Snowflake> g_Snowflakes;
ULONG_PTR gdiplusToken;

int g_ScreenWidth = 0;
int g_ScreenHeight = 0;
Bitmap *g_BackBuffer = nullptr;
float g_RgbHue = 0.0f;

bool IsKeyDownGlobal(int vKey) {
  return (GetAsyncKeyState(vKey) & 0x8000) != 0;
}

Color HSVtoRGB(float h, float s, float v) {
  float c = v * s;
  float x = c * (1.0f - fabs(fmod(h / 60.0f, 2.0f) - 1.0f));
  float m = v - c;

  float r, g, b;
  if (h < 60) {
    r = c;
    g = x;
    b = 0;
  } else if (h < 120) {
    r = x;
    g = c;
    b = 0;
  } else if (h < 180) {
    r = 0;
    g = c;
    b = x;
  } else if (h < 240) {
    r = 0;
    g = x;
    b = c;
  } else if (h < 300) {
    r = x;
    g = 0;
    b = c;
  } else {
    r = c;
    g = 0;
    b = x;
  }

  return Color(255, (BYTE)((r + m) * 255), (BYTE)((g + m) * 255),
               (BYTE)((b + m) * 255));
}

void InitLayout() {
  srand((unsigned int)time(NULL));

  float startX = (WINDOW_WIDTH - (5 * KEY_SIZE + 4 * KEY_SPACING)) / 2.0f;
  float startY = 50;

  auto AddBtn = [&](float x, float y, float w, std::wstring txt,
                    std::vector<int> keys) {
    g_Buttons.emplace_back(x, y, w, KEY_SIZE, txt, keys);
  };

  for (int i = 0; i < 5; i++) {
    AddBtn(startX + i * (KEY_SIZE + KEY_SPACING), startY, KEY_SIZE,
           std::to_wstring(i + 1), {'1' + i});
  }

  float r3y = startY + 2 * (KEY_SIZE + KEY_SPACING);
  float shiftW = KEY_SIZE * 1.6f;
  float r3StartX =
      (WINDOW_WIDTH - (shiftW + KEY_SPACING + 3 * KEY_SIZE + 2 * KEY_SPACING)) /
      2.0f;

  float offset = shiftW + KEY_SPACING;
  float posShift = r3StartX;
  float posA = r3StartX + offset;
  float posS = r3StartX + offset + 1 * (KEY_SIZE + KEY_SPACING);
  float posD = r3StartX + offset + 2 * (KEY_SIZE + KEY_SPACING);

  AddBtn(posShift, r3y, shiftW, L"SHIFT", {VK_LSHIFT, VK_RSHIFT});
  AddBtn(posA, r3y, KEY_SIZE, L"A", {'A', VK_LEFT});  // Arrow Left
  AddBtn(posS, r3y, KEY_SIZE, L"S", {'S', VK_DOWN});  // Arrow Down
  AddBtn(posD, r3y, KEY_SIZE, L"D", {'D', VK_RIGHT}); // Arrow Right

  float r2y = startY + KEY_SIZE + KEY_SPACING;
  float zxStartX = posA - (KEY_SIZE + KEY_SPACING);
  // C is to the left of Z
  AddBtn(zxStartX - (KEY_SIZE + KEY_SPACING), r2y, KEY_SIZE, L"C", {'C'});
  AddBtn(zxStartX, r2y, KEY_SIZE, L"Z", {'Z', 'J'});
  AddBtn(zxStartX + (KEY_SIZE + KEY_SPACING), r2y, KEY_SIZE, L"X", {'X', 'K'});
  AddBtn(posS, r2y, KEY_SIZE, L"W", {'W', VK_UP}); // Arrow Up

  g_Snowflakes.resize(MAX_SNOWFLAKES);
  for (auto &snow : g_Snowflakes) {
    snow.active = false;
  }
}

Color BlendColors(const Color &c1, const Color &c2, float t) {
  return Color((BYTE)(c1.GetA() * (1 - t) + c2.GetA() * t),
               (BYTE)(c1.GetR() * (1 - t) + c2.GetR() * t),
               (BYTE)(c1.GetG() * (1 - t) + c2.GetG() * t),
               (BYTE)(c1.GetB() * (1 - t) + c2.GetB() * t));
}

Color BlendColors3(const Color &c1, const Color &c2, const Color &c3, float t) {
  if (t < 0.5f) {
    return BlendColors(c1, c2, t * 2.0f);
  } else {
    return BlendColors(c2, c3, (t - 0.5f) * 2.0f);
  }
}

void UpdateLogic(HWND hwnd) {
  DWORD currentTime = GetTickCount();

  g_RgbHue += 2.0f;
  if (g_RgbHue >= 360.0f)
    g_RgbHue -= 360.0f;

  for (auto &btn : g_Buttons) {
    bool active = false;
    for (int vk : btn.vKeys) {
      if (IsKeyDownGlobal(vk)) {
        active = true;
        break;
      }
    }

    bool wasPressed = btn.isPressed;
    btn.isPressed = active;

    if (btn.isPressed) {
      btn.pressAnimation = 1.0f; // Instant activation
      if (!wasPressed)
        btn.glowIntensity = 1.0f;
    } else {
      btn.pressAnimation = (btn.pressAnimation - 0.1f) > 0.0f
                               ? (btn.pressAnimation - 0.1f)
                               : 0.0f;
    }

    btn.glowIntensity =
        (btn.glowIntensity - 0.05f) > 0.0f ? (btn.glowIntensity - 0.05f) : 0.0f;
  }

  POINT p;
  GetCursorPos(&p);

  float virtualX = ((float)p.x / (float)g_ScreenWidth) * WINDOW_WIDTH;
  float virtualY = ((float)p.y / (float)g_ScreenHeight) * WINDOW_HEIGHT;

  float margin = 8;
  if (virtualX < margin)
    virtualX = margin;
  if (virtualX > WINDOW_WIDTH - margin)
    virtualX = WINDOW_WIDTH - margin;
  if (virtualY < margin)
    virtualY = margin;
  if (virtualY > WINDOW_HEIGHT - margin)
    virtualY = WINDOW_HEIGHT - margin;

  g_Trail.push_front({PointF(virtualX, virtualY), 255.0f, currentTime});

  if (rand() % 100 < 40) {
    for (auto &snow : g_Snowflakes) {
      if (!snow.active) {
        snow.pos = PointF(virtualX + (rand() % 40 - 20),
                          virtualY + (rand() % 40 - 20));
        snow.size = 2.0f + (rand() % 4);
        snow.alpha = 255.0f;
        snow.rotation = (float)(rand() % 360);
        snow.fallSpeed = 0.3f + (rand() % 10) / 10.0f;
        snow.spawnTime = currentTime;
        snow.active = true;
        break;
      }
    }
  }

  for (auto &snow : g_Snowflakes) {
    if (snow.active) {
      snow.pos.Y += snow.fallSpeed;
      snow.pos.X += sin(snow.rotation * 3.14159f / 180.0f) * 0.3f;
      snow.rotation += 2.0f;

      float age = (currentTime - snow.spawnTime) / 1500.0f;
      snow.alpha = 255.0f * (1.0f - age);

      if (age > 1.0f || snow.pos.Y > WINDOW_HEIGHT) {
        snow.active = false;
      }
    }
  }

  if (g_Trail.size() > TRAIL_LENGTH) {
    g_Trail.pop_back();
  }

  for (auto &tp : g_Trail) {
    float age = (currentTime - tp.timestamp) / 800.0f;
    tp.alpha = (255.0f * (1.0f - age)) > 0.0f ? (255.0f * (1.0f - age)) : 0.0f;
  }
}

void DrawRoundedRect(Graphics *g, Brush *brush, const RectF &rect,
                     float radius) {
  GraphicsPath path;
  path.AddArc(rect.X, rect.Y, radius * 2, radius * 2, 180, 90);
  path.AddArc(rect.X + rect.Width - radius * 2, rect.Y, radius * 2, radius * 2,
              270, 90);
  path.AddArc(rect.X + rect.Width - radius * 2,
              rect.Y + rect.Height - radius * 2, radius * 2, radius * 2, 0, 90);
  path.AddArc(rect.X, rect.Y + rect.Height - radius * 2, radius * 2, radius * 2,
              90, 90);
  path.CloseFigure();
  g->FillPath(brush, &path);
}

void DrawRoundedRectOutline(Graphics *g, Pen *pen, const RectF &rect,
                            float radius) {
  GraphicsPath path;
  path.AddArc(rect.X, rect.Y, radius * 2, radius * 2, 180, 90);
  path.AddArc(rect.X + rect.Width - radius * 2, rect.Y, radius * 2, radius * 2,
              270, 90);
  path.AddArc(rect.X + rect.Width - radius * 2,
              rect.Y + rect.Height - radius * 2, radius * 2, radius * 2, 0, 90);
  path.AddArc(rect.X, rect.Y + rect.Height - radius * 2, radius * 2, radius * 2,
              90, 90);
  path.CloseFigure();
  g->DrawPath(pen, &path);
}

void Render() {
  if (!g_BackBuffer)
    return;

  Graphics g(g_BackBuffer);
  g.SetSmoothingMode(SmoothingModeAntiAlias);
  g.SetTextRenderingHint(TextRenderingHintAntiAlias);

  g.Clear(COL_BG);

  Color rgbColor = HSVtoRGB(g_RgbHue, 1.0f, 1.0f);
  Pen rgbPen(rgbColor, 3.0f);
  g.DrawRectangle(&rgbPen, 1, 1, WINDOW_WIDTH - 3, WINDOW_HEIGHT - 3);

  Color rgbGlow = HSVtoRGB(g_RgbHue, 1.0f, 0.8f);
  rgbGlow = Color(60, rgbGlow.GetR(), rgbGlow.GetG(), rgbGlow.GetB());
  Pen rgbGlowPen(rgbGlow, 5.0f);
  g.DrawRectangle(&rgbGlowPen, 3, 3, WINDOW_WIDTH - 7, WINDOW_HEIGHT - 7);

  FontFamily titleFamily(L"Segoe UI");
  Font titleFont(&titleFamily, 18, FontStyleBold, UnitPixel);
  StringFormat centerFormat;
  centerFormat.SetAlignment(StringAlignmentCenter);
  centerFormat.SetLineAlignment(StringAlignmentCenter);

  LinearGradientBrush titleBrush(
      PointF(0, 10), PointF((float)WINDOW_WIDTH, 35),
      HSVtoRGB(g_RgbHue, 1.0f, 1.0f),
      HSVtoRGB(fmod(g_RgbHue + 120.0f, 360.0f), 1.0f, 1.0f));

  RectF titleRect(0, 8, (float)WINDOW_WIDTH, 30);

  for (int i = 3; i > 0; i--) {
    Color glowColor = HSVtoRGB(g_RgbHue, 1.0f, 1.0f);
    glowColor = Color((BYTE)(30 * i), glowColor.GetR(), glowColor.GetG(),
                      glowColor.GetB());
    SolidBrush glowBrush(glowColor);
    RectF glowRect = titleRect;
    glowRect.Inflate((float)i, (float)i);
    g.DrawString(L"VIXINO", -1, &titleFont, glowRect, &centerFormat,
                 &glowBrush);
  }

  g.DrawString(L"VIXINO", -1, &titleFont, titleRect, &centerFormat,
               &titleBrush);

  // Draw keys FIRST (so trail/cursor can be on top)
  FontFamily fontFamily(L"Segoe UI");
  Font font(&fontFamily, 10, FontStyleBold, UnitPixel);
  Font smallFont(&fontFamily, 8, FontStyleBold, UnitPixel);

  int keyIndex = 0;
  for (auto &btn : g_Buttons) {
    float radius = 4.0f;

    // RGB ONLY for borders (rainbow wave)
    float keyHue = fmod(g_RgbHue + (keyIndex * 30.0f), 360.0f);
    Color borderColor = HSVtoRGB(keyHue, 1.0f, 1.0f);

    // NORMAL colors for background and text
    Color bgColor =
        BlendColors(COL_KEY_BG_DEFAULT, COL_KEY_BG_ACTIVE, btn.pressAnimation);
    Color textColor = BlendColors(COL_KEY_TEXT_DEFAULT, COL_KEY_TEXT_ACTIVE,
                                  btn.pressAnimation);

    if (btn.glowIntensity > 0.01f) {
      for (int i = 2; i > 0; i--) {
        RectF glowRect = btn.rect;
        float expand = i * 2.0f * btn.glowIntensity;
        glowRect.Inflate(expand, expand);
        Color glowColor = HSVtoRGB(keyHue, 1.0f, 1.0f);
        glowColor = Color((BYTE)(60 * btn.glowIntensity), glowColor.GetR(),
                          glowColor.GetG(), glowColor.GetB());
        SolidBrush glowBrush(glowColor);
        DrawRoundedRect(&g, &glowBrush, glowRect, radius + expand / 2);
      }
    }

    Color bgDark =
        Color(bgColor.GetA(), (BYTE)(bgColor.GetR() * 0.7f),
              (BYTE)(bgColor.GetG() * 0.7f), (BYTE)(bgColor.GetB() * 0.7f));
    LinearGradientBrush keyBrush(
        PointF(btn.rect.X, btn.rect.Y),
        PointF(btn.rect.X, btn.rect.Y + btn.rect.Height), bgColor, bgDark);

    DrawRoundedRect(&g, &keyBrush, btn.rect, radius);

    // RGB Border
    Pen borderPen(borderColor, btn.isPressed ? 2.5f : 1.5f);
    DrawRoundedRectOutline(&g, &borderPen, btn.rect, radius);

    if (btn.isPressed) {
      RectF highlightRect = btn.rect;
      highlightRect.Inflate(-1.5f, -1.5f);
      Pen highlightPen(Color(80, 255, 255, 255), 0.8f);
      DrawRoundedRectOutline(&g, &highlightPen, highlightRect, radius - 1);
    }

    SolidBrush textBrush(textColor);
    Font *useFont = (btn.label == L"SHIFT") ? &smallFont : &font;
    g.DrawString(btn.label.c_str(), -1, useFont, btn.rect, &centerFormat,
                 &textBrush);

    keyIndex++;
  }

  // NOW Draw trail ON TOP of keys
  if (g_Trail.size() >= 2) {
    for (size_t i = 0; i < g_Trail.size() - 1; i++) {
      PointF p1 = g_Trail[i].pos;
      PointF p2 = g_Trail[i + 1].pos;

      float t = (float)i / (float)g_Trail.size();
      Color trailColor =
          BlendColors3(COL_TRAIL_START, COL_TRAIL_MID, COL_TRAIL_END, t);
      BYTE finalAlpha = (BYTE)(g_Trail[i].alpha);
      trailColor = Color(finalAlpha, trailColor.GetR(), trailColor.GetG(),
                         trailColor.GetB());

      float sparkle = (sin((float)i * 0.5f + g_RgbHue * 0.1f) + 1.0f) / 2.0f;
      float width = (4.0f + sparkle * 2.0f) * (1.0f - t * 0.5f);

      Pen pen(trailColor, width);
      pen.SetStartCap(LineCapRound);
      pen.SetEndCap(LineCapRound);

      g.DrawLine(&pen, p1, p2);
    }
  }

  // Draw snowflakes ON TOP
  for (const auto &snow : g_Snowflakes) {
    if (snow.active) {
      Color snowColor(snow.alpha, 255, 255, 255);
      SolidBrush snowBrush(snowColor);

      float size = snow.size;
      PointF center = snow.pos;

      g.FillEllipse(&snowBrush, center.X - size, center.Y - size / 4, size * 2,
                    size / 2);
      g.FillEllipse(&snowBrush, center.X - size / 4, center.Y - size, size / 2,
                    size * 2);
      g.FillEllipse(&snowBrush, center.X - size * 0.7f, center.Y - size * 0.7f,
                    size * 1.4f, size * 1.4f);
    }
  }

  // Draw cursor ON TOP
  if (!g_Trail.empty()) {
    PointF cursorPos = g_Trail.front().pos;
    float cursorSize = 10.0f;
    RectF cursorRect(cursorPos.X - cursorSize / 2, cursorPos.Y - cursorSize / 2,
                     cursorSize, cursorSize);

    for (int i = 3; i > 0; i--) {
      float expand = i * 2.0f;
      RectF glowRect = cursorRect;
      glowRect.Inflate(expand, expand);
      Color glowColor(60, COL_TRAIL_MID.GetR(), COL_TRAIL_MID.GetG(),
                      COL_TRAIL_MID.GetB());
      SolidBrush glowBrush(glowColor);
      g.FillEllipse(&glowBrush, glowRect);
    }

    LinearGradientBrush cursorBrush(PointF(cursorRect.X, cursorRect.Y),
                                    PointF(cursorRect.X + cursorRect.Width,
                                           cursorRect.Y + cursorRect.Height),
                                    COL_TRAIL_START, COL_TRAIL_END);
    g.FillEllipse(&cursorBrush, cursorRect);

    Pen cursorPen(Color(255, 255, 255, 255), 1.5f);
    g.DrawEllipse(&cursorPen, cursorRect);
  }
}

void Draw(HWND hwnd) {
  PAINTSTRUCT ps;
  HDC hdc = BeginPaint(hwnd, &ps);

  Render();

  if (g_BackBuffer) {
    Graphics g(hdc);
    g.DrawImage(g_BackBuffer, 0, 0);
  }

  EndPaint(hwnd, &ps);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                            LPARAM lParam) {
  switch (uMsg) {
  case WM_PAINT:
    Draw(hwnd);
    return 0;
  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;
  case WM_NCHITTEST: {
    POINT pt = {LOWORD(lParam), HIWORD(lParam)};
    ScreenToClient(hwnd, &pt);
    if (pt.y < 30)
      return HTCAPTION;
  }
    return HTTRANSPARENT;
  case WM_KEYDOWN:
    if (wParam == VK_ESCAPE)
      PostQuitMessage(0);
    return 0;
  case WM_ERASEBKGND:
    return 1;
  case WM_ACTIVATEAPP:
    if (wParam == 0) {
      SetWindowPos(hwnd, HWND_BOTTOM, 0, 0, 0, 0,
                   SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    } else {
      SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                   SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
    return 0;
  }
  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {
  g_ScreenWidth = GetSystemMetrics(SM_CXSCREEN);
  g_ScreenHeight = GetSystemMetrics(SM_CYSCREEN);

  GdiplusStartupInput gdiplusStartupInput;
  GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

  g_BackBuffer = new Bitmap(WINDOW_WIDTH, WINDOW_HEIGHT, PixelFormat32bppARGB);

  const wchar_t CLASS_NAME[] = L"KeyOverlayClass";
  WNDCLASSW wc = {};
  wc.lpfnWndProc = WindowProc;
  wc.hInstance = hInstance;
  wc.lpszClassName = CLASS_NAME;
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = NULL;
  RegisterClassW(&wc);

  HWND hwnd =
      CreateWindowExW(WS_EX_TOPMOST | WS_EX_TRANSPARENT, CLASS_NAME,
                      L"Vixino Overlay", WS_POPUP, 100, 100, WINDOW_WIDTH,
                      WINDOW_HEIGHT, NULL, NULL, hInstance, NULL);

  if (!hwnd)
    return 0;

  ShowWindow(hwnd, nCmdShow);
  InitLayout();

  SetTimer(hwnd, 1, 16, NULL);

  MSG msg = {};
  while (GetMessage(&msg, NULL, 0, 0)) {
    if (msg.message == WM_TIMER) {
      UpdateLogic(hwnd);
      InvalidateRect(hwnd, NULL, FALSE);
    }
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  delete g_BackBuffer;
  GdiplusShutdown(gdiplusToken);
  return 0;
}