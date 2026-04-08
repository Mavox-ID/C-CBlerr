struct WNDCLASSEXA:
    cbSize: u32
    lpfnWndProc: (u64, u32, u64, u64) -> u64  # Тип указателя на функцию (§6.4)
    lpszClassName: str

extern def CreateWindowExA(dwExStyle: u64, lpClassName: str, lpWindowName: str, dwStyle: u64, x: i32, y: i32, nWidth: i32, nHeight: i32, hWndParent: u64, hMenu: u64, hInstance: u64, lpParam: u64) -> u64
extern def RegisterClassExA(lpwcx: u8*) -> u16
extern def ShowWindow(hWnd: u64, nCmdShow: i32) -> i32

def WndProc(hwnd: u64, msg: u32, wparam: u64, lparam: u64) -> u64:
    return 0 as u64

def main() -> void:
    let wc: WNDCLASSEXA = WNDCLASSEXA { 
        cbSize: sizeof(WNDCLASSEXA) as u32,
        lpszClassName: "CBlerr Calculator" 
    }
    
    wc.lpfnWndProc = WndProc 

    let hwnd: u64 = CreateWindowExA(
        0 as u64, 
        wc.lpszClassName, 
        "CBlerr Calculator", 
        0xCF0000 as u64, 
        100, 
        100, 
        300, 
        400, 
        0 as u64, 
        0 as u64, 
        0 as u64, 
        0 as u64
    )
    
    ShowWindow(hwnd, 1)