#pragma once

#include "common.hpp"

#include <string>

namespace app
{

	class config_ini
	{
	private:
		std::wstring path_;

		bool set_value(const std::wstring& _key, const std::wstring& _value);
		std::wstring get_value(const std::wstring& _key);
		UINT get_intvalue(const std::wstring& _key);

	public:
		config_ini();
		~config_ini();

		bool set_enable_backup(bool);
		bool get_enable_backup();

		bool set_enable_key(bool);
		bool get_enable_key();
	};
}
