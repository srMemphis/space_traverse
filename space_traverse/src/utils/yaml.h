#pragma once
#include "debug/Log.h"
#include "fkYAML/node.hpp"

#include <cassert>
#include <fstream>
#include <limits>
#include <sstream>
#include <streambuf>
#include <string_view>
#include <type_traits>
#include <utility>

namespace yaml
{
	using node = fkyaml::node;

	std::pair<node, bool> FromFile(std::string_view filename)
	{
		std::fstream file;
		file.open(filename.data());

		if (!file.is_open())
		{
			return {node(), false};
		}

		std::string str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

		auto res = fkyaml::node::deserialize(str);

		return {res, true};
	}

	bool NodeToString(const fkyaml::basic_node<>& node, std::string& str)
	{
		if (node.get_type() == fkyaml::node_type::STRING)
		{
			str = node.as_str();
			return true;
		}

		return false;
	}

	template<typename T>
	bool NodeToInt(const fkyaml::basic_node<>& node, T& integer)
	{
		static_assert(std::is_integral_v<T>, "Wrong type in template. Expected integer");

		switch (node.get_type())
		{
		case fkyaml::node_type::INTEGER:
		{
			const auto value = node.as_int();
			if (!std::in_range<T>(value))
				return false;

			integer = static_cast<T>(value);
			return true;
		}
		case fkyaml::node_type::FLOAT:
		{
			// The value truncated toward zero must fit in T. NaN fails both comparisons.
			const double value = node.as_float();
			if (!(value > static_cast<double>(std::numeric_limits<T>::min()) - 1.0 &&
				  value < static_cast<double>(std::numeric_limits<T>::max()) + 1.0))
				return false;

			integer = static_cast<T>(value);
			return true;
		}
		case fkyaml::node_type::STRING:
		{
			int value = 0;
			try
			{
				value = std::stoi(node.as_str());
			}
			catch (...)
			{
				LOG_CRITICAL("String to int failed. The string was: {}", node.as_str());
				return false;
			}

			if (!std::in_range<T>(value))
				return false;

			integer = static_cast<T>(value);
			return true;
		}
		default:
			return false;
		}
	}

	template<typename T>
	bool NodeToDouble(const fkyaml::basic_node<>& node, T& floating)
	{
		static_assert(std::is_floating_point_v<T>, "Wrong type in template. Expected floating");

		switch (node.get_type())
		{
		case fkyaml::node_type::INTEGER:
		{
			floating = node.as_int();
			return true;
		}
		case fkyaml::node_type::FLOAT:
		{
			floating = node.as_float();
			return true;
		}
		case fkyaml::node_type::STRING:
		{
			try
			{
				floating = std::stod(node.as_str());
			}
			catch (...)
			{
				LOG_CRITICAL("String to double failed. The string was: {}", node.as_str());
				return false;
			}

			return true;
		}
		default:
			return false;
		}
	}

	bool NodeToBool(const fkyaml::basic_node<>& node, bool& boolean)
	{
		switch (node.get_type())
		{
		case fkyaml::node_type::BOOLEAN:
		{
			boolean = node.as_bool();
			return true;
		}
		case fkyaml::node_type::INTEGER:
		{
			boolean = node.as_int();
			return true;
		}
		case fkyaml::node_type::FLOAT:
		{
			boolean = node.as_float();
			return true;
		}
		case fkyaml::node_type::STRING:
		{
			std::istringstream is(node.as_str());
			is >> std::boolalpha >> boolean;
			return !is.fail();
		}
		default:
			return false;
		}
	}

} // namespace yaml
