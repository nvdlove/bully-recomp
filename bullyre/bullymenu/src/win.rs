//! The Win32 API surface the menu uses, declared by hand.

#![allow(non_snake_case, non_camel_case_types, clippy::upper_case_acronyms, dead_code)]

use std::ffi::c_void;

pub type HANDLE = *mut c_void;
pub type HWND = *mut c_void;
pub type HDC = *mut c_void;
pub type HGDIOBJ = *mut c_void;
pub type HINSTANCE = *mut c_void;
pub type HMENU = *mut c_void;
pub type HKEY = *mut c_void;
pub type BOOL = i32;
pub type WPARAM = usize;
pub type LPARAM = isize;
pub type LRESULT = isize;
pub type WNDPROC = unsafe extern "system" fn(HWND, u32, WPARAM, LPARAM) -> LRESULT;

#[repr(C)]
#[derive(Clone, Copy, Default, Debug, PartialEq, Eq)]
pub struct RECT {
    pub left: i32,
    pub top: i32,
    pub right: i32,
    pub bottom: i32,
}

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct POINT {
    pub x: i32,
    pub y: i32,
}

#[repr(C)]
pub struct MSG {
    pub hwnd: HWND,
    pub message: u32,
    pub wParam: WPARAM,
    pub lParam: LPARAM,
    pub time: u32,
    pub pt: POINT,
    pub private: u32,
}

#[repr(C)]
pub struct WNDCLASSEXW {
    pub cbSize: u32,
    pub style: u32,
    pub lpfnWndProc: WNDPROC,
    pub cbClsExtra: i32,
    pub cbWndExtra: i32,
    pub hInstance: HINSTANCE,
    pub hIcon: HANDLE,
    pub hCursor: HANDLE,
    pub hbrBackground: HANDLE,
    pub lpszMenuName: *const u16,
    pub lpszClassName: *const u16,
    pub hIconSm: HANDLE,
}

#[repr(C)]
pub struct STARTUPINFOW {
    pub cb: u32,
    pub lpReserved: *mut u16,
    pub lpDesktop: *mut u16,
    pub lpTitle: *mut u16,
    pub dwX: u32,
    pub dwY: u32,
    pub dwXSize: u32,
    pub dwYSize: u32,
    pub dwXCountChars: u32,
    pub dwYCountChars: u32,
    pub dwFillAttribute: u32,
    pub dwFlags: u32,
    pub wShowWindow: u16,
    pub cbReserved2: u16,
    pub lpReserved2: *mut u8,
    pub hStdInput: HANDLE,
    pub hStdOutput: HANDLE,
    pub hStdError: HANDLE,
}

#[repr(C)]
pub struct PROCESS_INFORMATION {
    pub hProcess: HANDLE,
    pub hThread: HANDLE,
    pub dwProcessId: u32,
    pub dwThreadId: u32,
}

#[repr(C)]
pub struct INITCOMMONCONTROLSEX {
    pub dwSize: u32,
    pub dwICC: u32,
}

#[repr(C)]
pub struct PAINTSTRUCT {
    pub hdc: HDC,
    pub fErase: BOOL,
    pub rcPaint: RECT,
    pub fRestore: BOOL,
    pub fIncUpdate: BOOL,
    pub rgbReserved: [u8; 32],
}

#[repr(C)]
pub struct NONCLIENTMETRICSW {
    pub cbSize: u32,
    pub rest: [u8; 500 - 4],
}

// window messages and styles
pub const WM_CREATE: u32 = 0x0001;
pub const WM_DESTROY: u32 = 0x0002;
pub const WM_CLOSE: u32 = 0x0010;
pub const WM_PAINT: u32 = 0x000F;
pub const WM_SETFONT: u32 = 0x0030;
pub const WM_COMMAND: u32 = 0x0111;
pub const WM_TIMER: u32 = 0x0113;
pub const WM_HSCROLL: u32 = 0x0114;
pub const WM_CTLCOLORSTATIC: u32 = 0x0138;
pub const WM_CTLCOLORBTN: u32 = 0x0135;
pub const WM_ERASEBKGND: u32 = 0x0014;
pub const WM_NCHITTEST: u32 = 0x0084;
pub const WM_DPICHANGED: u32 = 0x02E0;

