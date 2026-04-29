extern def sin(x: f64) -> f64
extern def cos(x: f64) -> f64
extern def tan(x: f64) -> f64
extern def rand() -> int
extern def malloc(size: int) -> *int
extern def free(ptr: *int) -> void

extern def GetConsoleWindow() -> *void
extern def ShowWindow(hWnd: *void, nCmdShow: int) -> int
extern def CreateWindowExA(dwExStyle: int, lpClassName: *void, lpWindowName: *void, dwStyle: int, x: int, y: int, nWidth: int, nHeight: int, hWndParent: *void, hMenu: *void, hInstance: *void, lpParam: *void) -> *void
extern def GetDC(hwnd: *void) -> *void
extern def ReleaseDC(hwnd: *void, hdc: *void) -> int
extern def PeekMessageA(lpMsg: *void, hWnd: *void, wMsgFilterMin: int, wMsgFilterMax: int, wRemoveMsg: int) -> int
extern def DispatchMessageA(lpMsg: *void) -> int
extern def GetAsyncKeyState(vKey: int) -> i16
extern def Sleep(dwMilliseconds: int) -> void
extern def PlaySoundA(pszSound: *void, hmod: *void, fdwSound: int) -> int
extern def ExitProcess(uExitCode: int) -> void
extern def GetTickCount() -> int

extern def GetCursorPos(lpPoint: *void) -> int
extern def SetCursorPos(x: int, y: int) -> int
extern def ClientToScreen(hWnd: *void, lpPoint: *void) -> int
extern def ShowCursor(bShow: int) -> int
extern def SetBkMode(hdc: *void, mode: int) -> int
extern def SetTextColor(hdc: *void, color: int) -> int
extern def TextOutA(hdc: *void, x: int, y: int, lpString: *void, c: int) -> int

extern def ChoosePixelFormat(hdc: *void, ppfd: *int) -> int
extern def SetPixelFormat(hdc: *void, format: int, ppfd: *int) -> int
extern def wglCreateContext(hdc: *void) -> *void
extern def wglMakeCurrent(hdc: *void, hglrc: *void) -> int
extern def SwapBuffers(hdc: *void) -> int

extern def glClearColor(red: float, green: float, blue: float, alpha: float) -> void
extern def glClear(mask: int) -> void
extern def glEnable(cap: int) -> void
extern def glDisable(cap: int) -> void
extern def glBlendFunc(sfactor: int, dfactor: int) -> void
extern def glMatrixMode(mode: int) -> void
extern def glLoadIdentity() -> void
extern def glPushMatrix() -> void
extern def glPopMatrix() -> void
extern def glTranslated(x: f64, y: f64, z: f64) -> void
extern def glRotated(angle: f64, x: f64, y: f64, z: f64) -> void
extern def glFrustum(left: f64, right: f64, bottom: f64, top: f64, zNear: f64, zFar: f64) -> void
extern def glOrtho(left: f64, right: f64, bottom: f64, top: f64, near: f64, far: f64) -> void
extern def glColor3d(red: f64, green: f64, blue: f64) -> void
extern def glColor4d(red: f64, green: f64, blue: f64, alpha: f64) -> void
extern def glVertex3d(x: f64, y: f64, z: f64) -> void
extern def glGenTextures(n: int, textures: *int) -> void
extern def glBindTexture(target: int, texture: int) -> void
extern def glTexParameteri(target: int, pname: int, param: int) -> void
extern def glTexImage2D(target: int, level: int, internalformat: int, width: int, height: int, border: int, format: int, type: int, pixels: *int) -> void
extern def glBegin(mode: int) -> void
extern def glEnd() -> void
extern def glTexCoord2d(s: f64, t: f64) -> void
extern def glFogi(pname: int, param: int) -> void
extern def glFogf(pname: int, param: float) -> void

extern def PostQuitMessage(nExitCode: int) -> void
extern def DefWindowProcA(hWnd: *void, Msg: int, wParam: *void, lParam: *void) -> int
extern def GetModuleHandleA(lpModuleName: *void) -> *void
extern def RegisterClassA(lpWndClass: *void) -> int
extern def GetSystemMetrics(nIndex: int) -> int
extern def SetWindowLongA(hWnd: *void, nIndex: int, dwNewLong: int) -> int
extern def SetWindowPos(hWnd: *void, hWndInsertAfter: *void, X: int, Y: int, cx: int, cy: int, uFlags: int) -> int
extern def glViewport(x: int, y: int, width: int, height: int) -> void
