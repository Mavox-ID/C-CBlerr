import "lib.cbl"

struct MSG:
    hwnd: *void
    message: int
    wParam: *void
    lParam: *void
    time: int
    pt_x: int
    pt_y: int

struct WNDCLASSA:
    style: int
    lpfnWndProc: *void
    cbClsExtra: int
    cbWndExtra: int
    hInstance: *void
    hIcon: *void
    hCursor: *void
    hbrBackground: *void
    lpszMenuName: *void
    lpszClassName: *void

const WM_DESTROY: int = 2
const WM_CLOSE: int = 16
const WM_QUIT: int = 18

def custom_wnd_proc(hwnd: *void, msg_code: int, w_param: *void, l_param: *void) -> int:
    if msg_code == WM_CLOSE:
        PostQuitMessage(0)
        return 0
    if msg_code == WM_DESTROY:
        PostQuitMessage(0)
        return 0
    return DefWindowProcA(hwnd, msg_code, w_param, l_param)

const WINDOW_W: int = 960
const WINDOW_H: int = 720
const MAP_WIDTH: int = 32
const MAP_HEIGHT: int = 32

world_map: *int = [
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,1,0,0,0,0,0,0,0,1,0,0,1,0,0,1,0,0,1,0,0,1,0,0,1,0,0,1,0,0,1,
    1,0,1,0,1,1,1,1,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,1,0,1,0,0,0,1,0,0,0,0,1,0,0,1,0,0,1,0,0,1,0,0,1,0,0,1,0,0,1,
    1,0,1,0,1,0,1,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,1,0,1,0,1,0,1,0,1,0,0,1,0,0,1,0,0,1,0,0,1,0,0,1,0,0,1,0,0,1,
    1,0,1,0,1,0,1,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,1,0,1,0,1,0,1,0,1,0,0,1,0,0,1,0,0,1,0,0,1,0,0,1,0,0,1,0,0,1,
    1,0,1,0,1,0,1,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,1,0,1,0,1,0,1,0,1,0,0,1,0,0,1,0,0,1,0,0,1,0,0,1,0,0,1,0,0,1,
    1,0,1,0,1,0,1,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,1,0,1,0,1,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,1,1,1,0,1,1,1,0,1,0,0,0,1,1,1,0,0,0,1,1,1,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,1,0,0,0,1,1,1,0,0,0,1,1,1,0,0,0,0,0,0,0,0,1,
    1,1,1,1,1,0,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,1,0,1,1,1,1,1,1,0,0,1,1,1,0,1,
    1,0,1,1,1,1,1,0,1,1,1,0,0,0,1,0,0,1,0,0,0,0,0,0,1,0,0,1,0,0,0,1,
    1,0,1,0,0,0,1,0,1,0,0,0,1,0,1,0,0,1,1,1,1,1,1,0,1,0,0,1,0,1,1,1,
    1,0,1,0,1,0,1,0,1,0,1,1,1,0,1,0,0,0,0,0,0,0,0,0,1,0,0,1,0,0,0,1,
    1,0,1,0,1,0,0,0,1,0,0,0,0,0,1,0,0,1,1,1,1,0,1,1,1,0,0,1,1,1,0,1,
    1,0,1,0,1,1,1,1,1,1,1,1,1,0,1,0,0,1,0,0,0,0,1,0,0,0,0,0,0,1,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,0,1,0,1,1,1,1,0,1,1,1,1,0,1,0,1,
    1,0,1,1,1,1,1,1,1,1,1,0,1,0,1,0,0,1,0,0,0,0,0,0,1,0,0,0,0,1,0,1,
    1,0,1,0,0,0,0,0,0,0,1,0,1,0,1,0,0,1,1,1,1,1,1,1,1,0,1,1,1,1,0,1,
    1,0,1,0,1,1,1,1,1,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,1,
    1,0,1,0,1,0,0,0,1,0,1,0,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,0,1,1,1,1,
    1,0,1,0,1,0,1,0,1,0,1,0,0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,0,1,0,0,1,
    1,0,0,0,0,0,1,0,0,0,1,0,1,1,1,1,1,1,0,1,0,1,0,1,1,1,1,1,1,0,0,1,
    1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
]

def get_map(x: int, y: int) -> int:
    if 0 > x:
        return 1
    if x >= MAP_WIDTH:
        return 1
    if 0 > y:
        return 1
    if y >= MAP_HEIGHT:
        return 1
    return world_map[y * MAP_WIDTH + x]

def noise(x: int, y: int) -> f64:
    n: int = x * 137 + y * 281
    n = n % 8192
    n = (n * n * 41 + n * 11)
    n = n % 1000
    if 0 > n:
        n = 0 - n
    return (n as f64) / 1000.0