pub const WS_OVERLAPPED: u32 = 0x0000_0000;
pub const WS_POPUP: u32 = 0x8000_0000;
pub const WS_CHILD: u32 = 0x4000_0000;
pub const WS_VISIBLE: u32 = 0x1000_0000;
pub const WS_CLIPSIBLINGS: u32 = 0x0400_0000;
pub const WS_CAPTION: u32 = 0x00C0_0000;
pub const WS_SYSMENU: u32 = 0x0008_0000;
pub const WS_THICKFRAME: u32 = 0x0004_0000;
pub const WS_MINIMIZEBOX: u32 = 0x0002_0000;
pub const WS_MAXIMIZEBOX: u32 = 0x0001_0000;
pub const WS_TABSTOP: u32 = 0x0001_0000;
pub const WS_GROUP: u32 = 0x0002_0000;
pub const WS_BORDER: u32 = 0x0080_0000;
pub const WS_MINIMIZE: u32 = 0x2000_0000;

pub const WS_EX_TOPMOST: u32 = 0x0000_0008;
pub const WS_EX_TRANSPARENT: u32 = 0x0000_0020;
pub const WS_EX_TOOLWINDOW: u32 = 0x0000_0080;
pub const WS_EX_LAYERED: u32 = 0x0008_0000;
pub const WS_EX_NOACTIVATE: u32 = 0x0800_0000;
pub const WS_EX_CLIENTEDGE: u32 = 0x0000_0200;

pub const BS_PUSHBUTTON: u32 = 0x0;
pub const BS_AUTOCHECKBOX: u32 = 0x3;
pub const BS_GROUPBOX: u32 = 0x7;
pub const ES_NUMBER: u32 = 0x2000;
pub const ES_AUTOHSCROLL: u32 = 0x80;
pub const SS_LEFT: u32 = 0x0;
pub const BM_GETCHECK: u32 = 0x00F0;
pub const BM_SETCHECK: u32 = 0x00F1;
pub const BST_CHECKED: usize = 1;
pub const BN_CLICKED: u16 = 0;

pub const TBS_AUTOTICKS: u32 = 0x1;
pub const TBS_HORZ: u32 = 0x0;
pub const TBM_GETPOS: u32 = 0x0400;
pub const TBM_SETPOS: u32 = 0x0405;
pub const TBM_SETRANGE: u32 = 0x0406;
pub const TBM_SETTICFREQ: u32 = 0x0414;
pub const TBM_SETPAGESIZE: u32 = 0x0415;
pub const ICC_BAR_CLASSES: u32 = 0x4;
pub const ICC_STANDARD_CLASSES: u32 = 0x4000;

pub const GWL_STYLE: i32 = -16;
pub const GWL_EXSTYLE: i32 = -20;
pub const SW_HIDE: i32 = 0;
pub const SW_SHOWNOACTIVATE: i32 = 4;
pub const SW_SHOW: i32 = 5;
pub const SW_RESTORE: i32 = 9;
pub const SWP_NOSIZE: u32 = 0x1;
pub const SWP_NOMOVE: u32 = 0x2;
pub const SWP_NOZORDER: u32 = 0x4;
pub const SWP_NOACTIVATE: u32 = 0x10;
pub const SWP_FRAMECHANGED: u32 = 0x20;
pub const SWP_SHOWWINDOW: u32 = 0x40;
pub const HWND_TOPMOST: HWND = -1isize as HWND;
pub const HWND_TOP: HWND = std::ptr::null_mut();
pub const GW_HWNDPREV: u32 = 3;
pub const LWA_COLORKEY: u32 = 0x1;
pub const IDC_ARROW: usize = 32512;
pub const COLOR_WINDOW: usize = 5;
pub const MB_OK: u32 = 0;
pub const MB_ICONERROR: u32 = 0x10;
pub const SPI_GETWORKAREA: u32 = 0x30;
pub const SPI_GETNONCLIENTMETRICS: u32 = 0x29;
pub const DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2: HANDLE = -4isize as HANDLE;

// processes and memory
pub const PROCESS_ALL_ACCESS: u32 = 0x001F_FFFF;
pub const CREATE_SUSPENDED: u32 = 0x4;
pub const CREATE_UNICODE_ENVIRONMENT: u32 = 0x400;
pub const PAGE_EXECUTE_READWRITE: u32 = 0x40;
pub const STILL_ACTIVE: u32 = 259;

