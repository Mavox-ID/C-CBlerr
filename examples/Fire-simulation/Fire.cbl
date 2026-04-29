# надо бы мне написать наконец таки stdlib для CBlerr, чтобы не плодить по каждому файлу эти объявления и инъекции строк, но пока так потому что мне лень, а так же потому что в этом файле есть куча всяких математических функций, которые я не уверен, что буду использовать в других местах

const RENDER_W: i32 = 240
const RENDER_H: i32 = 160

extern def GetModuleHandleA(lpModuleName: i64) -> i64
extern def LoadLibraryA(lpLibFileName: i64) -> i64
extern def GetProcAddress(hModule: i64, lpProcName: i64) -> i64
extern def RegisterClassA(lpWndClass: i64) -> i16
extern def CreateWindowExA(dwExStyle: i32, lpClassName: i64, lpWindowName: i64, dwStyle: i32, x: i32, y: i32, nWidth: i32, nHeight: i32, hWndParent: i64, hMenu: i64, hInstance: i64, lpParam: i64) -> i64
extern def ShowWindow(hWnd: i64, nCmdShow: i32) -> i32
extern def GetConsoleWindow() -> i64
extern def PeekMessageA(lpMsg: i64, hWnd: i64, wMsgFilterMin: i32, wMsgFilterMax: i32, wRemoveMsg: i32) -> i32
extern def TranslateMessage(lpMsg: i64) -> i32
extern def DispatchMessageA(lpMsg: i64) -> i64
extern def IsWindow(hWnd: i64) -> i32
extern def GetDC(hWnd: i64) -> i64
extern def ReleaseDC(hWnd: i64, hDC: i64) -> i32
extern def StretchDIBits(hdc: i64, XDest: i32, YDest: i32, nDestWidth: i32, nDestHeight: i32, XSrc: i32, YSrc: i32, nSrcWidth: i32, nSrcHeight: i32, lpBits: i64, lpBitsInfo: i64, iUsage: i32, rop: i32) -> i32
extern def LocalAlloc(uFlags: i32, uBytes: i64) -> i64
extern def GetAsyncKeyState(vKey: i32) -> i16
extern def GetForegroundWindow() -> i64
extern def GetCursorPos(lpPoint: i64) -> i32
extern def SetCursorPos(x: i32, y: i32) -> i32
extern def ShowCursor(bShow: i32) -> i32
extern def LoadCursorA(hInstance: i64, lpCursorName: i64) -> i64
extern def GetTickCount() -> i32

def mul_fp(a: i32, b: i32) -> i32:
    return (a * b) / 1024

def div_fp(a: i32, b: i32) -> i32:
    return (a * 1024) / b

def sqrt_int(val: i32) -> i32:
    if 0 >= val:
        return 0
    res: i32 = 0
    bit: i32 = 1073741824
    while bit > val:
        bit = bit / 4
    while bit != 0:
        if val >= res + bit:
            val = val - (res + bit)
            res = (res / 2) + bit
        else:
            res = res / 2
        bit = bit / 4
    return res

def sqrt_fp(val: i32) -> i32:
    return sqrt_int(val * 1024)

def sin_fp(x: i32) -> i32:
    x = x % 2048
    if x > 1024:
        x = x - 2048
    if (0 - 1024) > x:
        x = x + 2048
    x_abs: i32 = x
    if 0 > x:
        x_abs = 0 - x
    res: i32 = (x * 4) - ((x * x_abs * 4) / 1024)
    res_abs: i32 = res
    if 0 > res:
        res_abs = 0 - res
    return mul_fp(230, mul_fp(res, res_abs) - res) + res

def cos_fp(x: i32) -> i32:
    return sin_fp(x + 512)

def smooth_noise(x: i32, y: i32, z: i32) -> i32:
    nx: i32 = sin_fp(x)
    ny: i32 = sin_fp(y)
    nz: i32 = sin_fp(z)
    return (nx + ny + nz) / 3

