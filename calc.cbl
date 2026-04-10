extern def sin(x: f64) -> f64
extern def cos(x: f64) -> f64
extern def tan(x: f64) -> f64
extern def malloc(size: int) -> *int
extern def free(ptr: *int) -> void

extern def GetConsoleWindow() -> *void
extern def ShowWindow(hWnd: *void, nCmdShow: int) -> int
extern def CreateWindowExA(dwExStyle: int, lpClassName: *void, lpWindowName: *void, dwStyle: int, x: int, y: int, nWidth: int, nHeight: int, hWndParent: *void, hMenu: *void, hInstance: *void, lpParam: *void) -> *void
extern def GetDC(hwnd: *void) -> *void
extern def ReleaseDC(hwnd: *void, hdc: *void) -> int
extern def PeekMessageA(lpMsg: *void, hWnd: *void, wMsgFilterMin: int, wMsgFilterMax: int, wRemoveMsg: int) -> int
extern def DispatchMessageA(lpMsg: *void) -> int
extern def StretchDIBits(hdc: *void, xDest: int, yDest: int, DestWidth: int, DestHeight: int, xSrc: int, ySrc: int, SrcWidth: int, SrcHeight: int, lpBits: *int, lpbmi: *int, iUsage: int, rop: int) -> int
extern def GetAsyncKeyState(vKey: int) -> i16
extern def Sleep(dwMilliseconds: int) -> void

const RENDER_W: int = 320
const RENDER_H: int = 240
const WINDOW_W: int = 960
const WINDOW_H: int = 720
const MAP_WIDTH: int = 16
const MAP_HEIGHT: int = 16

world_map: *int = [
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,1,1,1,0,1,0,1,1,1,1,1,1,0,1,
    1,0,1,0,0,0,0,0,2,0,0,0,0,1,0,1,
    1,0,1,0,1,1,1,1,1,1,1,1,0,1,0,1,
    1,0,0,0,1,0,0,0,0,0,0,1,0,1,0,1,
    1,1,1,0,1,0,1,1,1,1,0,1,0,0,0,1,
    1,0,0,0,0,0,1,0,0,1,0,1,1,1,0,1,
    1,0,1,1,1,1,1,0,1,1,0,0,0,1,0,1,
    1,0,0,0,0,0,0,0,1,0,0,1,0,1,0,1,
    1,1,1,1,1,1,1,0,1,0,1,1,0,1,0,1,
    1,0,0,0,0,0,1,0,0,0,1,0,0,1,0,1,
    1,0,1,1,1,0,1,1,1,1,1,0,1,1,0,1,
    1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,1,1,1,1,1,1,1,1,1,1,0,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
]

def get_map(x: int, y: int) -> int:
    if 0 > x or x >= MAP_WIDTH or 0 > y or y >= MAP_HEIGHT:
        return 1
    return world_map[y * MAP_WIDTH + x]

