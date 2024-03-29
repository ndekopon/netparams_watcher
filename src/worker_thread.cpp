﻿#define _CRT_SECURE_NO_WARNINGS

#include "worker_thread.hpp"

#include <chrono>
#include <regex>
#include <set>

#include <shlobj.h>
#include <objbase.h>
#include <shlwapi.h>
#include <Wincrypt.h>

#pragma comment(lib, "Shlwapi.lib")

namespace {

	std::wstring get_savedgame_path()
	{
		std::wstring r = L"";
		PWSTR path = nullptr;
		auto rc = ::SHGetKnownFolderPath(FOLDERID_SavedGames, KF_FLAG_CREATE, NULL, &path);
		if (rc == S_OK)
		{
			r = path;
			r = std::wstring(L"\\\\?\\") + path;
		}
		
		if (path != nullptr)
		{
			::CoTaskMemFree(path);
		}

		return r;
	}

	std::wstring get_apex_netparam_dir()
	{
		auto base = get_savedgame_path();
		if (base == L"") return L"";

		base += L"\\Respawn\\Apex\\assets\\temp";
		return base;
	}

	std::wstring get_backup_dir()
	{
		auto base = get_apex_netparam_dir();
		if (base == L"") return L"";

		base += L"\\backup";
		return base;
	}
	std::wstring get_backup_filename()
	{
		auto base = get_backup_dir();
		if (base == L"") return L"";
		
		// 日付・時刻を取得する
		std::time_t t = std::time(nullptr);
		wchar_t timestr[64];
		if (std::wcsftime(timestr, 64, L"%Y%m%d_%H%M%S", std::localtime(&t)))
		{
			base += L"\\netparams_";
			base += timestr;
		}
		return base;
	}

