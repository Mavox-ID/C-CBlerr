extern def MessageBoxA(hwnd: u64, text: str, caption: str, type: u32) -> i32
extern def ExitProcess(exitCode: u32) -> void
extern def Sleep(ms: u32) -> void
extern def RtlAdjustPrivilege(Privilege: u32, Enable: i32, CurrentThread: i32, Enabled: *i32) -> u32
extern def NtRaiseHardError(Status: u32, Params: u32, Mask: u32, Ptr: u64, Opt: u32, Res: *u32) -> u32

def main() -> int:
    msgbox_type: u32 = 52
    
    let user_choice = MessageBoxA(0, "Execute BSOD?", "WARNING", msgbox_type)
    
    if user_choice == 7:
        ExitProcess(0)
        endofcode
        
    if user_choice == 6:
        dummy: i32 = 0
        RtlAdjustPrivilege(19, 1, 0, &dummy)
        
        Sleep(1000)
        
        response: u32 = 0
        status: u32 = 3221225473 
        
        NtRaiseHardError(status, 0, 0, 0, 6, &response)
        
    endofcode