def main() -> int:
    console_hwnd: *void = GetConsoleWindow()
    ShowWindow(console_hwnd, 0)
    
    cls_name: *void = "STATIC".data as *void
    win_name: *void = "Liminal Space - Native CBlerr Engine".data as *void

    hwnd: *void = CreateWindowExA(0, cls_name, win_name, 282001408, 100, 100, WINDOW_W, WINDOW_H, 0 as *void, 0 as *void, 0 as *void, 0 as *void)
    hdc: *void = GetDC(hwnd)
    pixels: *int = malloc(307200)

    bmi: *int = [40, RENDER_W, 0 - RENDER_H, 2097153, 0, 0, 0, 0, 0, 0]
    msg: *int = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]

    player_x: f64 = 1.5
    player_y: f64 = 1.5
    
    player_a: f64 = 0.0
    fov: f64 = 0.8
    
    bob_timer: f64 = 0.0
    is_running: int = 1

    while is_running == 1:
        
        while PeekMessageA(msg as *void, 0 as *void, 0, 0, 1) != 0:
            DispatchMessageA(msg as *void)
            
        bob_offset: int = (sin(bob_timer) * 6.0) as int
        horizon: int = 120 + bob_offset
        
        x: int = 0
        while RENDER_W > x:
            
            ray_angle: f64 = (player_a - fov / 2.0) + ((x as f64) / (RENDER_W as f64)) * fov
            eye_x: f64 = sin(ray_angle)
            eye_y: f64 = cos(ray_angle)
            
            distance: f64 = 0.0
            hit_wall: int = 0
            is_door: int = 0
            
            while hit_wall == 0 and 20.0 > distance:
                distance = distance + 0.05
                test_x: int = (player_x + eye_x * distance) as int
                test_y: int = (player_y + eye_y * distance) as int
                
                if 0 > test_x or test_x >= MAP_WIDTH or 0 > test_y or test_y >= MAP_HEIGHT:
                    hit_wall = 1
                    distance = 20.0
                else:
                    cell: int = get_map(test_x, test_y)
                    if cell == 1:
                        hit_wall = 1
                    if cell == 2:
                        hit_wall = 1
                        is_door = 1

            exact_x: f64 = player_x + eye_x * distance
            exact_y: f64 = player_y + eye_y * distance
            hit_x_int: int = exact_x as int
            hit_y_int: int = exact_y as int
            fract_x: f64 = exact_x - (hit_x_int as f64)
            fract_y: f64 = exact_y - (hit_y_int as f64)
            
            is_edge: int = 0
            if 0.05 > fract_x:
                is_edge = 1
            if fract_x > 0.95:
                is_edge = 1
            if 0.05 > fract_y:
                is_edge = 1
            if fract_y > 0.95:
                is_edge = 1

            is_handle_x: int = 0
            if is_door == 1:
                if fract_x > 0.75 and 0.85 > fract_x:
                    is_handle_x = 1
                if fract_y > 0.75 and 0.85 > fract_y:
                    is_handle_x = 1

            corrected_dist: f64 = distance * cos(ray_angle - player_a)
            if 0.1 > corrected_dist:
                corrected_dist = 0.1

            wall_height: f64 = (RENDER_H as f64) / corrected_dist
            ceiling_int: int = (horizon as f64 - wall_height) as int
            floor_int: int = (horizon as f64 + wall_height) as int
            
            dx_val: f64 = ((x as f64) - 160.0) / 160.0
            lens_curve: int = (dx_val * dx_val * 25.0) as int
            
            ceiling_int = ceiling_int - lens_curve
            floor_int = floor_int + lens_curve
            
            if distance >= 19.9:
                ceiling_int = horizon
                floor_int = horizon
            
            color: int = 13943940 
            
            if is_door == 1:
                color = 6042391
                if is_edge == 1:
                    color = 4005660 
            else:
                if is_edge == 1:
                    color = 10652759

                if distance > 12.0:
                    color = 2761754
                else:
                    if distance > 8.0:
                        if is_edge == 1:
                            color = 5915185
                        else:
                            color = 7692600
                    else:
                        if distance > 5.0:
                            if is_edge == 1:
                                color = 7692600
                            else:
                                color = 10652759
                        else:
                            if distance > 3.0:
                                if is_edge == 1:
                                    color = 10652759
                                else:
                                    color = 12363891

            wall_h: int = floor_int - ceiling_int
            half_wall: int = wall_h / 2

            y: int = 0
            while RENDER_H > y:
                idx: int = y * RENDER_W + x
                
                if ceiling_int > y:
                    pixels[idx] = 14737105
                else:
                    if y >= ceiling_int and floor_int >= y:
                        draw_color: int = color
                        
                        if is_door == 1 and is_handle_x == 1:
                            if y > (ceiling_int + half_wall - 4) and (ceiling_int + half_wall + 4) > y:
                                draw_color = 14935040
                                
                        pixels[idx] = draw_color
                    else:
                        pixels[idx] = 5915185
                
                dy_val: f64 = ((y as f64) - 120.0) / 120.0
                dist_sq: f64 = dx_val * dx_val + dy_val * dy_val
                if dist_sq > 1.3:
                    pixels[idx] = 1579032 
                
                y = y + 1

            x = x + 1

        StretchDIBits(hdc, 0, 0, WINDOW_W, WINDOW_H, 0, 0, RENDER_W, RENDER_H, pixels, bmi, 0, 13369376)

        next_x: f64 = player_x
        next_y: f64 = player_y
        is_moving: int = 0

        if GetAsyncKeyState(87) != 0:
            next_x = player_x + sin(player_a) * 0.08
            next_y = player_y + cos(player_a) * 0.08
            is_moving = 1
        if GetAsyncKeyState(83) != 0:
            next_x = player_x - sin(player_a) * 0.08
            next_y = player_y - cos(player_a) * 0.08
            is_moving = 1
            
        if get_map(next_x as int, player_y as int) == 0:
            player_x = next_x
        if get_map(player_x as int, next_y as int) == 0:
            player_y = next_y

        if is_moving == 1:
            bob_timer = bob_timer + 0.15

        if GetAsyncKeyState(65) != 0:
            player_a = player_a - 0.06
        if GetAsyncKeyState(68) != 0:
            player_a = player_a + 0.06
            
        if GetAsyncKeyState(69) != 0:
            look_x: int = (player_x + sin(player_a) * 1.5) as int
            look_y: int = (player_y + cos(player_a) * 1.5) as int
            
            if look_x >= 0 and MAP_WIDTH > look_x and look_y >= 0 and MAP_HEIGHT > look_y:
                if get_map(look_x, look_y) == 2:
                    world_map[look_y * MAP_WIDTH + look_x] = 0 
                    
        if GetAsyncKeyState(27) != 0:
            is_running = 0

        Sleep(16)

    free(pixels)
    ReleaseDC(hwnd, hdc)
    return 0