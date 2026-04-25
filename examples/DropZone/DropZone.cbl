struct POINT:
    x: i32
    y: i32

struct MSG:
    hwnd: *void
    message: u32
    wParam: u64
    lParam: i64
    time: u32
    pt: POINT

struct RECT:
    left: i32
    top: i32
    right: i32
    bottom: i32

struct WNDCLASSEXA:
    cbSize: u32
    style: u32
    lpfnWndProc: *void
    cbClsExtra: i32
    cbWndExtra: i32
    hInstance: *void
    hIcon: *void
    hCursor: *void
    hbrBackground: *void
    lpszMenuName: *u8
    lpszClassName: *u8
    hIconSm: *void

struct ACCENT_POLICY:
    AccentState: i32
    AccentFlags: i32
    GradientColor: u32
    AnimationId: i32

struct WINDOWCOMPOSITIONATTRIBDATA:
    Attrib: i32
    pvData: *void
    cbData: u32

const SM_CXSCREEN: i32 = 0
const SM_CYSCREEN: i32 = 1
const SWP_NOZORDER: u32 = 4
const SW_SHOW: i32 = 5
const WM_DESTROY: u32 = 2
const WM_PAINT: u32 = 15
const WM_TIMER: u32 = 275
const WM_DROPFILES: u32 = 563
const WM_LBUTTONDOWN: u32 = 513
const WM_RBUTTONDOWN: u32 = 516

const WS_EX_STYLE: u32 = 524424     
const WS_MAIN_STYLE: u32 = 2147483648 

g_wc: WNDCLASSEXA = {0}
g_msg: MSG = {0}
g_pt: POINT = {0}
g_rc: RECT = {0} 

g_hover_time: i32 = 0
g_is_triggered: i32 = 0
g_shelf_current_x: i32 = 0
g_screen_w: i32 = 0
g_screen_h: i32 = 0

g_tile_count: i32 = 0
g_icons: *u64 = 0 as *u64
g_paths: *u8 = 0 as *u8

extern def GetProcessHeap() -> *void
extern def HeapAlloc(hHeap: *void, dwFlags: u32, dwBytes: u64) -> *u8
extern def HeapFree(hHeap: *void, dwFlags: u32, lpMem: *void) -> i32
extern def wsprintfA(buf: *u8, format: *u8, val: i32) -> i32
extern def lstrcpyA(lpString1: *u8, lpString2: *u8) -> *u8
extern def lstrlenA(lpString: *u8) -> i32