def fire_density(px: i32, py: i32, pz: i32, time: i32) -> i32:
    bend_x: i32 = mul_fp(sin_fp(py * 2 - time * 3), 250)
    px = px + bend_x
    
    dist_xz: i32 = sqrt_fp(mul_fp(px, px) + mul_fp(pz, pz))
    py_clamped: i32 = py
    if 0 > py_clamped:
        py_clamped = 0 - py_clamped
        
    radius: i32 = 1024 - mul_fp(py_clamped, 400)
    if 0 > py:
        radius = 1024 - mul_fp(py_clamped, 800)
        
    shape: i32 = radius - dist_xz
    if 0 > shape:
        return 0
        
    nx: i32 = px * 3
    ny: i32 = py * 3 - time * 5
    nz: i32 = pz * 3
    
    turb1: i32 = smooth_noise(nx, ny, nz)
    turb2: i32 = smooth_noise(nx * 2 + 512, ny * 2 - 256, nz * 2) / 2
    turb: i32 = turb1 + turb2
    
    if 0 > turb:
        turb = 0 - turb
        
    density: i32 = shape + 200 - mul_fp(turb, 1300)
    return density

def get_vector_color(density: i32) -> i32:
    if 100 > density:
        return 16777215 
    if 350 > density:
        return 16711680
    if 750 > density:
        return 16737792 
    return 16763904     