def create_texture(type_id: int) -> int:
    tex_id: *int = malloc(4)
    glGenTextures(1, tex_id)
    glBindTexture(3553, tex_id[0]) 
    glTexParameteri(3553, 10241, 9728) 
    glTexParameteri(3553, 10240, 9728)
    glTexParameteri(3553, 10242, 10497) 
    glTexParameteri(3553, 10243, 10497)

    buf: *int = malloc(16384) 
    y: int = 0
    while 64 > y:
        x: int = 0
        while 64 > x:
            r: int = 0
            g: int = 0
            b: int = 0
            a: int = 255
            
            if type_id == 1:
                u: f64 = (x as f64) / 64.0
                v: f64 = (y as f64) / 64.0
                col_r: f64 = 210.0
                col_g: f64 = 185.0
                col_b: f64 = 85.0
                shade: f64 = 0.85
                is_board: int = 0
                if 0.05 > v:
                    is_board = 1
                if v > 0.95:
                    is_board = 1
                if is_board == 1:
                    col_r = 70.0
                    col_g = 50.0
                    col_b = 30.0
                else:
                    pat: int = (u * 40.0) as int
                    if (pat % 2) == 0:
                        shade = 0.75
                    shade = shade - sin(v * 200.0) * 0.05 - noise(x * 5, y * 5) * 0.15
                r = (col_r * shade) as int
                g = (col_g * shade) as int
                b = (col_b * shade) as int
            
            if type_id == 2:
                base_f: f64 = 0.5 + noise(x * 61, y * 61) * 0.4
                if noise(x * 2, y * 2) > 0.7:
                    base_f = base_f * 0.6
                r = (120.0 * base_f) as int
                g = (110.0 * base_f) as int
                b = (60.0 * base_f) as int
            
            if type_id == 3:
                u_c: f64 = (x as f64) / 64.0
                v_c: f64 = (y as f64) / 64.0
                ceil_shade: f64 = 0.85
                if 0.05 > u_c:
                    ceil_shade = 0.15
                if 0.05 > v_c:
                    ceil_shade = 0.15
                if ceil_shade > 0.2:
                    ceil_shade = ceil_shade - noise(x * 2, y * 2) * 0.3
                    if noise(x * 3, y * 3) > 0.8:
                        ceil_shade = ceil_shade * 0.5
                r = (190.0 * ceil_shade) as int
                g = (190.0 * ceil_shade) as int
                b = (175.0 * ceil_shade) as int
                
            if type_id == 4:
                w_mod: int = rand() % 2
                u_e: f64 = (x as f64) / 64.0
                v_e: f64 = (y as f64) / 64.0
                draw_px: int = 0
                if u_e > 0.4:
                    if 0.6 > u_e:
                        draw_px = 1
                seg: int = (v_e * 4.0) as int
                if seg == 0:
                    if w_mod == 1:
                        if 0.6 > u_e:
                            if u_e > 0.2:
                                draw_px = 1
                if seg == 1:
                    if w_mod == 0:
                        if u_e > 0.4:
                            if 0.9 > u_e:
                                draw_px = 1
                if seg == 2:
                    if w_mod == 1:
                        if u_e > 0.1:
                            if 0.8 > u_e:
                                draw_px = 1
                if seg == 3:
                    if w_mod == 0:
                        if u_e > 0.3:
                            if 0.7 > u_e:
                                draw_px = 1
                if draw_px == 1:
                    r = 0
                    g = 0
                    b = 0
                    if v_e > 0.15:
                        if 0.25 > v_e:
                            if u_e > 0.3:
                                if 0.4 > u_e:
                                    r = 255
                            if u_e > 0.6:
                                if 0.7 > u_e:
                                    r = 255
                else:
                    a = 0
            
            if type_id == 5:
                dx: f64 = ((x as f64) - 32.0) / 32.0
                dy: f64 = ((y as f64) - 32.0) / 32.0
                alpha: f64 = (dx * dx + dy * dy) * 1.3
                if alpha > 1.0:
                    alpha = 1.0
                a = (alpha * 255.0) as int
                r = 0
                g = 0
                b = 0

            if type_id == 6:
                if (y % 4) == 0:
                    r = 0
                    g = 0
                    b = 0
                    a = 140 
                else:
                    n_val: int = rand() % 255
                    r = n_val
                    g = n_val
                    b = n_val
                    a = 40 
                
            if r > 255:
                r = 255
            if g > 255:
                g = 255
            if b > 255:
                b = 255
            if 0 > r:
                r = 0
            if 0 > g:
                g = 0
            if 0 > b:
                b = 0
            
            buf[y * 64 + x] = r + g * 256 + b * 65536 + a * 16777216
            x = x + 1
        y = y + 1
        
    glTexImage2D(3553, 0, 6408, 64, 64, 0, 6408, 5121, buf)
    res: int = tex_id[0]
    free(buf)
    free(tex_id)
    return res

def draw_world(t_wall: int, t_floor: int, t_ceil: int, px: f64, py: f64, pa: f64, fov: f64, vis_map: *int) -> void:
    i: int = 0
    while 1024 > i:
        vis_map[i] = 0
        i = i + 1

    p_xi: int = px as int
    p_yi: int = py as int
    
    dy_i: int = -2
    while 3 > dy_i:
        dx_i: int = -2
        while 3 > dx_i:
            nx: int = p_xi + dx_i
            ny: int = p_yi + dy_i
            if nx >= 0:
                if MAP_WIDTH > nx:
                    if ny >= 0:
                        if MAP_HEIGHT > ny:
                            vis_map[ny * MAP_WIDTH + nx] = 1
            dx_i = dx_i + 1
        dy_i = dy_i + 1

    ray_count: int = 360
    ray: int = 0
    angle_step: f64 = 6.2831853 / (ray_count as f64)

    while ray_count > ray:
        ra: f64 = (ray as f64) * angle_step
        rx: f64 = px
        ry: f64 = py
        rdx: f64 = sin(ra) * 0.15
        rdy: f64 = cos(ra) * 0.15
        depth: int = 0
        hit: int = 0
        
        while 200 > depth:
            rx = rx + rdx
            ry = ry + rdy
            map_x: int = rx as int
            map_y: int = ry as int
            if map_x >= 0:
                if MAP_WIDTH > map_x:
                    if map_y >= 0:
                        if MAP_HEIGHT > map_y:
                            idx: int = map_y * MAP_WIDTH + map_x
                            vis_map[idx] = 1
                            if world_map[idx] > 0:
                                hit = 1
                                depth = 200 
                                mx_m: int = map_x - 1
                                mx_p: int = map_x + 1
                                my_m: int = map_y - 1
                                my_p: int = map_y + 1
                                if mx_m >= 0:
                                    vis_map[map_y * MAP_WIDTH + mx_m] = 1
                                if MAP_WIDTH > mx_p:
                                    vis_map[map_y * MAP_WIDTH + mx_p] = 1
                                if my_m >= 0:
                                    vis_map[my_m * MAP_WIDTH + map_x] = 1
                                if MAP_HEIGHT > my_p:
                                    vis_map[my_p * MAP_WIDTH + map_x] = 1
            if hit == 1:
                depth = 200
            depth = depth + 1
        ray = ray + 1

    glColor3d(1.0, 1.0, 1.0)
    glBindTexture(3553, t_floor)
    glBegin(7)
    glTexCoord2d(0.0, 0.0)
    glVertex3d(0.0, 0.0, 0.0)
    glTexCoord2d(32.0, 0.0)
    glVertex3d(32.0, 0.0, 0.0)
    glTexCoord2d(32.0, 32.0)
    glVertex3d(32.0, 0.0, 32.0)
    glTexCoord2d(0.0, 32.0)
    glVertex3d(0.0, 0.0, 32.0)
    glEnd()

    glBindTexture(3553, t_ceil)
    glBegin(7)
    glTexCoord2d(0.0, 0.0)
    glVertex3d(0.0, 1.0, 0.0)
    glTexCoord2d(32.0, 0.0)
    glVertex3d(32.0, 1.0, 0.0)
    glTexCoord2d(32.0, 32.0)
    glVertex3d(32.0, 1.0, 32.0)
    glTexCoord2d(0.0, 32.0)
    glVertex3d(0.0, 1.0, 32.0)
    glEnd()

    glBindTexture(3553, t_wall)
    glBegin(7)
    y: int = 0
    while MAP_HEIGHT > y:
        x: int = 0
        while MAP_WIDTH > x:
            idx_w: int = y * MAP_WIDTH + x
            if vis_map[idx_w] == 1:
                if world_map[idx_w] > 0:
                    xf: f64 = x as f64
                    yf: f64 = y as f64
                    if get_map(x, y - 1) == 0:
                        glTexCoord2d(0.0, 1.0)
                        glVertex3d(xf, 0.0, yf)
                        glTexCoord2d(1.0, 1.0)
                        glVertex3d(xf + 1.0, 0.0, yf)
                        glTexCoord2d(1.0, 0.0)
                        glVertex3d(xf + 1.0, 1.0, yf)
                        glTexCoord2d(0.0, 0.0)
                        glVertex3d(xf, 1.0, yf)
                    if get_map(x, y + 1) == 0:
                        glTexCoord2d(0.0, 1.0)
                        glVertex3d(xf + 1.0, 0.0, yf + 1.0)
                        glTexCoord2d(1.0, 1.0)
                        glVertex3d(xf, 0.0, yf + 1.0)
                        glTexCoord2d(1.0, 0.0)
                        glVertex3d(xf, 1.0, yf + 1.0)
                        glTexCoord2d(0.0, 0.0)
                        glVertex3d(xf + 1.0, 1.0, yf + 1.0)
                    if get_map(x - 1, y) == 0:
                        glTexCoord2d(0.0, 1.0)
                        glVertex3d(xf, 0.0, yf + 1.0)
                        glTexCoord2d(1.0, 1.0)
                        glVertex3d(xf, 0.0, yf)
                        glTexCoord2d(1.0, 0.0)
                        glVertex3d(xf, 1.0, yf)
                        glTexCoord2d(0.0, 0.0)
                        glVertex3d(xf, 1.0, yf + 1.0)
                    if get_map(x + 1, y) == 0:
                        glTexCoord2d(0.0, 1.0)
                        glVertex3d(xf + 1.0, 0.0, yf)
                        glTexCoord2d(1.0, 1.0)
                        glVertex3d(xf + 1.0, 0.0, yf + 1.0)
                        glTexCoord2d(1.0, 0.0)
                        glVertex3d(xf + 1.0, 1.0, yf + 1.0)
                        glTexCoord2d(0.0, 0.0)
                        glVertex3d(xf + 1.0, 1.0, yf)
            x = x + 1
        y = y + 1
    glEnd()