// registry
pub const HKEY_CURRENT_USER: HKEY = 0x8000_0001usize as HKEY;
pub const KEY_ALL_ACCESS: u32 = 0xF003F;
pub const REG_DWORD: u32 = 4;

// GDI
pub const SRCCOPY: u32 = 0x00CC_0020;
pub const TRANSPARENT: i32 = 1;
pub const PS_SOLID: i32 = 0;
pub const NULL_BRUSH: i32 = 5;
pub const FW_NORMAL: i32 = 400;
pub const FW_BOLD: i32 = 700;
pub const NONANTIALIASED_QUALITY: u32 = 3;
pub const CLEARTYPE_QUALITY: u32 = 5;
pub const DEFAULT_CHARSET: u32 = 1;

#[link(name = "kernel32")]
unsafe extern "system" {
    pub fn GetModuleHandleW(name: *const u16) -> HINSTANCE;
    pub fn CreateProcessW(
        app: *const u16,
        cmd: *mut u16,
        pa: *mut c_void,
        ta: *mut c_void,
        inherit: BOOL,
        flags: u32,
        env: *mut c_void,
        cwd: *const u16,
        si: *mut STARTUPINFOW,
        pi: *mut PROCESS_INFORMATION,
    ) -> BOOL;
    pub fn OpenProcess(access: u32, inherit: BOOL, pid: u32) -> HANDLE;
    pub fn ReadProcessMemory(p: HANDLE, addr: *const c_void, buf: *mut c_void, n: usize, read: *mut usize) -> BOOL;
    pub fn WriteProcessMemory(p: HANDLE, addr: *mut c_void, buf: *const c_void, n: usize, written: *mut usize) -> BOOL;
    pub fn VirtualProtectEx(p: HANDLE, addr: *mut c_void, n: usize, prot: u32, old: *mut u32) -> BOOL;
    pub fn FlushInstructionCache(p: HANDLE, addr: *const c_void, n: usize) -> BOOL;
    pub fn ResumeThread(t: HANDLE) -> u32;
    pub fn TerminateProcess(p: HANDLE, code: u32) -> BOOL;
    pub fn GetExitCodeProcess(p: HANDLE, code: *mut u32) -> BOOL;
    pub fn CloseHandle(h: HANDLE) -> BOOL;
    pub fn GetLastError() -> u32;
    pub fn CopyFileW(from: *const u16, to: *const u16, fail_if_exists: BOOL) -> BOOL;
    pub fn VirtualAllocEx(p: HANDLE, addr: *mut c_void, n: usize, ty: u32, prot: u32) -> *mut c_void;
    pub fn VirtualFreeEx(p: HANDLE, addr: *mut c_void, n: usize, ty: u32) -> BOOL;
    pub fn CreateRemoteThread(p: HANDLE, sa: *mut c_void, stack: usize, start: *mut c_void, param: *mut c_void, flags: u32, tid: *mut u32) -> HANDLE;
    pub fn WaitForSingleObject(h: HANDLE, ms: u32) -> u32;
    pub fn IsWow64Process(p: HANDLE, wow: *mut BOOL) -> BOOL;
}

/// SHBrowseForFolderW's description of the dialog.
#[repr(C)]
pub struct BROWSEINFOW {
    pub hwndOwner: HWND,
    pub pidlRoot: *const c_void,
    pub pszDisplayName: *mut u16,
    pub lpszTitle: *const u16,
    pub ulFlags: u32,
    pub lpfn: Option<unsafe extern "system" fn(HWND, u32, LPARAM, LPARAM) -> i32>,
    pub lParam: LPARAM,
    pub iImage: i32,
}
pub const BIF_RETURNONLYFSDIRS: u32 = 0x1;
pub const BIF_EDITBOX: u32 = 0x10;
pub const BIF_NEWDIALOGSTYLE: u32 = 0x40;
pub const BFFM_INITIALIZED: u32 = 1;
pub const BFFM_SETSELECTIONW: u32 = 0x400 + 103;

#[link(name = "shell32")]
unsafe extern "system" {
    pub fn SHBrowseForFolderW(bi: *const BROWSEINFOW) -> *mut c_void;
    pub fn SHGetPathFromIDListW(pidl: *const c_void, path: *mut u16) -> BOOL;
}

#[link(name = "ole32")]
unsafe extern "system" {
    pub fn CoTaskMemFree(p: *mut c_void);
}

