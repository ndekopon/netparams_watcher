#include "main_window.hpp"

#include <imm.h>

#pragma comment(lib, "imm32.lib")

namespace app
{
	constexpr UINT MID_RESTORE = 1;

	const wchar_t* main_window::window_class_ = L"netparams_watcher-mainwindow";
	const wchar_t* main_window::window_title_ = L"netparams_watcher";
	const wchar_t* main_window::window_mutex_ = L"netparams_watcher_mutex";
	const LONG main_window::window_width_ = 320;
	const LONG main_window::window_height_ = 160;


	main_window::main_window(HINSTANCE _instance)
		: instance_(_instance)
		, window_(nullptr)
		, button_(nullptr)
		, font_(nullptr)
		, worker_thread_()
		, backup_(false)
	{
	}

	main_window::~main_window()
	{
	}

	bool main_window::init()
	{
		HANDLE mutex = ::CreateMutexW(NULL, TRUE, window_mutex_);
		if (::GetLastError() == ERROR_ALREADY_EXISTS)
		{
			return false;
		}

		disable_ime();

		set_dpi_awareness();

		// create window
		register_window_class();
		if (!create_window())
			return false;

		return true;
	}


	int main_window::loop()
	{
		MSG message;

		while (::GetMessageW(&message, nullptr, 0, 0))
		{
			::TranslateMessage(&message);
			::DispatchMessageW(&message);
		}
		return (int)message.wParam;
	}

	void main_window::disable_ime()
	{
		::ImmDisableIME(-1);
	}

	void main_window::set_dpi_awareness()
	{
		auto desired_context = DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED;
		if (::IsValidDpiAwarenessContext(desired_context))
		{
			auto hr = ::SetProcessDpiAwarenessContext(desired_context);
			if (hr)
				return;
		}
	}

	ATOM main_window::register_window_class()
	{
		WNDCLASSEXW wcex;

		wcex.cbSize = sizeof(WNDCLASSEXW);

		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = window_proc_common;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = instance_;
		wcex.hIcon = ::LoadIconW(nullptr, IDI_APPLICATION);
		wcex.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
		wcex.hbrBackground = nullptr;
		wcex.lpszMenuName = nullptr;
		wcex.lpszClassName = window_class_;
		wcex.hIconSm = ::LoadIconW(nullptr, IDI_APPLICATION);

		return ::RegisterClassExW(&wcex);
	}

	bool main_window::create_window()
	{
		window_ = ::CreateWindowExW(0, window_class_, window_title_, WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, instance_, this);

		if (window_ == nullptr)
		{
			return false;
		}

		::ShowWindow(window_, SW_NORMAL);
		::UpdateWindow(window_);

		return true;
	}

	LRESULT main_window::window_proc(UINT _message, WPARAM _wparam, LPARAM _lparam)
	{
		switch (_message)
		{
		case WM_CREATE:
			// フォント作成
			font_ = ::CreateFontW(
			18, 0, 0, 0, FW_REGULAR,
				FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
				CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_MODERN,
				L"MS Shell Dlg");

			// ボタン追加
		{
			RECT rect;
			if (::GetClientRect(window_, &rect))
			{
				auto width = rect.right - rect.left;
				auto height = rect.bottom - rect.top;
				button_ = ::CreateWindowExW(
					WS_EX_TOPMOST,
					L"BUTTON", L"RESTORE",
					WS_CHILD | WS_VISIBLE | WS_DISABLED | BS_PUSHBUTTON, 20, 20,
					width - 40, height - 40, window_, (HMENU)(INT_PTR)MID_RESTORE, instance_, NULL);
				::SendMessageW(button_, WM_SETFONT, (WPARAM)font_, MAKELPARAM(true, 0));
			}
		}

			// スレッド開始
			if (!worker_thread_.run(window_)) return -1;

			return 0;

		case WM_DESTROY:
			// スレッド停止
			worker_thread_.stop();

			::DeleteObject(font_);
			::PostQuitMessage(0);
			return 0;

		case WM_COMMAND:
			{
				WORD id = LOWORD(_wparam);
				if (id == MID_RESTORE)
				{
					worker_thread_.restore();
				}
			}
			break;

		case CWM_NETPARAMS_CREATED:
			::EnableWindow(button_, FALSE);
			break;
		case CWM_NETPARAMS_DELETED:
			if (backup_) {
				::EnableWindow(button_, TRUE);
			}
			break;
		case CWM_NETPARAMS_BACKUP_OK:
			backup_ = true;
			break;
		case CWM_NETPARAMS_BACKUP_NG:
			backup_ = false;
			break;
		case CWM_NETPARAMS_RESTORE_OK:
			break;
		case CWM_NETPARAMS_RESTORE_NG:
			::MessageBoxW(window_, L"Error: Restore failed.", window_title_, MB_ICONERROR);
			break;
		default:
			break;
		}

		return ::DefWindowProcW(window_, _message, _wparam, _lparam);
	}

	LRESULT CALLBACK main_window::window_proc_common(HWND _window, UINT _message, WPARAM _wparam, LPARAM _lparam)
	{
		if (_message == WM_NCCREATE)
		{
			// createwindowで指定したポイントからインスタンスを取得
			auto cs = reinterpret_cast<CREATESTRUCTW*>(_lparam);
			auto instance = reinterpret_cast<main_window*>(cs->lpCreateParams);

			instance->window_ = _window;

			// USERDATAにポインタ格納
			::SetWindowLongPtrW(_window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(instance));
		}
		else if(_message == WM_GETMINMAXINFO)
		{
			MINMAXINFO* mminfo = (MINMAXINFO*)_lparam;
			mminfo->ptMaxSize.x = window_width_;
			mminfo->ptMaxSize.y = window_height_;
			mminfo->ptMaxPosition.x = 0;
			mminfo->ptMaxPosition.y = 0;
			mminfo->ptMinTrackSize.x = window_width_;
			mminfo->ptMinTrackSize.y = window_height_;
			mminfo->ptMaxTrackSize.x = window_width_;
			mminfo->ptMaxTrackSize.y = window_height_;
			return 0;
		}

		// 既にデータが格納されていたらインスタンスのプロシージャを呼び出す
		if (auto ptr = reinterpret_cast<main_window*>(::GetWindowLongPtrW(_window, GWLP_USERDATA)))
		{
			return ptr->window_proc(_message, _wparam, _lparam);
		}

		return ::DefWindowProcW(_window, _message, _wparam, _lparam);
	}
}