extern def GetModuleHandleA(lpModuleName: *u8) -> *void
extern def RegisterClassExA(lpwcx: *void) -> u16
extern def CreateWindowExA(dwExStyle: u32, lpClassName: *u8, lpWindowName: *u8, dwStyle: u32, X: i32, Y: i32, nWidth: i32, nHeight: i32, hWndParent: *void, hMenu: *void, hInstance: *void, lpParam: *void) -> *void
extern def SetWindowPos(hWnd: *void, hWndInsertAfter: *void, X: i32, Y: i32, cx: i32, cy: i32, uFlags: u32) -> i32
extern def ShowWindow(hWnd: *void, nCmdShow: i32) -> i32
extern def GetMessageA(lpMsg: *void, hWnd: *void, wMsgFilterMin: u32, wMsgFilterMax: u32) -> i32
extern def TranslateMessage(lpMsg: *void) -> i32
extern def DispatchMessageA(lpMsg: *void) -> i64
extern def DefWindowProcA(hWnd: *void, Msg: u32, wParam: u64, lParam: i64) -> i64
extern def PostQuitMessage(nExitCode: i32) -> void
extern def GetSystemMetrics(nIndex: i32) -> i32
extern def SetTimer(hWnd: *void, nIDEvent: u64, uElapse: u32, lpTimerFunc: *void) -> u64
extern def GetCursorPos(lpPoint: *void) -> i32
extern def ExitProcess(uExitCode: u32) -> void
extern def SetLayeredWindowAttributes(hwnd: *void, crKey: u32, bAlpha: u8, dwFlags: u32) -> i32
extern def SetWindowCompositionAttribute(hwnd: *void, data: *void) -> i32
extern def BeginPaint(hWnd: *void, lpPaint: *void) -> *void
extern def EndPaint(hWnd: *void, lpPaint: *void) -> i32
extern def InvalidateRect(hWnd: *void, lpRect: *void, bErase: i32) -> i32
extern def FillRect(hdc: *void, lprc: *void, hbr: *void) -> i32
extern def RoundRect(hdc: *void, left: i32, top: i32, right: i32, bottom: i32, width: i32, height: i32) -> i32
extern def CreateSolidBrush(color: u32) -> *void
extern def CreatePen(iStyle: i32, cWidth: i32, color: u32) -> *void
extern def DeleteObject(ho: *void) -> i32
extern def SelectObject(hdc: *void, h: *void) -> *void
extern def DrawIcon(hDC: *void, x: i32, y: i32, hIcon: *void) -> i32
extern def DrawTextA(hdc: *void, lpchText: *u8, cchText: i32, lprc: *void, format: u32) -> i32
extern def SetBkMode(hdc: *void, mode: i32) -> i32
extern def SetTextColor(hdc: *void, color: u32) -> i32
extern def CreateFontA(cHeight: i32, cWidth: i32, cEscapement: i32, cOrientation: i32, cWeight: i32, bItalic: u32, bUnderline: u32, bStrikeOut: u32, iCharSet: u32, iOutPrecision: u32, iClipPrecision: u32, iQuality: u32, iPitchAndFamily: u32, pszFaceName: *u8) -> *void
extern def ShellExecuteA(hwnd: *void, lpOperation: *u8, lpFile: *u8, lpParameters: *u8, lpDirectory: *u8, nShowCmd: i32) -> *void
extern def DragAcceptFiles(hWnd: *void, fAccept: i32) -> void
extern def DragQueryFileA(hDrop: *void, iFile: u32, lpszFile: *u8, cch: u32) -> u32
extern def DragFinish(hDrop: *void) -> void
extern def SHGetFileInfoA(pszPath: *u8, dwFileAttributes: u32, psfi: *void, cbFileInfo: u32, uFlags: u32) -> u64
extern def RegCreateKeyA(hKey: u64, lpSubKey: *u8, phkResult: *u64) -> i32
extern def RegOpenKeyA(hKey: u64, lpSubKey: *u8, phkResult: *u64) -> i32
extern def RegSetValueExA(hKey: u64, lpValueName: *u8, Reserved: u32, dwType: u32, lpData: *u8, cbData: u32) -> i32
extern def RegQueryValueExA(hKey: u64, lpValueName: *u8, lpReserved: *u32, lpType: *u32, lpData: *u8, lpcbData: *u32) -> i32
extern def RegCloseKey(hKey: u64) -> i32

def GetFileName(path: *u8) -> *u8:
    len: i32 = lstrlenA(path)
    base: u64 = path as u64
    idx: i32 = len - 1
    
    while (idx) >= 0:
        ptr_addr: u64 = base + (idx as u64)
        ptr: *u8 = ptr_addr as *u8
        val: u8 = *ptr
        
        if (val) == 92: 
            res_addr: u64 = base + ((idx + 1) as u64)
            return res_addr as *u8
            
        if (val) == 47: 
            res_addr2: u64 = base + ((idx + 1) as u64)
            return res_addr2 as *u8
            
        idx = idx - 1
        
    return path

def SaveRegistry() -> void:
    hKey: u64 = 0
    hHeap: *void = GetProcessHeap()
    res: i32 = RegCreateKeyA(2147483649, "Software\\GlobalDropZone".data as *u8, (&hKey) as *u64)
    if (res) == 0:
        count_val: i32 = g_tile_count
        RegSetValueExA(hKey, "Count".data as *u8, 0, 4, (&count_val) as *u8, 4)
        i: i32 = 0
        while (i) < g_tile_count:
            key_buf: *u8 = HeapAlloc(hHeap, 8, 32)
            wsprintfA(key_buf, "File%d".data as *u8, i)
            path_addr: u64 = (g_paths as u64) + ((i as u64) * 260)
            len_path: i32 = lstrlenA(path_addr as *u8)
            len_path = len_path + 1
            RegSetValueExA(hKey, key_buf, 0, 1, path_addr as *u8, len_path as u32)
            HeapFree(hHeap, 0, key_buf as *void)
            i = i + 1
        RegCloseKey(hKey)