unsafe extern "system" fn browse_start(h: HWND, msg: u32, _l: LPARAM, data: LPARAM) -> i32 {
    if msg == BFFM_INITIALIZED && data != 0 {
        unsafe { SendMessageW(h, BFFM_SETSELECTIONW, 1, data) };
    }
    0
}

/// The folder picker (modal on `owner`), starting at `start`; None if cancelled.
pub fn pick_folder(owner: HWND, title: &str, start: &str) -> Option<String> {
    let title = wide(title);
    let start = wide(start);
    let mut name = [0u16; 260];
    let bi = BROWSEINFOW {
        hwndOwner: owner,
        pidlRoot: std::ptr::null(),
        pszDisplayName: name.as_mut_ptr(),
        lpszTitle: title.as_ptr(),
        ulFlags: BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_EDITBOX | 0x200, // (no "new folder" button)
        lpfn: Some(browse_start),
        lParam: if start.len() > 1 { start.as_ptr() as LPARAM } else { 0 },
        iImage: 0,
    };
    let pidl = unsafe { SHBrowseForFolderW(&bi) };
    if pidl.is_null() {
        return None;
    }
    let mut path = [0u16; 520];
    let ok = unsafe { SHGetPathFromIDListW(pidl, path.as_mut_ptr()) } != 0;
    unsafe { CoTaskMemFree(pidl) };
    let n = path.iter().position(|&c| c == 0).unwrap_or(path.len());
    (ok && n > 0).then(|| String::from_utf16_lossy(&path[..n]))
}

#[link(name = "user32")]
unsafe extern "system" {
    pub fn RegisterClassExW(wc: *const WNDCLASSEXW) -> u16;
    pub fn CreateWindowExW(
        ex: u32,
        class: *const u16,
        title: *const u16,
        style: u32,
        x: i32,
        y: i32,
        w: i32,
        h: i32,
        parent: HWND,
        menu: HMENU,
        inst: HINSTANCE,
        param: *mut c_void,
    ) -> HWND;
    pub fn DefWindowProcW(h: HWND, m: u32, w: WPARAM, l: LPARAM) -> LRESULT;
    pub fn GetMessageW(msg: *mut MSG, h: HWND, min: u32, max: u32) -> BOOL;
    pub fn TranslateMessage(msg: *const MSG) -> BOOL;
    pub fn DispatchMessageW(msg: *const MSG) -> LRESULT;
    pub fn IsDialogMessageW(h: HWND, msg: *mut MSG) -> BOOL;
    pub fn PostQuitMessage(code: i32);
    pub fn SendMessageW(h: HWND, m: u32, w: WPARAM, l: LPARAM) -> LRESULT;
    pub fn SetWindowTextW(h: HWND, s: *const u16) -> BOOL;
    pub fn GetWindowTextW(h: HWND, s: *mut u16, n: i32) -> i32;
    pub fn SetTimer(h: HWND, id: usize, ms: u32, f: *const c_void) -> usize;
    pub fn KillTimer(h: HWND, id: usize) -> BOOL;
    pub fn FindWindowW(class: *const u16, title: *const u16) -> HWND;
    pub fn GetWindowThreadProcessId(h: HWND, pid: *mut u32) -> u32;
    pub fn GetClientRect(h: HWND, r: *mut RECT) -> BOOL;
    pub fn GetWindowRect(h: HWND, r: *mut RECT) -> BOOL;
    pub fn ClientToScreen(h: HWND, p: *mut POINT) -> BOOL;
    pub fn SetWindowPos(h: HWND, after: HWND, x: i32, y: i32, w: i32, hh: i32, flags: u32) -> BOOL;
    pub fn GetWindowLongW(h: HWND, i: i32) -> i32;
    pub fn SetWindowLongW(h: HWND, i: i32, v: i32) -> i32;
    pub fn AdjustWindowRectExForDpi(r: *mut RECT, style: u32, menu: BOOL, ex: u32, dpi: u32) -> BOOL;
    pub fn SetLayeredWindowAttributes(h: HWND, key: u32, alpha: u8, flags: u32) -> BOOL;
    pub fn ShowWindow(h: HWND, cmd: i32) -> BOOL;
    pub fn IsWindow(h: HWND) -> BOOL;
    pub fn IsWindowVisible(h: HWND) -> BOOL;
    pub fn IsIconic(h: HWND) -> BOOL;
    pub fn IsZoomed(h: HWND) -> BOOL;
    pub fn IsHungAppWindow(h: HWND) -> BOOL;
    pub fn GetDC(h: HWND) -> HDC;
    pub fn ReleaseDC(h: HWND, dc: HDC) -> i32;
    pub fn BeginPaint(h: HWND, ps: *mut PAINTSTRUCT) -> HDC;
    pub fn EndPaint(h: HWND, ps: *const PAINTSTRUCT) -> BOOL;
    pub fn FillRect(dc: HDC, r: *const RECT, brush: HGDIOBJ) -> i32;
    pub fn FrameRect(dc: HDC, r: *const RECT, brush: HGDIOBJ) -> i32;
    pub fn LoadCursorW(inst: HINSTANCE, name: usize) -> HANDLE;
    pub fn MessageBoxW(h: HWND, text: *const u16, caption: *const u16, t: u32) -> i32;
    pub fn SetProcessDpiAwarenessContext(ctx: HANDLE) -> BOOL;
    pub fn GetDpiForWindow(h: HWND) -> u32;
    pub fn GetForegroundWindow() -> HWND;
    pub fn EnableWindow(h: HWND, enable: BOOL) -> BOOL;
    pub fn SystemParametersInfoW(action: u32, param: u32, pv: *mut c_void, ini: u32) -> BOOL;
    pub fn SystemParametersInfoForDpi(action: u32, param: u32, pv: *mut c_void, ini: u32, dpi: u32) -> BOOL;
    pub fn GetDlgItem(h: HWND, id: i32) -> HWND;
    pub fn GetWindow(h: HWND, cmd: u32) -> HWND;
    pub fn ClipCursor(r: *const RECT) -> BOOL;
    pub fn GetCursorPos(p: *mut POINT) -> BOOL;
}