def main() -> int:
    console_hwnd: *void = GetConsoleWindow()
    ShowWindow(console_hwnd, 0)
    
    vis_map: *int = malloc(4096)
    
    wave_buf_idle: *int = malloc(8240) 
    wave_buf_walk: *int = malloc(8240) 
    wave_buf_run: *int = malloc(8240) 
    wave_buf_monster: *int = malloc(8240)
    
    wave_buf_idle[0] = 1179011410
    wave_buf_walk[0] = 1179011410
    wave_buf_run[0] = 1179011410
    wave_buf_monster[0] = 1179011410
    wave_buf_idle[1] = 8228
    wave_buf_walk[1] = 8228
    wave_buf_run[1] = 8228
    wave_buf_monster[1] = 8228
    wave_buf_idle[2] = 1163280727
    wave_buf_walk[2] = 1163280727
    wave_buf_run[2] = 1163280727
    wave_buf_monster[2] = 1163280727
    wave_buf_idle[3] = 544501094
    wave_buf_walk[3] = 544501094
    wave_buf_run[3] = 544501094
    wave_buf_monster[3] = 544501094
    wave_buf_idle[4] = 16
    wave_buf_walk[4] = 16
    wave_buf_run[4] = 16
    wave_buf_monster[4] = 16
    wave_buf_idle[5] = 65537
    wave_buf_walk[5] = 65537
    wave_buf_run[5] = 65537
    wave_buf_monster[5] = 65537
    wave_buf_idle[6] = 8192
    wave_buf_walk[6] = 8192
    wave_buf_run[6] = 8192
    wave_buf_monster[6] = 8192
    wave_buf_idle[7] = 8192
    wave_buf_walk[7] = 8192
    wave_buf_run[7] = 8192
    wave_buf_monster[7] = 8192
    wave_buf_idle[8] = 524289
    wave_buf_walk[8] = 524289
    wave_buf_run[8] = 524289
    wave_buf_monster[8] = 524289
    wave_buf_idle[9] = 1635017060
    wave_buf_walk[9] = 1635017060
    wave_buf_run[9] = 1635017060
    wave_buf_monster[9] = 1635017060
    wave_buf_idle[10] = 8192
    wave_buf_walk[10] = 8192
    wave_buf_run[10] = 8192
    wave_buf_monster[10] = 8192

    wi: int = 0
    while 2048 > wi:
        val_idle: int = 0
        val_walk: int = 0
        val_run: int = 0
        byte_idx: int = 0
        while 4 > byte_idx:
            idx: int = wi * 4 + byte_idx
            t: f64 = (idx as f64) / 8192.0
            sf: f64 = sin(t * 170.0 * 6.2831853) * 0.4 + sin(t * 60.0 * 6.2831853) * 0.3
            
            t_w: f64 = t + 0.25
            if t_w >= 1.0:
                t_w = t_w - 1.0
            beat_w: f64 = t_w * 2.0
            beat_w_int: int = beat_w as int
            beat_w = beat_w - (beat_w_int as f64)
            thud_w: f64 = 0.0
            if 0.25 > beat_w:
                env_w: f64 = 1.0
                if 0.03 > beat_w:
                    env_w = beat_w / 0.03
                else:
                    env_w = 1.0 - ((beat_w - 0.03) / 0.22)
                thud_w = sin(beat_w * 45.0 * 6.2831853) * env_w * env_w * 1.5
                
            t_r: f64 = t + 0.166666
            if t_r >= 1.0:
                t_r = t_r - 1.0
            beat_r: f64 = t_r * 3.0
            beat_r_int: int = beat_r as int
            beat_r = beat_r - (beat_r_int as f64)
            thud_r: f64 = 0.0
            if 0.15 > beat_r:
                env_r: f64 = 1.0
                if 0.02 > beat_r:
                    env_r = beat_r / 0.02
                else:
                    env_r = 1.0 - ((beat_r - 0.02) / 0.13)
                thud_r = sin(beat_r * 55.0 * 6.2831853) * env_r * env_r * 1.8
                
            s_i: f64 = sf
            s_w: f64 = sf + thud_w
            s_r: f64 = sf + thud_r
            if s_i > 1.0:
                s_i = 1.0
            if -1.0 > s_i:
                s_i = -1.0
            if s_w > 1.0:
                s_w = 1.0
            if -1.0 > s_w:
                s_w = -1.0
            if s_r > 1.0:
                s_r = 1.0
            if -1.0 > s_r:
                s_r = -1.0
            b_i: int = (s_i * 80.0 + 128.0) as int
            b_w: int = (s_w * 80.0 + 128.0) as int
            b_r: int = (s_r * 80.0 + 128.0) as int
            
            if byte_idx == 0:
                val_idle = val_idle + b_i
                val_walk = val_walk + b_w
                val_run = val_run + b_r
            if byte_idx == 1:
                val_idle = val_idle + b_i * 256
                val_walk = val_walk + b_w * 256
                val_run = val_run + b_r * 256
            if byte_idx == 2:
                val_idle = val_idle + b_i * 65536
                val_walk = val_walk + b_w * 65536
                val_run = val_run + b_r * 65536
            if byte_idx == 3:
                val_idle = val_idle + b_i * 16777216
                val_walk = val_walk + b_w * 16777216
                val_run = val_run + b_r * 16777216
            byte_idx = byte_idx + 1
            
        wave_buf_idle[11 + wi] = val_idle
        wave_buf_walk[11 + wi] = val_walk
        wave_buf_run[11 + wi] = val_run
        wi = wi + 1

    PlaySoundA(wave_buf_idle as *void, 0 as *void, 13)

    cls_name: *void = "CBLGameClass".data as *void
    win_name: *void = "Backrooms. A new footage.".data as *void
    
    h_inst: *void = GetModuleHandleA(0 as *void)
    
    wc: *WNDCLASSA = malloc(sizeof(WNDCLASSA))
    wc.style = 3
    wc.lpfnWndProc = custom_wnd_proc as *void
    wc.cbClsExtra = 0
    wc.cbWndExtra = 0
    wc.hInstance = h_inst
    wc.hIcon = 0 as *void
    wc.hCursor = 0 as *void
    wc.hbrBackground = 0 as *void
    wc.lpszMenuName = 0 as *void
    wc.lpszClassName = cls_name
    
    RegisterClassA(wc as *void)

    hwnd: *void = CreateWindowExA(0, cls_name, win_name, 282001408, 100, 100, WINDOW_W, WINDOW_H, 0 as *void, 0 as *void, h_inst, 0 as *void)
    hdc: *void = GetDC(hwnd)
    
    pfd: *int = malloc(40)
    pfd[0] = 65576 
    pfd[1] = 37    
    pfd[2] = 8192  
    pfd[3] = 0
    pfd[4] = 0
    pfd[5] = 0
    pfd[6] = 0
    pfd[7] = 0
    pfd[8] = 0
    pfd[9] = 0

    pixel_format: int = ChoosePixelFormat(hdc, pfd)
    SetPixelFormat(hdc, pixel_format, pfd)
    hrc: *void = wglCreateContext(hdc)
    wglMakeCurrent(hdc, hrc)
    free(pfd)
    
    glEnable(2929) 
    glEnable(3553) 
    glClearColor(0.0 as float, 0.0 as float, 0.0 as float, 1.0 as float)
    
    glViewport(0, 0, WINDOW_W, WINDOW_H)

    tex_wall: int = create_texture(1)
    tex_floor: int = create_texture(2)
    tex_ceil: int = create_texture(3)
    tex_ent: int = create_texture(4)
    tex_vig: int = create_texture(5)
    tex_vhs: int = create_texture(6)

    msg_raw: *int = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
    msg_ptr: *MSG = msg_raw as *MSG
    msg_void: *void = msg_raw as *void

    game_state: int = 0         
    menu_debounce: int = 0
    bind_target: int = 0
    cursor_visible: int = 1
    
    is_fullscreen: int = 0
    f11_debounce: int = 0
    current_w: int = WINDOW_W
    current_h: int = WINDOW_H

    key_fwd: int = 87 
    key_bck: int = 83 
    key_lft: int = 65 
    key_rgt: int = 68 
    mouse_sens: f64 = 0.003
    pitch_offset: f64 = 0.0

    pt: *int = malloc(8)        
    key_buf: *int = malloc(4)   
    key_buf[0] = 0

    player_x: f64 = 1.5
    player_y: f64 = 1.5
    player_a: f64 = 0.0
    fov: f64 = 1.2    

    bob_timer: f64 = 0.0
    tremor_timer: f64 = 0.0
    blink_timer: f64 = 0.0
    eye_openness: f64 = 1.0
    
    is_running: int = 1
    last_move_state: int = -1

    ent_x: f64 = 0.0
    ent_y: f64 = 0.0
    ent_active: int = 0
    ent_state: int = 0
    ent_timer: f64 = 0.0
    ent_audio_timer: f64 = 1.0
    
    jumpscare_frames: int = 0
    last_time: int = GetTickCount()

    while is_running == 1:
        curr_time: int = GetTickCount()
        delta_ms: int = curr_time - last_time
        last_time = curr_time
        delta_s: f64 = (delta_ms as f64) / 1000.0
        
        frame_start: int = GetTickCount()
        
        while PeekMessageA(msg_void, 0 as *void, 0, 0, 1) != 0:
            if msg_ptr.message == WM_QUIT:
                is_running = 0
            DispatchMessageA(msg_void)

        if f11_debounce > 0:
            f11_debounce = f11_debounce - 1

        if GetAsyncKeyState(122) != 0: 
            if f11_debounce == 0:
                if is_fullscreen == 0:
                    is_fullscreen = 1
                    sw: int = GetSystemMetrics(0)
                    sh: int = GetSystemMetrics(1)
                    SetWindowLongA(hwnd, -16, -1879048192) 
                    SetWindowPos(hwnd, 0 as *void, 0, 0, sw, sh, 100)
                    current_w = sw
                    current_h = sh
                    glViewport(0, 0, sw, sh)
                else:
                    is_fullscreen = 0
                    SetWindowLongA(hwnd, -16, 282001408)
                    SetWindowPos(hwnd, 0 as *void, 100, 100, WINDOW_W, WINDOW_H, 100)
                    current_w = WINDOW_W
                    current_h = WINDOW_H
                    glViewport(0, 0, WINDOW_W, WINDOW_H)
                f11_debounce = 30

        if menu_debounce > 0:
            menu_debounce = menu_debounce - 1

        if GetAsyncKeyState(27) != 0:
            if menu_debounce == 0:
                if game_state == 1:
                    game_state = 0
                else:
                    if game_state == 0:
                        game_state = 1
                menu_debounce = 15

        if game_state == 0:
            if GetAsyncKeyState(49) != 0:
                if menu_debounce == 0: 
                    game_state = 1
                    menu_debounce = 15
            if GetAsyncKeyState(50) != 0:
                if menu_debounce == 0: 
                    game_state = 2
                    menu_debounce = 15
            if GetAsyncKeyState(51) != 0:
                if menu_debounce == 0: 
                    is_running = 0

        if game_state == 2:
            if GetAsyncKeyState(49) != 0:
                if menu_debounce == 0: 
                    mouse_sens = mouse_sens + 0.001
                    if mouse_sens > 0.01:
                        mouse_sens = 0.001
                    menu_debounce = 15
            if GetAsyncKeyState(50) != 0:
                if menu_debounce == 0: 
                    game_state = 3
                    bind_target = 1
                    menu_debounce = 15
            if GetAsyncKeyState(51) != 0:
                if menu_debounce == 0: 
                    game_state = 3
                    bind_target = 2
                    menu_debounce = 15
            if GetAsyncKeyState(52) != 0:
                if menu_debounce == 0: 
                    game_state = 3
                    bind_target = 3
                    menu_debounce = 15
            if GetAsyncKeyState(53) != 0:
                if menu_debounce == 0: 
                    game_state = 3
                    bind_target = 4
                    menu_debounce = 15
            if GetAsyncKeyState(54) != 0:
                if menu_debounce == 0: 
                    game_state = 0
                    menu_debounce = 15

        if game_state == 3:
            k: int = 8
            while 255 > k:
                if GetAsyncKeyState(k) != 0:
                    if menu_debounce == 0:
                        if bind_target == 1:
                            key_fwd = k
                        if bind_target == 2:
                            key_bck = k
                        if bind_target == 3:
                            key_lft = k
                        if bind_target == 4:
                            key_rgt = k
                        game_state = 2
                        menu_debounce = 15
                        k = 256
                k = k + 1

        if game_state == 1:
            if cursor_visible == 1:
                ShowCursor(0)
                cursor_visible = 0
        if game_state != 1:
            if cursor_visible == 0:
                ShowCursor(1)
                cursor_visible = 1

        is_moving: int = 0
        if game_state == 1:
            if jumpscare_frames == 0:
                if GetAsyncKeyState(key_fwd) != 0:
                    is_moving = 1
                if GetAsyncKeyState(key_bck) != 0:
                    is_moving = 1
                if GetAsyncKeyState(key_lft) != 0:
                    is_moving = 1
                if GetAsyncKeyState(key_rgt) != 0:
                    is_moving = 1

        move_state: int = 0
        if is_moving == 1:
            if GetAsyncKeyState(16) != 0:
                move_state = 2
            else:
                move_state = 1
        else:
            move_state = 0
            
        move_speed: f64 = 0.02
        bob_amp: f64 = 0.03
        target_fov: f64 = 1.2
        
        if move_state == 2: 
            move_speed = 0.045
            bob_amp = 0.06
            target_fov = 1.45

        if move_state != last_move_state:
            if jumpscare_frames == 0:
                if game_state == 1:
                    last_move_state = move_state
                    bob_timer = 3.14159 
                    if ent_state == 0:
                        if move_state == 2:
                            PlaySoundA(wave_buf_run as *void, 0 as *void, 13)
                        if move_state == 1:
                            PlaySoundA(wave_buf_walk as *void, 0 as *void, 13)
                        if move_state == 0:
                            PlaySoundA(wave_buf_idle as *void, 0 as *void, 13)

        if is_moving == 1:
            if move_state == 2:
                bob_timer = bob_timer + delta_s * 18.8495559
            else:
                bob_timer = bob_timer + delta_s * 12.5663706

        if player_x > 1.0:
            if 2.0 > player_x:
                if player_y > 28.0:
                    player_y = 2.0 
        if player_x > 3.0:
            if 5.0 > player_x:
                if player_y > 1.0:
                    if 2.0 > player_y:
                        view_dir: f64 = sin(player_a)
                        if view_dir > 0.5:
                            world_map[1 * MAP_WIDTH + 2] = 1
                        if -0.5 > view_dir:
                            world_map[1 * MAP_WIDTH + 2] = 0

        blink_timer = blink_timer + delta_s
        if blink_timer > 5.0:
            blink_timer = 0.0
            
        eye_openness = 1.0
        if blink_timer > 4.7:
            phase_b: f64 = (blink_timer - 4.7) / 0.3
            closed: f64 = sin(phase_b * 3.14159)
            eye_openness = 1.0 - closed

        if game_state == 1:
            if jumpscare_frames == 0:
                pt[0] = current_w / 2
                pt[1] = current_h / 2
                ClientToScreen(hwnd, pt as *void)
                c_x: int = pt[0]
                c_y: int = pt[1]

                GetCursorPos(pt as *void)
                d_x: int = pt[0] - c_x
                d_y: int = pt[1] - c_y

                if d_x != 0:
                    player_a = player_a - (d_x as f64) * mouse_sens
                    SetCursorPos(c_x, c_y)
                if d_y != 0:
                    pitch_offset = pitch_offset + (d_y as f64) * mouse_sens * 60.0
                    if pitch_offset > 80.0:
                        pitch_offset = 80.0
                    if -80.0 > pitch_offset:
                        pitch_offset = -80.0
                    SetCursorPos(c_x, c_y)

        if ent_active == 0:
            rx_int: int = rand() % (MAP_WIDTH - 2) + 1
            ry_int: int = rand() % (MAP_HEIGHT - 2) + 1
            c1: int = get_map(rx_int, ry_int)
            c2: int = get_map(rx_int + 1, ry_int)
            c3: int = get_map(rx_int, ry_int + 1)
            c4: int = get_map(rx_int + 1, ry_int + 1)
            
            if 2 > (c1 + c2 + c3 + c4):
                dx_s: f64 = (rx_int as f64) - player_x
                dy_s: f64 = (ry_int as f64) - player_y
                if (dx_s * dx_s + dy_s * dy_s) > 40.0:
                    r_spawn: int = rand()
                    if 60 > (r_spawn % 100):
                        ent_x = (rx_int as f64) + 0.5
                        ent_y = (ry_int as f64) + 0.5
                        ent_active = 1
                        ent_state = 0
                        ent_timer = 0.0

        ent_dist_approx: f64 = 999.0
        
        if ent_active == 1:
            if game_state == 1:
                if jumpscare_frames == 0:
                    ent_timer = ent_timer + delta_s
                    if ent_timer > 15.0:
                        ent_active = 0
                        ent_state = 0

        if ent_active == 1:
            dx_e: f64 = player_x - ent_x
            dy_e: f64 = player_y - ent_y
            dist_e_sq: f64 = dx_e * dx_e + dy_e * dy_e
            
            abs_dx: f64 = dx_e
            if 0.0 > abs_dx:
                abs_dx = 0.0 - abs_dx
            abs_dy: f64 = dy_e
            if 0.0 > abs_dy:
                abs_dy = 0.0 - abs_dy
            ent_dist_approx = abs_dx + abs_dy
            if 0.01 > ent_dist_approx:
                ent_dist_approx = 0.01
                
            if ent_state == 0:
                if jumpscare_frames == 0:
                    if 100.0 > dist_e_sq:
                        has_los: int = 1
                        ray_steps: int = 20
                        ray_dx: f64 = dx_e / 20.0
                        ray_dy: f64 = dy_e / 20.0
                        cx: f64 = ent_x
                        cy: f64 = ent_y
                        step: int = 0
                        
                        while ray_steps > step:
                            cx = cx + ray_dx
                            cy = cy + ray_dy
                            cx_i: int = cx as int
                            cy_i: int = cy as int
                            if 0 > cx_i:
                                has_los = 0
                                step = ray_steps
                            if cx_i >= MAP_WIDTH:
                                has_los = 0
                                step = ray_steps
                            if 0 > cy_i:
                                has_los = 0
                                step = ray_steps
                            if cy_i >= MAP_HEIGHT:
                                has_los = 0
                                step = ray_steps
                            if has_los == 1:
                                if world_map[cy_i * MAP_WIDTH + cx_i] > 0:
                                    has_los = 0
                                    step = ray_steps 
                            step = step + 1
                        
                        if has_los == 1:
                            ent_state = 1 
                        
            if ent_state == 1:
                if jumpscare_frames == 0:
                    if game_state == 1:
                        if dist_e_sq > 625.0:
                            ent_active = 0
                            ent_state = 0
                        else:
                            ent_speed: f64 = 0.044775
                            nx_e: f64 = ent_x + (dx_e / ent_dist_approx) * ent_speed
                            ny_e: f64 = ent_y + (dy_e / ent_dist_approx) * ent_speed
                            
                            if get_map(nx_e as int, ent_y as int) == 0:
                                ent_x = nx_e
                            if get_map(ent_x as int, ny_e as int) == 0:
                                ent_y = ny_e
                            
                            if 0.16 > dist_e_sq:
                                jumpscare_frames = 30
                                m_wi_j: int = 0
                                while 2048 > m_wi_j:
                                    m_val_j: int = 0
                                    m_byte_j: int = 0
                                    while 4 > m_byte_j:
                                        m_t_j: f64 = ((m_wi_j * 4 + m_byte_j) as f64) / 8192.0
                                        phase_j: f64 = m_t_j * 800.0
                                        phase_int_j: int = phase_j as int
                                        fract_j: f64 = phase_j - (phase_int_j as f64)
                                        sq_val_j: f64 = -1.0
                                        if 0.5 > fract_j:
                                            sq_val_j = 1.0
                                        r_n_j: int = rand()
                                        b_m_j: int = (sq_val_j * 60.0 + ((r_n_j % 255) as f64) * 0.3 + 128.0) as int
                                        
                                        if m_byte_j == 0:
                                            m_val_j = m_val_j + b_m_j
                                        if m_byte_j == 1:
                                            m_val_j = m_val_j + b_m_j * 256
                                        if m_byte_j == 2:
                                            m_val_j = m_val_j + b_m_j * 65536
                                        if m_byte_j == 3:
                                            m_val_j = m_val_j + b_m_j * 16777216
                                        m_byte_j = m_byte_j + 1
                                        
                                    wave_buf_monster[11 + m_wi_j] = m_val_j
                                    m_wi_j = m_wi_j + 1
                                PlaySoundA(wave_buf_monster as *void, 0 as *void, 13)

        if ent_state == 1:
            if jumpscare_frames == 0:
                if game_state == 1:
                    ent_audio_timer = ent_audio_timer + delta_s
                    if ent_audio_timer > 0.4:
                        ent_audio_timer = 0.0
                        vol_mult: f64 = 1.0 / (ent_dist_approx + 1.0)
                        if vol_mult > 1.0:
                            vol_mult = 1.0
                        
                        base_freq: f64 = 290.0
                        r_mod: int = rand() % 3
                        if r_mod == 0:
                            base_freq = 260.0
                        if r_mod == 2:
                            base_freq = 320.0
                        
                        m_wi: int = 0
                        while 2048 > m_wi:
                            m_val: int = 0
                            m_byte: int = 0
                            while 4 > m_byte:
                                m_t: f64 = ((m_wi * 4 + m_byte) as f64) / 8192.0
                                phase_s: f64 = m_t * base_freq
                                phase_int_s: int = phase_s as int
                                fract_s: f64 = phase_s - (phase_int_s as f64)
                                sq_val_s: f64 = -1.0
                                if 0.5 > fract_s:
                                    sq_val_s = 1.0
                                sq_val_s = sq_val_s * vol_mult * 0.9
                                b_m: int = (sq_val_s * 80.0 + 128.0) as int
                                
                                if m_byte == 0:
                                    m_val = m_val + b_m
                                if m_byte == 1:
                                    m_val = m_val + b_m * 256
                                if m_byte == 2:
                                    m_val = m_val + b_m * 65536
                                if m_byte == 3:
                                    m_val = m_val + b_m * 16777216
                                m_byte = m_byte + 1
                                
                            wave_buf_monster[11 + m_wi] = m_val
                            m_wi = m_wi + 1
                        PlaySoundA(wave_buf_monster as *void, 0 as *void, 5)

        tremor_timer = tremor_timer + delta_s * 0.24 
        cam_roll: f64 = sin(tremor_timer * 1.2) * 0.01
        fov = fov + (target_fov - fov) * 0.1

        cam_pitch_f: f64 = sin(tremor_timer * 1.9) * 3.0
        bob_z: f64 = 0.0
        
        if is_moving == 1:
            cam_roll = cam_roll + sin(bob_timer * 0.5) * 0.015
            cam_pitch_f = cam_pitch_f + sin(bob_timer * 1.0) * 6.0
            bob_z = (0.0 - cos(bob_timer)) * bob_amp 

        cam_pitch_f = cam_pitch_f + pitch_offset
        
        tape_noise: f64 = ((rand() % 10) as f64 - 5.0) * 0.002
        cam_pitch_f = cam_pitch_f + tape_noise * 3.0
        cam_roll = cam_roll + tape_noise * 0.5 
        
        flicker: f64 = 1.0 + sin(tremor_timer * 12.0) * 0.05
        if (rand() % 60) == 1:
            flicker = flicker * 0.3

        glClear(16640) 

        glMatrixMode(5889)
        glLoadIdentity()
        
        aspect: f64 = (current_w as f64) / (current_h as f64)
        znear: f64 = 0.02
        zfar: f64 = 55.0
        
        half_fov: f64 = fov * 0.5
        tan_half_fov: f64 = tan(half_fov)
        fh: f64 = znear * tan_half_fov
        fw: f64 = fh * aspect
        glFrustum(0.0 - fw, fw, 0.0 - fh, fh, znear, zfar)

        glMatrixMode(5888)
        glLoadIdentity()

        cam_pitch_deg: f64 = cam_pitch_f * 0.3
        cam_roll_deg: f64 = cam_roll * 57.2957
        yaw_deg: f64 = player_a * 57.2957
        yaw_deg = yaw_deg + tape_noise * 2.0

        glRotated(cam_pitch_deg, 1.0, 0.0, 0.0)
        glRotated(cam_roll_deg, 0.0, 0.0, 1.0)
        glRotated(180.0 - yaw_deg, 0.0, 1.0, 0.0)

        cam_y: f64 = 0.5 + bob_z
        glTranslated(0.0 - player_x, 0.0 - cam_y, 0.0 - player_y)

        glEnable(2912) 
        glFogi(2917, 2048) 
        fog_den: f64 = 0.06 / flicker
        if jumpscare_frames > 0:
            fog_den = 0.0
        glFogf(2914, fog_den as float) 

        draw_world(tex_wall, tex_floor, tex_ceil, player_x, player_y, player_a, fov, vis_map)

        if ent_active == 1:
            glEnable(3042)
            glBlendFunc(770, 771) 
            glBindTexture(3553, tex_ent)
            glPushMatrix()
            glTranslated(ent_x, 0.5, ent_y)
            glRotated(yaw_deg - 180.0, 0.0, 1.0, 0.0)
            glColor3d(1.0, 1.0, 1.0)
            glBegin(7)
            glTexCoord2d(0.0, 1.0)
            glVertex3d(-0.4, -0.5, 0.0)
            glTexCoord2d(1.0, 1.0)
            glVertex3d(0.4, -0.5, 0.0)
            glTexCoord2d(1.0, 0.0)
            glVertex3d(0.4, 0.5, 0.0)
            glTexCoord2d(0.0, 0.0)
            glVertex3d(-0.4, 0.5, 0.0)
            glEnd()
            glPopMatrix()
            glDisable(3042)

        glDisable(2912) 

        glMatrixMode(5889)
        glLoadIdentity()
        glOrtho(0.0, 1.0, 1.0, 0.0, -1.0, 1.0)
        glMatrixMode(5888)
        glLoadIdentity()
        glDisable(2929) 
        
        if game_state == 1:
            glEnable(3042)
            glBlendFunc(770, 771)
            glBindTexture(3553, tex_vig)
            glColor4d(1.0, 1.0, 1.0, 1.0)
            glBegin(7)
            glTexCoord2d(0.0, 0.0)
            glVertex3d(0.0, 0.0, 0.0)
            glTexCoord2d(1.0, 0.0)
            glVertex3d(1.0, 0.0, 0.0)
            glTexCoord2d(1.0, 1.0)
            glVertex3d(1.0, 1.0, 0.0)
            glTexCoord2d(0.0, 1.0)
            glVertex3d(0.0, 1.0, 0.0)
            glEnd()
            glDisable(3042)

            glEnable(3042)
            glBlendFunc(770, 771)
            glBindTexture(3553, tex_vhs)
            shift_x: f64 = ((rand() % 100) as f64) / 100.0
            glColor4d(1.0, 1.0, 1.0, 1.0)
            glBegin(7)
            v_rep: f64 = 15.0
            glTexCoord2d(shift_x, 0.0)
            glVertex3d(0.0, 0.0, 0.0)
            glTexCoord2d(1.0 + shift_x, 0.0)
            glVertex3d(1.0, 0.0, 0.0)
            glTexCoord2d(1.0 + shift_x, v_rep)
            glVertex3d(1.0, 1.0, 0.0)
            glTexCoord2d(shift_x, v_rep)
            glVertex3d(0.0, 1.0, 0.0)
            glEnd()
            glDisable(3042)

            if (rand() % 100) > 65:
                glEnable(3042)
                glDisable(3553)
                glBlendFunc(770, 771)
                glColor4d(0.8, 0.8, 0.8, 0.25)
                glBegin(7)
                band_y: f64 = 0.75 + ((rand() % 20) as f64) * 0.01
                band_h: f64 = 0.15
                glVertex3d(0.0, band_y, 0.0)
                glVertex3d(1.0, band_y, 0.0)
                glVertex3d(1.0, band_y + band_h, 0.0)
                glVertex3d(0.0, band_y + band_h, 0.0)
                glEnd()
                glEnable(3553)
                glDisable(3042)
                
            glEnable(3042)
            glDisable(3553)
            glBlendFunc(770, 771)
            glColor4d(0.0, 0.05, 0.1, 0.15)
            glBegin(7)
            glVertex3d(0.0, 0.0, 0.0)
            glVertex3d(1.0, 0.0, 0.0)
            glVertex3d(1.0, 1.0, 0.0)
            glVertex3d(0.0, 1.0, 0.0)
            glEnd()
            glEnable(3553)
            glDisable(3042)

        if jumpscare_frames > 0:
            glEnable(3042)
            glBlendFunc(330, 0) 
            glDisable(3553)
            glColor3d(1.0, 1.0, 1.0)
            glBegin(7)
            glVertex3d(0.0, 0.0, 0.0)
            glVertex3d(1.0, 0.0, 0.0)
            glVertex3d(1.0, 1.0, 0.0)
            glVertex3d(0.0, 1.0, 0.0)
            glEnd()
            glBlendFunc(770, 771)
            rnd_op: f64 = ((rand() % 100) as f64) / 100.0
            glColor4d(1.0, 0.0, 0.0, rnd_op)
            glBegin(7)
            glVertex3d(0.0, 0.0, 0.0)
            glVertex3d(1.0, 0.0, 0.0)
            glVertex3d(1.0, 1.0, 0.0)
            glVertex3d(0.0, 1.0, 0.0)
            glEnd()
            glEnable(3553)
            glDisable(3042)

        if eye_openness < 1.0:
            if jumpscare_frames == 0:
                glEnable(3042)
                glDisable(3553)
                glBlendFunc(770, 771)
                e_diff: f64 = 1.0 - eye_openness
                glColor4d(0.0, 0.0, 0.0, e_diff)
                glBegin(7)
                glVertex3d(0.0, 0.0, 0.0)
                glVertex3d(1.0, 0.0, 0.0)
                glVertex3d(1.0, 1.0, 0.0)
                glVertex3d(0.0, 1.0, 0.0)
                glEnd()
                glEnable(3553)
                glDisable(3042)

        if game_state != 1:
            glEnable(3042)
            glDisable(3553)
            glBlendFunc(770, 771)
            glColor4d(0.0, 0.0, 0.0, 0.75)
            glBegin(7)
            glVertex3d(0.0, 0.0, 0.0)
            glVertex3d(1.0, 0.0, 0.0)
            glVertex3d(1.0, 1.0, 0.0)
            glVertex3d(0.0, 1.0, 0.0)
            glEnd()
            glEnable(3553)
            glDisable(3042)

        glEnable(2929)
        SwapBuffers(hdc)

        if game_state != 1:
            SetBkMode(hdc, 1)
            SetTextColor(hdc, 16777215)
            
            cen_x: int = current_w / 2
            cen_y: int = current_h / 2

            if game_state == 0:
                TextOutA(hdc, cen_x - 80, cen_y - 85, "--- BACKROOMS ---".data as *void, 17)
                TextOutA(hdc, cen_x - 80, cen_y - 35, "1. PLAY".data as *void, 7)
                TextOutA(hdc, cen_x - 80, cen_y + 15, "2. SETTINGS".data as *void, 11)
                TextOutA(hdc, cen_x - 80, cen_y + 65, "3. EXIT".data as *void, 7)
                TextOutA(hdc, cen_x - 80, cen_y + 115, "F11. FULLSCREEN".data as *void, 15)

            if game_state == 2:
                TextOutA(hdc, cen_x - 80, cen_y - 160, "--- SETTINGS ---".data as *void, 16)
                TextOutA(hdc, cen_x - 80, cen_y - 110, "1. SENSITIVITY (CLICK TO CYCLE)".data as *void, 31)
                TextOutA(hdc, cen_x - 80, cen_y - 60, "2. BIND FORWARD: ".data as *void, 17)
                key_buf[0] = key_fwd
                TextOutA(hdc, cen_x + 120, cen_y - 60, key_buf as *void, 1)
                TextOutA(hdc, cen_x - 80, cen_y - 10, "3. BIND BACKWARD: ".data as *void, 18)
                key_buf[0] = key_bck
                TextOutA(hdc, cen_x + 120, cen_y - 10, key_buf as *void, 1)
                TextOutA(hdc, cen_x - 80, cen_y + 40, "4. BIND LEFT: ".data as *void, 14)
                key_buf[0] = key_lft
                TextOutA(hdc, cen_x + 120, cen_y + 40, key_buf as *void, 1)
                TextOutA(hdc, cen_x - 80, cen_y + 90, "5. BIND RIGHT: ".data as *void, 15)
                key_buf[0] = key_rgt
                TextOutA(hdc, cen_x + 120, cen_y + 90, key_buf as *void, 1)
                TextOutA(hdc, cen_x - 80, cen_y + 140, "6. BACK".data as *void, 7)

            if game_state == 3:
                TextOutA(hdc, cen_x - 80, cen_y, "PRESS ANY KEY...".data as *void, 16)

        if jumpscare_frames > 0:
            jumpscare_frames = jumpscare_frames - 1
            if jumpscare_frames == 0:
                is_running = 0

        if jumpscare_frames == 0:
            if game_state == 1:
                next_x: f64 = player_x
                next_y: f64 = player_y

                if GetAsyncKeyState(key_fwd) != 0:
                    next_x = next_x + sin(player_a) * move_speed
                    next_y = next_y + cos(player_a) * move_speed
                if GetAsyncKeyState(key_bck) != 0:
                    next_x = next_x - sin(player_a) * move_speed
                    next_y = next_y - cos(player_a) * move_speed
                if GetAsyncKeyState(key_lft) != 0:
                    next_x = next_x + cos(player_a) * move_speed
                    next_y = next_y - sin(player_a) * move_speed
                if GetAsyncKeyState(key_rgt) != 0:
                    next_x = next_x - cos(player_a) * move_speed
                    next_y = next_y + sin(player_a) * move_speed

                pad: f64 = 0.25
                nx_p: int = (next_x + pad) as int
                nx_m: int = (next_x - pad) as int
                py_p: int = (player_y + pad) as int
                py_m: int = (player_y - pad) as int

                if get_map(nx_p, py_p) == 0:
                    if get_map(nx_p, py_m) == 0:
                        if get_map(nx_m, py_p) == 0:
                            if get_map(nx_m, py_m) == 0:
                                player_x = next_x
                    
                ny_p: int = (next_y + pad) as int
                ny_m: int = (next_y - pad) as int
                px_p: int = (player_x + pad) as int
                px_m: int = (player_x - pad) as int

                if get_map(px_p, ny_p) == 0:
                    if get_map(px_p, ny_m) == 0:
                        if get_map(px_m, ny_p) == 0:
                            if get_map(px_m, ny_m) == 0:
                                player_y = next_y

                if GetAsyncKeyState(69) != 0:
                    look_x: int = (player_x + sin(player_a) * 1.5) as int
                    look_y: int = (player_y + cos(player_a) * 1.5) as int
                    if look_x >= 0:
                        if MAP_WIDTH > look_x:
                            if look_y >= 0:
                                if MAP_HEIGHT > look_y:
                                    if get_map(look_x, look_y) == 2:
                                        world_map[look_y * MAP_WIDTH + look_x] = 0 
            
        frame_time: int = GetTickCount() - frame_start
        if 16 > frame_time:
            Sleep(16 - frame_time)
        else:
            Sleep(1)

    PlaySoundA(0 as *void, 0 as *void, 0)
    wglMakeCurrent(0 as *void, 0 as *void)
    
    free(wave_buf_idle)
    free(wave_buf_walk)
    free(wave_buf_run)
    free(wave_buf_monster)
    free(pt)
    free(key_buf)
    free(vis_map)
    
    ShowCursor(1) 
    ReleaseDC(hwnd, hdc)
    ExitProcess(0)
    endofcode