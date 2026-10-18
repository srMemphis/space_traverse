#pragma once

#include <filesystem>
#include <string_view>

// The path of a file in space_traverse/tests/data. premake5.lua defines TEST_DATA_DIR as the absolute path of
// that folder, so the result does not depend on the working directory.
inline std::filesystem::path FixturePath(std::string_view name)
{
	return std::filesystem::path(TEST_DATA_DIR) / name;
}