	DWORD read_file(const std::wstring& path, std::vector<uint8_t>& buffer)
	{
		DWORD readed = 0;
		WIN32_FILE_ATTRIBUTE_DATA attr;
		if (::GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &attr))
		{
			auto size = ((int64_t)attr.nFileSizeHigh * MAXDWORD) + attr.nFileSizeLow;
			if (size > 0)
			{
				auto handle = ::CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
				if (handle != INVALID_HANDLE_VALUE)
				{
					if (::ReadFile(handle, buffer.data(), buffer.size(), &readed, NULL))
					{

					}
					::CloseHandle(handle);
				}
			}
		}
		return readed;
	}

	std::wstring md5_to_string(const std::array<BYTE, 16>& md5)
	{
		std::wstring r = L"";
		WCHAR digits[] = L"0123456789abcdef";
		for (size_t i = 0; i < md5.size(); i++)
		{
			r += digits[md5.at(i) >> 4];
			r += digits[md5.at(i) & 0xf];
		}
		return r;
	}

	bool remove_old_backup(const std::wstring &path)
	{
		WIN32_FIND_DATA ffd;
		const std::wregex re(LR"(^netparams_[0-9]{8}_[0-9]{6}$)"); // ファイル名の条件
		std::set<std::wstring> filenames;

		// ディレクトリ一覧を取得
		auto search_path = path + L"\\*";
		auto handle = ::FindFirstFileW(search_path.c_str(), &ffd);
		if (handle == INVALID_HANDLE_VALUE)
		{
			return false;
		}

		do
		{
			if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				std::wstring name = ffd.cFileName;
				if (std::regex_match(name, re))
				{
					filenames.insert(name);
				}
			}
		} while (::FindNextFileW(handle, &ffd) != 0);

		// 10世代以上は削除
		auto size = filenames.size();
		while (size > 10)
		{
			const auto &name = *filenames.begin();
			std::wstring delete_path = path + L"\\" + name;
			::DeleteFileW(delete_path.c_str());
			filenames.erase(filenames.begin());
			size--;
		}

		return true;
	}

	std::array<BYTE, 16> get_md5_hash(const std::vector<uint8_t> &buffer, DWORD len)
	{
		std::array<BYTE, 16> md5 = {};
		HCRYPTPROV cryptprov = 0;
		HCRYPTHASH crypthash = 0;

		// Get handle to the crypto provider
		if (::CryptAcquireContextW(&cryptprov, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
		{
			if (::CryptCreateHash(cryptprov, CALG_MD5, 0, 0, &crypthash))
			{
				if (::CryptHashData(crypthash, buffer.data(), len, 0))
				{
					DWORD hashlen = md5.size();
					if (!::CryptGetHashParam(crypthash, HP_HASHVAL, md5.data(), &hashlen, 0))
					{
						md5.fill(0x00);
					}
				}
				::CryptDestroyHash(crypthash);
			}
			::CryptReleaseContext(cryptprov, 0);
		}
		return md5;
	}
}

namespace app {
	worker_thread::worker_thread()
		: window_(NULL)
		, thread_(NULL)
		, mtx_()
		, event_close_(NULL)
		, event_restore_(NULL)
	{
	}

	worker_thread::~worker_thread()
	{
		stop();
		if (event_close_) ::CloseHandle(event_close_);
		if (event_restore_) ::CloseHandle(event_restore_);
	}

	DWORD WINAPI worker_thread::proc_common(LPVOID _p)
	{
		auto p = reinterpret_cast<worker_thread*>(_p);
		return p->proc();
	}

	DWORD worker_thread::proc()
	{
		DWORD rc = 0;

		auto hr = ::CoInitializeEx(0, COINIT_MULTITHREADED);

		auto watch_path = get_apex_netparam_dir();
		auto netparams_path = watch_path + L"\\netparams";
		auto backup_path = get_backup_dir();
		std::vector<uint8_t> buffer;
		buffer.resize(1024 * 64, 0); // 64kb
		bool enabled = enabled_;

		auto handle = ::FindFirstChangeNotificationW(watch_path.c_str(), FALSE, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE);
		if (handle != INVALID_HANDLE_VALUE)
		{

			bool netparams_exists = false;
			std::array<BYTE, 16> backup_md5 = {};
			std::wstring backup_file = L"";

			// あらかじめバックアップ先のディレクトリを作っておく
			if (!::PathFileExistsW(backup_path.c_str()))
			{
				::CreateDirectoryW(backup_path.c_str(), NULL);
			}

			HANDLE events[] = {
				event_close_,
				event_restore_,
				handle
			};

			while (true)
			{
				auto id = WaitForMultipleObjects(ARRAYSIZE(events), events, FALSE, INFINITE);

				{
					std::lock_guard<std::mutex> lock_(mtx_);
					enabled = enabled_;
				}

				if (id == WAIT_OBJECT_0)
				{
					break;
				}
				else if (id == WAIT_OBJECT_0 + 1)
				{
					// リストア実施
					if (!netparams_exists && backup_file != L"")
					{
						if (::CopyFileW(backup_file.c_str(), netparams_path.c_str(), TRUE))
						{
							::PostMessageW(window_, CWM_NETPARAMS_RESTORE_OK, NULL, NULL);
							continue;
						}
					}
					::PostMessageW(window_, CWM_NETPARAMS_RESTORE_NG, NULL, NULL);
				}
				else if (id == WAIT_OBJECT_0 + 2)
				{
					// ファイル変更通知受信
					if (::FindNextChangeNotification(handle) == FALSE)
					{
						break;
					}

					if (::PathFileExistsW(netparams_path.c_str()) == TRUE)
					{
						if (!netparams_exists)
						{
							::PostMessageW(window_, CWM_NETPARAMS_CREATED, NULL, NULL);
							netparams_exists = true;
						}
						if (enabled)
						{
							auto readed = read_file(netparams_path, buffer);
							if (readed > 0)
							{
								auto md5 = get_md5_hash(buffer, readed);
								if (md5 != std::array<BYTE, 16>({ 0 }) && md5 != backup_md5)
								{
									// バックアップ実施
									backup_md5 = md5;
									backup_file = get_backup_filename();
									if (backup_file != L"" && ::CopyFileW(netparams_path.c_str(), backup_file.c_str(), TRUE))
									{
										// バックアップ完了、古いものを削除
										::PostMessageW(window_, CWM_NETPARAMS_BACKUP_OK, NULL, NULL);
										remove_old_backup(backup_path);
									}
									else
									{
										// バックアップ失敗
										::PostMessageW(window_, CWM_NETPARAMS_BACKUP_NG, NULL, NULL);
										backup_md5.fill(0);
										backup_file = L"";
									}
								}
							}
						}
					}
					else
					{
						if (netparams_exists)
						{
							// ウィンドウにファイルがなくなったことを通知
							::PostMessageW(window_, CWM_NETPARAMS_DELETED, NULL, NULL);
							netparams_exists = false;
						}
					}
				}
			}

			::FindCloseChangeNotification(handle);
		}
		
		::CoUninitialize();

		return rc;
	}

	bool worker_thread::run(HWND _window, bool _enabled)
	{
		window_ = _window;
		enabled_ = _enabled;

		// イベント生成
		if (event_close_ == NULL) event_close_ = ::CreateEventW(NULL, FALSE, FALSE, NULL);
		if (event_close_ == NULL) return false;
		if (event_restore_ == NULL) event_restore_ = ::CreateEventW(NULL, FALSE, FALSE, NULL);
		if (event_restore_ == NULL) return false;

		// スレッド起動
		thread_ = ::CreateThread(NULL, 0, proc_common, this, 0, NULL);
		return thread_ != NULL;
	}

	void worker_thread::stop()
	{
		if (thread_ != NULL)
		{
			::SetEvent(event_close_);
			::WaitForSingleObject(thread_, INFINITE);
			thread_ = NULL;
		}
	}

	void worker_thread::restore()
	{
		if (thread_ != NULL)
		{
			::SetEvent(event_restore_);
		}
	}

	void worker_thread::enable()
	{
		std::lock_guard<std::mutex> lock_(mtx_);
		enabled_ = true;
	}

	void worker_thread::disable()
	{
		std::lock_guard<std::mutex> lock_(mtx_);
		enabled_ = false;
	}
}
