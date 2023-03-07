#pragma once

#include "common.hpp"

#include <array>
#include <mutex>
#include <string>
#include <vector>

namespace app
{

	class worker_thread
	{
	private:
		HWND window_;
		HANDLE thread_;
		bool enabled_;
		std::mutex mtx_;
		std::mutex cfg_mtx_;
		std::mutex stats_mtx_;
		HANDLE event_close_;
		HANDLE event_restore_;
		UINT64 stats_total_skip_;
		UINT64 stats_total_duplicate_;

		static DWORD WINAPI proc_common(LPVOID);
		DWORD proc();
	public:
		worker_thread();
		~worker_thread();

		// コピー不可
		worker_thread(const worker_thread&) = delete;
		worker_thread& operator = (const worker_thread&) = delete;
		// ムーブ不可
		worker_thread(worker_thread&&) = delete;
		worker_thread& operator = (worker_thread&&) = delete;

		bool run(HWND, bool);
		void stop();

		void restore();
		void enable();
		void disable();
	};
}