def main() -> i32:
    hinst: i64 = GetModuleHandleA(0)
    
    con_hwnd: i64 = GetConsoleWindow()
    ShowWindow(con_hwnd, 0)
    
    str_buf: i64 = LocalAlloc(64, 128)
    str_ptr: *i32 = str_buf as *i32
    
    str_ptr[0] = 1919251317 
    str_ptr[1] = 1680749107
    str_ptr[2] = 27756      
    u32_addr: i64 = str_buf
    
    str_ptr[4] = 1466328388 
    str_ptr[5] = 1868852841 
    str_ptr[6] = 1869762679 
    str_ptr[7] = 16739      
    dwp_addr: i64 = str_buf + 16
    
    str_ptr[8] = 1701996870 
    str_ptr[9] = 0          
    cls_addr: i64 = str_buf + 32
    
    str_ptr[12] = 1162625603 
    str_ptr[13] = 1444958802 
    str_ptr[14] = 1869898597 
    str_ptr[15] = 1766203506
    str_ptr[16] = 25970      
    title_addr: i64 = str_buf + 48
    
    u32_handle: i64 = LoadLibraryA(u32_addr)
    dwp_ptr: i64 = GetProcAddress(u32_handle, dwp_addr)
    
    wc_addr: i64 = LocalAlloc(64, 72)
    wc_i32: *i32 = wc_addr as *i32
    wc_i64: *i64 = wc_addr as *i64
    
    wc_i32[0] = 3                 
    wc_i64[1] = dwp_ptr            
    wc_i64[3] = hinst              
    wc_i64[5] = LoadCursorA(0, 32512) 
    wc_i64[8] = cls_addr           
    
    RegisterClassA(wc_addr)
    
    hwnd: i64 = CreateWindowExA(0, cls_addr, title_addr, 282001408, 100, 100, 800, 600, 0, 0, hinst, 0)
    ShowWindow(hwnd, 5)
    
    hdc: i64 = GetDC(hwnd)
    pixels_size: i64 = ((RENDER_W * RENDER_H * 4) as i64)
    pixels_addr: i64 = LocalAlloc(64, pixels_size)
    pixel_array: *i32 = pixels_addr as *i32
    
    bmi_addr: i64 = LocalAlloc(64, 40)
    bmi_ptr: *i32 = bmi_addr as *i32
    bmi_ptr[0] = 40
    bmi_ptr[1] = RENDER_W
    bmi_ptr[2] = 0 - RENDER_H
    bmi_ptr[3] = 2097153
    bmi_ptr[4] = 0
    
    msg_addr: i64 = LocalAlloc(64, 48)
    mouse_addr: i64 = LocalAlloc(64, 8)
    mouse_ptr: *i32 = mouse_addr as *i32
    
    cam_x: i32 = 0
    cam_y: i32 = 0
    cam_z: i32 = 0 - 3500
    cam_yaw: i32 = 0
    cam_pitch: i32 = 0
    
    is_mouse_hidden: i32 = 0
    
    running: i32 = 1
    while running == 1:
        if IsWindow(hwnd) == 0:
            running = 0
            break
            
        while PeekMessageA(msg_addr, 0, 0, 0, 1) != 0:
            TranslateMessage(msg_addr)
            DispatchMessageA(msg_addr)
            
        time_ms: i32 = GetTickCount()
        time_ms = time_ms % 1000000
        if 0 > time_ms:
            time_ms = 0 - time_ms
        t: i32 = time_ms / 3
        
        fg_hwnd: i64 = GetForegroundWindow()
        if fg_hwnd == hwnd:
            key_lmb: i32 = GetAsyncKeyState(1) as i32
            
            if key_lmb != 0:
                if 0 == is_mouse_hidden:
                    ShowCursor(0)
                    is_mouse_hidden = 1
                    
                GetCursorPos(mouse_addr)
                mx: i32 = mouse_ptr[0]
                my: i32 = mouse_ptr[1]
                
                dx: i32 = (mx - 400) / 4
                dy: i32 = (my - 300) / 4
                cam_yaw = cam_yaw + dx
                cam_pitch = cam_pitch + dy
                
                if cam_pitch > 512:
                    cam_pitch = 512
                if (0 - 512) > cam_pitch:
                    cam_pitch = 0 - 512
                    
                SetCursorPos(400, 300)
                
            if 0 == key_lmb:
                if is_mouse_hidden != 0:
                    ShowCursor(1)
                    is_mouse_hidden = 0
                
            speed: i32 = 40
            fw_x: i32 = sin_fp(cam_yaw)
            fw_z: i32 = cos_fp(cam_yaw)
            
            key_w: i32 = GetAsyncKeyState(87) as i32
            key_s: i32 = GetAsyncKeyState(83) as i32
            key_a: i32 = GetAsyncKeyState(65) as i32
            key_d: i32 = GetAsyncKeyState(68) as i32
            
            if key_w != 0:
                cam_x = cam_x + mul_fp(fw_x, speed)
                cam_z = cam_z + mul_fp(fw_z, speed)
            if key_s != 0:
                cam_x = cam_x - mul_fp(fw_x, speed)
                cam_z = cam_z - mul_fp(fw_z, speed)
            if key_a != 0:
                cam_x = cam_x - mul_fp(fw_z, speed)
                cam_z = cam_z + mul_fp(fw_x, speed)
            if key_d != 0:
                cam_x = cam_x + mul_fp(fw_z, speed)
                cam_z = cam_z - mul_fp(fw_x, speed)
                
        key_esc: i32 = GetAsyncKeyState(27) as i32
        if key_esc != 0:
            running = 0
            
        dir_x: i32 = mul_fp(cos_fp(cam_pitch), sin_fp(cam_yaw))
        dir_y: i32 = sin_fp(0 - cam_pitch)
        dir_z: i32 = mul_fp(cos_fp(cam_pitch), cos_fp(cam_yaw))
        
        right_x: i32 = cos_fp(cam_yaw)
        right_z: i32 = sin_fp(0 - cam_yaw)
        
        up_x: i32 = mul_fp(dir_y, right_z)
        up_y: i32 = mul_fp(dir_z, right_x) - mul_fp(dir_x, right_z)
        up_z: i32 = mul_fp(0 - dir_y, right_x)
        
        y: i32 = 0
        while RENDER_H > y:
            x: i32 = 0
            while RENDER_W > x:
                uv_x: i32 = ((x * 2048) / RENDER_W) - 1024
                uv_y: i32 = ((y * 2048) / RENDER_H) - 1024
                
                rx: i32 = dir_x + mul_fp(right_x, uv_x) - mul_fp(up_x, uv_y)
                ry: i32 = dir_y - mul_fp(up_y, uv_y)
                rz: i32 = dir_z + mul_fp(right_z, uv_x) - mul_fp(up_z, uv_y)
                
                r_len: i32 = sqrt_fp(mul_fp(rx, rx) + mul_fp(ry, ry) + mul_fp(rz, rz))
                if r_len > 0:
                    rx = div_fp(rx, r_len)
                    ry = div_fp(ry, r_len)
                    rz = div_fp(rz, r_len)
                    
                dist: i32 = 0
                max_density: i32 = 0
                step: i32 = 0
                
                while 25 > step:
                    px: i32 = cam_x + mul_fp(rx, dist)
                    py: i32 = cam_y + mul_fp(ry, dist)
                    pz: i32 = cam_z + mul_fp(rz, dist)
                    
                    d: i32 = fire_density(px, py, pz, t)
                    if d > max_density:
                        max_density = d
                        
                    if max_density > 800:
                        break
                        
                    dist = dist + 256
                    step = step + 1
                    
                final_color: i32 = get_vector_color(max_density)
                offset: i32 = (y * RENDER_W) + x
                pixel_array[offset] = final_color
                
                x = x + 1
            y = y + 1
            
        StretchDIBits(hdc, 0, 0, 800, 600, 0, 0, RENDER_W, RENDER_H, pixels_addr, bmi_addr, 0, 13369376)
        
    ShowWindow(con_hwnd, 5) 
    ReleaseDC(hwnd, hdc)
    return 0