#[link(name = "gdi32")]
unsafe extern "system" {
    pub fn CreateCompatibleDC(dc: HDC) -> HDC;
    pub fn CreateCompatibleBitmap(dc: HDC, w: i32, h: i32) -> HGDIOBJ;
    pub fn SelectObject(dc: HDC, o: HGDIOBJ) -> HGDIOBJ;
    pub fn DeleteObject(o: HGDIOBJ) -> BOOL;
    pub fn DeleteDC(dc: HDC) -> BOOL;
    pub fn BitBlt(dst: HDC, x: i32, y: i32, w: i32, h: i32, src: HDC, sx: i32, sy: i32, rop: u32) -> BOOL;
    pub fn CreatePen(style: i32, width: i32, color: u32) -> HGDIOBJ;
    pub fn CreateSolidBrush(color: u32) -> HGDIOBJ;
    pub fn GetStockObject(i: i32) -> HGDIOBJ;
    pub fn Rectangle(dc: HDC, l: i32, t: i32, r: i32, b: i32) -> BOOL;
    pub fn MoveToEx(dc: HDC, x: i32, y: i32, old: *mut POINT) -> BOOL;
    pub fn LineTo(dc: HDC, x: i32, y: i32) -> BOOL;
    pub fn SetBkMode(dc: HDC, mode: i32) -> i32;
    pub fn SetTextColor(dc: HDC, c: u32) -> u32;
    pub fn SetBkColor(dc: HDC, c: u32) -> u32;
    pub fn TextOutW(dc: HDC, x: i32, y: i32, s: *const u16, n: i32) -> BOOL;
    pub fn CreateFontW(
        h: i32,
        w: i32,
        esc: i32,
        orient: i32,
        weight: i32,
        italic: u32,
        underline: u32,
        strike: u32,
        charset: u32,
        out_prec: u32,
        clip_prec: u32,
        quality: u32,
        pitch: u32,
        face: *const u16,
    ) -> HGDIOBJ;
}

#[link(name = "comctl32")]
unsafe extern "system" {
    pub fn InitCommonControlsEx(icc: *const INITCOMMONCONTROLSEX) -> BOOL;
}