def LoadRegistry() -> void:
    hKey: u64 = 0
    hHeap: *void = GetProcessHeap()
    res: i32 = RegOpenKeyA(2147483649, "Software\\GlobalDropZone".data as *u8, (&hKey) as *u64)
    if (res) == 0:
        loaded_count: i32 = 0
        cbData: u32 = 4
        res2: i32 = RegQueryValueExA(hKey, "Count".data as *u8, 0 as *u32, 0 as *u32, (&loaded_count) as *u8, (&cbData) as *u32)
        if (res2) == 0:
            i: i32 = 0
            while (i) < loaded_count:
                key_buf: *u8 = HeapAlloc(hHeap, 8, 32)
                wsprintfA(key_buf, "File%d".data as *u8, i)
                path_buf: *u8 = HeapAlloc(hHeap, 8, 260)
                cbPath: u32 = 260
                res3: i32 = RegQueryValueExA(hKey, key_buf, 0 as *u32, 0 as *u32, path_buf, (&cbPath) as *u32)
                if (res3) == 0:
                    psfi: *void = HeapAlloc(hHeap, 8, 400) as *void
                    SHGetFileInfoA(path_buf, 0, psfi, 400, 256)
                    ptr_to_hicon: *u64 = psfi as *u64
                    hIcon_val: u64 = *ptr_to_hicon
                    if hIcon_val != 0:
                        g_icons[g_tile_count] = hIcon_val
                        target_addr: u64 = (g_paths as u64) + ((g_tile_count as u64) * 260)
                        lstrcpyA(target_addr as *u8, path_buf)
                        g_tile_count = g_tile_count + 1
                    HeapFree(hHeap, 0, psfi as *void)
                HeapFree(hHeap, 0, path_buf as *void)
                HeapFree(hHeap, 0, key_buf as *void)
                i = i + 1
        RegCloseKey(hKey)

