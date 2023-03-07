#pragma once

#include "common.hpp"

#include "worker_thread.hpp"
#include "config_ini.hpp"

#include <shellapi.h>

namespace app
{
	class main_window
	{
	private:
		const HINSTANCE instance_;
		HWND window_;
		HWND button_;
		HWND check_backup_;
		HWND check_key_;
		HFONT font_;
		worker_thread worker_thread_;
		config_ini ini_;
		bool backup_;

		static const wchar_t* window_class_;
		static const wchar_t* window_title_;
		static const wchar_t* window_mutex_;
		static const LONG window_width_;
		static const LONG window_height_;

		void disable_ime();
		void set_dpi_awareness();
		ATOM register_window_class();
		bool create_window();

		LRESULT window_proc(UINT, WPARAM, LPARAM);
		static LRESULT CALLBACK window_proc_common(HWND, UINT, WPARAM, LPARAM);

		bool get_key_checked();
		bool get_backup_checked();

	public:
		main_window(HINSTANCE);
		~main_window();

		// コピー不可
		main_window(const main_window&) = delete;
		main_window& operator = (const main_window&) = delete;
		// ムーブ不可
		main_window(main_window&&) = delete;
		main_window& operator = (main_window&&) = delete;

		bool init();
		int  loop();
	};
}