#[link(name = "advapi32")]
unsafe extern "system" {
    pub fn RegCreateKeyExW(
        key: HKEY,
        sub: *const u16,
        reserved: u32,
        class: *mut u16,
        options: u32,
        sam: u32,
        sa: *mut c_void,
        out: *mut HKEY,
        disp: *mut u32,
    ) -> i32;
    pub fn RegSetValueExW(key: HKEY, name: *const u16, reserved: u32, ty: u32, data: *const u8, n: u32) -> i32;
    pub fn RegQueryValueExW(key: HKEY, name: *const u16, reserved: *mut u32, ty: *mut u32, data: *mut u8, n: *mut u32) -> i32;
    pub fn RegCloseKey(key: HKEY) -> i32;
}

/// UTF-16, NUL-terminated.
pub fn wide(s: &str) -> Vec<u16> {
    s.encode_utf16().chain(std::iter::once(0)).collect()
}

pub const fn rgb(r: u8, g: u8, b: u8) -> u32 {
    r as u32 | (g as u32) << 8 | (b as u32) << 16
}

// --- audio (winmm) and input ------------------------------------------------

#[repr(C)]
pub struct WAVEFORMATEX {
    pub wFormatTag: u16,
    pub nChannels: u16,
    pub nSamplesPerSec: u32,
    pub nAvgBytesPerSec: u32,
    pub nBlockAlign: u16,
    pub wBitsPerSample: u16,
    pub cbSize: u16,
}

#[repr(C)]
pub struct WAVEHDR {
    pub lpData: *mut u8,
    pub dwBufferLength: u32,
    pub dwBytesRecorded: u32,
    pub dwUser: usize,
    pub dwFlags: u32,
    pub dwLoops: u32,
    pub lpNext: *mut WAVEHDR,
    pub reserved: usize,
}

#[repr(C)]
pub struct WAVEINCAPSW {
    pub wMid: u16,
    pub wPid: u16,
    pub vDriverVersion: u32,
    pub szPname: [u16; 32],
    pub dwFormats: u32,
    pub wChannels: u16,
    pub wReserved1: u16,
}

pub const WAVE_MAPPER: u32 = u32::MAX;
pub const CALLBACK_NULL: u32 = 0;
pub const CALLBACK_EVENT: u32 = 0x0005_0000;
pub const WHDR_DONE: u32 = 1;

#[link(name = "winmm")]
unsafe extern "system" {
    pub fn waveInGetNumDevs() -> u32;
    pub fn waveInGetDevCapsW(dev: usize, caps: *mut WAVEINCAPSW, size: u32) -> u32;
    pub fn waveInOpen(h: *mut HANDLE, dev: u32, fmt: *const WAVEFORMATEX, cb: usize, inst: usize, flags: u32) -> u32;
    pub fn waveInPrepareHeader(h: HANDLE, hdr: *mut WAVEHDR, size: u32) -> u32;
    pub fn waveInUnprepareHeader(h: HANDLE, hdr: *mut WAVEHDR, size: u32) -> u32;
    pub fn waveInAddBuffer(h: HANDLE, hdr: *mut WAVEHDR, size: u32) -> u32;
    pub fn waveInStart(h: HANDLE) -> u32;
    pub fn waveInStop(h: HANDLE) -> u32;
    pub fn waveInReset(h: HANDLE) -> u32;
    pub fn waveInClose(h: HANDLE) -> u32;
    pub fn waveOutOpen(h: *mut HANDLE, dev: u32, fmt: *const WAVEFORMATEX, cb: usize, inst: usize, flags: u32) -> u32;
    pub fn waveOutPrepareHeader(h: HANDLE, hdr: *mut WAVEHDR, size: u32) -> u32;
    pub fn waveOutUnprepareHeader(h: HANDLE, hdr: *mut WAVEHDR, size: u32) -> u32;
    pub fn waveOutWrite(h: HANDLE, hdr: *mut WAVEHDR, size: u32) -> u32;
    pub fn waveOutReset(h: HANDLE) -> u32;
    pub fn waveOutClose(h: HANDLE) -> u32;
}

#[link(name = "kernel32")]
unsafe extern "system" {
    pub fn CreateEventW(sa: *mut c_void, manual: BOOL, initial: BOOL, name: *const u16) -> HANDLE;
    pub fn SetEvent(h: HANDLE) -> BOOL;
    pub fn WaitForMultipleObjects(count: u32, handles: *const HANDLE, all: BOOL, ms: u32) -> u32;
}
pub const WAIT_OBJECT_0: u32 = 0;

#[link(name = "user32")]
unsafe extern "system" {
    pub fn GetAsyncKeyState(vk: i32) -> i16;
}