def WindowProc(hWnd: *void, msg: u32, wParam: u64, lParam: i64) -> i64:
    hHeap: *void = GetProcessHeap()

    if msg == WM_TIMER:
        GetCursorPos((&g_pt) as *void)
        
        if g_pt.x >= (g_screen_w - 4):
            g_hover_time = g_hover_time + 15
            if g_hover_time >= 150:
                g_is_triggered = 1
        else:
            g_hover_time = 0
            if g_is_triggered == 1:
                if g_pt.x < (g_screen_w - 300):
                    g_is_triggered = 0

        target_x: i32 = g_screen_w - 4 
        if g_is_triggered == 1:
            target_x = g_screen_w - 300

        diff: i32 = target_x - g_shelf_current_x
        if (diff) != 0:
            step: i32 = diff / 5
            if (step) == 0:
                if (diff) > 0:
                    step = 1
                else:
                    step = -1
            
            g_shelf_current_x = g_shelf_current_x + step
            SetWindowPos(hWnd, 0 as *void, g_shelf_current_x, 0, 300, g_screen_h, SWP_NOZORDER)
        return 0 as i64

    if msg == WM_LBUTTONDOWN:
        lp32: i32 = lParam as i32
        x: i32 = lp32 % 65536
        y: i32 = lp32 / 65536
        
        if (x) >= 20:
            if (x) <= 280:
                if (y) >= 60:
                    col: i32 = (x - 20) / 135
                    row: i32 = (y - 60) / 90
                    index: i32 = (row * 2) + col
                    
                    if (index) < g_tile_count:
                        path_addr: u64 = (g_paths as u64) + ((index as u64) * 260)
                        ShellExecuteA(0 as *void, "open".data as *u8, path_addr as *u8, 0 as *u8, 0 as *u8, 5)
        return 0 as i64

    if msg == WM_RBUTTONDOWN:
        lp32r: i32 = lParam as i32
        rx: i32 = lp32r % 65536
        ry: i32 = lp32r / 65536
        
        if (rx) >= 20:
            if (rx) <= 280:
                if (ry) >= 60:
                    r_col: i32 = (rx - 20) / 135
                    r_row: i32 = (ry - 60) / 90
                    r_idx: i32 = (r_row * 2) + r_col
                    
                    if (r_idx) < g_tile_count:
                        j: i32 = r_idx
                        limit: i32 = g_tile_count - 1
                        while (j) < limit:
                            g_icons[j] = g_icons[j + 1]
                            dest_addr: u64 = (g_paths as u64) + ((j as u64) * 260)
                            src_addr: u64 = (g_paths as u64) + (((j + 1) as u64) * 260)
                            lstrcpyA(dest_addr as *u8, src_addr as *u8)
                            j = j + 1
                        
                        g_tile_count = g_tile_count - 1
                        SaveRegistry()
                        InvalidateRect(hWnd, 0 as *void, 1)
        return 0 as i64

    if msg == WM_DROPFILES:
        hDrop: *void = wParam as *void
        file_count: u32 = DragQueryFileA(hDrop, 4294967295, 0 as *u8, 0)
        
        i: u32 = 0
        while (i) < file_count:
            path_buf: *u8 = HeapAlloc(hHeap, 8, 260)
            DragQueryFileA(hDrop, i, path_buf, 260)
            
            psfi: *void = HeapAlloc(hHeap, 8, 400) as *void
            SHGetFileInfoA(path_buf, 0, psfi, 400, 256)
            
            ptr_to_hicon_addr: *u64 = psfi as *u64
            hIcon_val: u64 = *ptr_to_hicon_addr
            
            if hIcon_val != 0:
                g_icons[g_tile_count] = hIcon_val
                target_addr: u64 = (g_paths as u64) + ((g_tile_count as u64) * 260)
                lstrcpyA(target_addr as *u8, path_buf)
                g_tile_count = g_tile_count + 1
            
            HeapFree(hHeap, 0, psfi as *void)
            HeapFree(hHeap, 0, path_buf as *void)
            i = i + 1
        
        DragFinish(hDrop)
        SaveRegistry()
        InvalidateRect(hWnd, 0 as *void, 1)
        return 0 as i64

    if msg == WM_PAINT:
        ps_buf: *void = HeapAlloc(hHeap, 8, 128) as *void
        hdc: *void = BeginPaint(hWnd, ps_buf)
        SetBkMode(hdc, 1)
        
        hFontTitle: *void = CreateFontA(-18, 0, 0, 0, 600, 0, 0, 0, 1, 0, 0, 5, 0, "Segoe UI".data as *u8)
        hFontText: *void = CreateFontA(-13, 0, 0, 0, 400, 0, 0, 0, 1, 0, 0, 5, 0, "Segoe UI".data as *u8)
        
        SetTextColor(hdc, 16777215) 
        oldFont: *void = SelectObject(hdc, hFontTitle)
        
        g_rc.left = 20
        g_rc.top = 20
        g_rc.right = 300
        g_rc.bottom = 50
        DrawTextA(hdc, "Drop Zone".data as *u8, -1, (&g_rc) as *void, 36) 
        
        if g_tile_count == 0:
            SelectObject(hdc, hFontText)
            SetTextColor(hdc, 10066329) 
            g_rc.left = 20
            g_rc.top = g_screen_h / 2
            g_rc.right = 280
            g_rc.bottom = (g_screen_h / 2) + 30
            DrawTextA(hdc, "Drag and drop files here".data as *u8, -1, (&g_rc) as *void, 1) 
        
        SelectObject(hdc, hFontText)
        
        tile_bg: *void = CreateSolidBrush(2829099)     # RGB(43,43,43) - #2B2B2B
        tile_border: *void = CreatePen(0, 1, 4210752)  # PS_SOLID, 1px, RGB(64,64,64) - #404040
        oldPen: *void = SelectObject(hdc, tile_border)
        oldBrush: *void = SelectObject(hdc, tile_bg)
        
        k: i32 = 0
        while (k) < g_tile_count:
            col2: i32 = k % 2
            row2: i32 = k / 2
            
            base_x: i32 = 20 + (col2 * 135)
            base_y: i32 = 60 + (row2 * 90)
            
            RoundRect(hdc, base_x, base_y, base_x + 125, base_y + 80, 8, 8)
            
            hIcon2: *void = g_icons[k] as *void
            DrawIcon(hdc, base_x + 46, base_y + 12, hIcon2)
            
            path_addr2: u64 = (g_paths as u64) + ((k as u64) * 260)
            file_name: *u8 = GetFileName(path_addr2 as *u8)
                
            SetTextColor(hdc, 16777215)
            g_rc.left = base_x + 5
            g_rc.top = base_y + 48
            g_rc.right = base_x + 120
            g_rc.bottom = base_y + 75
            # DT_CENTER | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS = 1 + 32 + 4 + 32768 = 32805
            DrawTextA(hdc, file_name, -1, (&g_rc) as *void, 32805)
            
            k = k + 1
            
        SelectObject(hdc, oldPen)
        SelectObject(hdc, oldBrush)
        DeleteObject(tile_bg)
        DeleteObject(tile_border)
        
        SelectObject(hdc, oldFont)
        DeleteObject(hFontTitle)
        DeleteObject(hFontText)
        
        EndPaint(hWnd, ps_buf)
        HeapFree(hHeap, 0, ps_buf as *void)
        return 0 as i64

    if msg == WM_DESTROY:
        PostQuitMessage(0)
        return 0 as i64

    return DefWindowProcA(hWnd, msg, wParam, lParam)

def WinMain(hInstance: *void, hPrevInstance: *void, lpCmdLine: *u8, nCmdShow: i32) -> i32:
    main()
    return 0

def main() -> void:
    hHeap: *void = GetProcessHeap()
    g_icons = HeapAlloc(hHeap, 8, 8000) as *u64
    g_paths = HeapAlloc(hHeap, 8, 260000) as *u8

    LoadRegistry()

    hInstance: *void = GetModuleHandleA(0 as *u8)
    
    g_screen_w = GetSystemMetrics(SM_CXSCREEN)
    g_screen_h = GetSystemMetrics(SM_CYSCREEN)
    g_shelf_current_x = g_screen_w - 4

    g_wc.cbSize = sizeof(WNDCLASSEXA)
    g_wc.style = 0
    g_wc.lpfnWndProc = WindowProc as *void 
    g_wc.cbClsExtra = 0
    g_wc.cbWndExtra = 0
    g_wc.hInstance = hInstance
    g_wc.hIcon = 0 as *void
    g_wc.hCursor = 0 as *void
    g_wc.hbrBackground = CreateSolidBrush(1315860) 
    g_wc.lpszMenuName = 0 as *u8
    g_wc.lpszClassName = "GDZClass".data as *u8
    g_wc.hIconSm = 0 as *void

    RegisterClassExA((&g_wc) as *void)

    hWnd: *void = CreateWindowExA(
        WS_EX_STYLE, 
        "GDZClass".data as *u8, 
        "GlobalDropZone".data as *u8, 
        WS_MAIN_STYLE,
        g_shelf_current_x, 
        0, 
        300, 
        g_screen_h, 
        0 as *void, 
        0 as *void, 
        hInstance, 
        0 as *void
    )

    SetLayeredWindowAttributes(hWnd, 0, 255, 2)

    policy: ACCENT_POLICY = {0}
    policy.AccentState = 3     
    policy.AccentFlags = 2     
    
    # Цвет ABGR: 0xA0151515 Около 62% прозрачности, темно-серый
    policy.GradientColor = 2685736213 
    policy.AnimationId = 0
    
    data: WINDOWCOMPOSITIONATTRIBDATA = {0}
    data.Attrib = 19
    data.pvData = (&policy) as *void
    data.cbData = sizeof(ACCENT_POLICY)
    
    SetWindowCompositionAttribute(hWnd, (&data) as *void)

    DragAcceptFiles(hWnd, 1)
    ShowWindow(hWnd, SW_SHOW)
    
    SetTimer(hWnd, 1, 15, 0 as *void)

    while GetMessageA((&g_msg) as *void, 0 as *void, 0, 0) > 0:
        TranslateMessage((&g_msg) as *void)
        DispatchMessageA((&g_msg) as *void)

    ExitProcess(0)