#include "support/approx.h"
#include "utils/yaml.h"

#include <doctest/doctest.h>

#include <cstdint>
#include <string>

namespace
{
	// Parses one YAML document from text and returns its root node.
	yaml::node Parse(const std::string& text)
	{
		return fkyaml::node::deserialize(text);
	}
} // namespace

TEST_SUITE("yaml")
{
	// ---------------------------------- strings ----------------------------------

	TEST_CASE("a string value converts to text in plain, double-quoted and single-quoted form")
	{
		const auto root = Parse("plain: Target Boundary\n"
								"double: \"Target Boundary\"\n"
								"single: 'Target Boundary'\n");

		for (const std::string key : {"plain", "double", "single"})
		{
			CAPTURE(key);
			std::string text;
			CHECK(yaml::NodeToString(root[key], text));
			CHECK(text == "Target Boundary");
		}
	}

	TEST_CASE("a value that is not a string is rejected as text")
	{
		auto root = Parse("integer: 42\n"
						  "floating: 1.5\n"
						  "boolean: true\n"
						  "empty: ~\n"
						  "list: [a, b]\n"
						  "map: {a: b}\n");

		for (const std::string key : {"integer", "floating", "boolean", "empty", "list", "map"})
		{
			CAPTURE(key);
			std::string text;
			CHECK_FALSE(yaml::NodeToString(root[key], text));
		}
	}

	// ---------------------------------- integers ----------------------------------

	TEST_CASE("an integer converts in decimal, signed, hexadecimal and octal notation")
	{
		// YAML 1.2 core schema integer forms.
		const auto root = Parse("decimal: 42\n"
								"plus: +42\n"
								"minus: -42\n"
								"hex: 0x2A\n"
								"octal: 0o52\n");

		int value = 0;
		CHECK(yaml::NodeToInt(root["decimal"], value));
		CHECK(value == 42);
		CHECK(yaml::NodeToInt(root["plus"], value));
		CHECK(value == 42);
		CHECK(yaml::NodeToInt(root["minus"], value));
		CHECK(value == -42);
		CHECK(yaml::NodeToInt(root["hex"], value));
		CHECK(value == 42);
		CHECK(yaml::NodeToInt(root["octal"], value));
		CHECK(value == 42);
	}

	TEST_CASE("a whole number written as a floating value converts to an integer")
	{
		const auto root = Parse("point: 250.0\n"
								"exponent: 1e3\n"
								"mantissa: 2.5e2\n");

		size_t value = 0;
		CHECK(yaml::NodeToInt(root["point"], value));
		CHECK(value == 250);
		CHECK(yaml::NodeToInt(root["exponent"], value));
		CHECK(value == 1000);
		CHECK(yaml::NodeToInt(root["mantissa"], value));
		CHECK(value == 250);
	}

	TEST_CASE("an integer written as a quoted string converts to an integer")
	{
		const auto root = Parse("count: \"40000\"\n");

		uint32_t value = 0;
		CHECK(yaml::NodeToInt(root["count"], value));
		CHECK(value == 40000);
	}

	TEST_CASE("a value that is not a number is rejected as an integer")
	{
		auto root = Parse("boolean: true\n"
						  "word: abc\n"
						  "empty: ~\n"
						  "list: [1, 2]\n"
						  "map: {a: 1}\n");

		for (const std::string key : {"boolean", "word", "empty", "list", "map"})
		{
			CAPTURE(key);
			int value = 0;
			CHECK_FALSE(yaml::NodeToInt(root[key], value));
		}
	}

	TEST_CASE("a value the integer type cannot hold is rejected")
	{
		// The counts in the config (numtracks, numsteps, iterations, saveInterval) are unsigned.
		auto root = Parse("negative: -1\n"
						  "negativeFloating: -1.0\n"
						  "negativeString: \"-1\"\n"
						  "tooLarge: 5000000000\n"
						  "tooLargeFloating: 5e9\n");

		for (const std::string key : {"negative", "negativeFloating", "negativeString", "tooLarge", "tooLargeFloating"})
		{
			CAPTURE(key);
			uint32_t value = 0;
			CHECK_FALSE(yaml::NodeToInt(root[key], value));
		}
	}

	// ---------------------------------- doubles ----------------------------------

	TEST_CASE("a floating value converts in decimal and exponent notation")
	{
		const auto root = Parse("decimal: 0.0005\n"
								"exponent: 1e-4\n"
								"negativeExponent: -100e-9\n"
								"positiveExponent: 10e3\n"
								"upperCase: 1.5E+3\n"
								"leadingPoint: .5\n");

		double value = 0.0;
		CHECK(yaml::NodeToDouble(root["decimal"], value));
		CHECK_NEAR(value, 0.0005);
		CHECK(yaml::NodeToDouble(root["exponent"], value));
		CHECK_NEAR(value, 1e-4);
		CHECK(yaml::NodeToDouble(root["negativeExponent"], value));
		CHECK_NEAR(value, -1e-7);
		CHECK(yaml::NodeToDouble(root["positiveExponent"], value));
		CHECK_NEAR(value, 1e4);
		CHECK(yaml::NodeToDouble(root["upperCase"], value));
		CHECK_NEAR(value, 1500.0);
		CHECK(yaml::NodeToDouble(root["leadingPoint"], value));
		CHECK_NEAR(value, 0.5);
	}

	TEST_CASE("an integer converts to a floating value")
	{
		const auto root = Parse("positive: 1000\n"
								"negative: -3\n"
								"zero: 0\n");

		double value = 1.0;
		CHECK(yaml::NodeToDouble(root["positive"], value));
		CHECK_NEAR(value, 1000.0);
		CHECK(yaml::NodeToDouble(root["negative"], value));
		CHECK_NEAR(value, -3.0);
		CHECK(yaml::NodeToDouble(root["zero"], value));
		CHECK_NEAR_ABS(value, 0.0, 0.0);
	}

	TEST_CASE("a number written as a quoted string converts to a floating value")
	{
		const auto root = Parse("current: \"-100e-9\"\n");

		double value = 0.0;
		CHECK(yaml::NodeToDouble(root["current"], value));
		CHECK_NEAR(value, -1e-7);
	}

	TEST_CASE("a value that is not a number is rejected as a floating value")
	{
		auto root = Parse("boolean: false\n"
						  "word: abc\n"
						  "empty: ~\n"
						  "list: [1.0, 2.0]\n"
						  "map: {a: 1.0}\n");

		for (const std::string key : {"boolean", "word", "empty", "list", "map"})
		{
			CAPTURE(key);
			double value = 0.0;
			CHECK_FALSE(yaml::NodeToDouble(root[key], value));
		}
	}

	// ---------------------------------- booleans ----------------------------------

	TEST_CASE("a boolean converts in every core schema spelling")
	{
		// YAML 1.2 core schema: true|True|TRUE and false|False|FALSE.
		const auto root = Parse("t1: true\nt2: True\nt3: TRUE\n"
								"f1: false\nf2: False\nf3: FALSE\n");

		for (const std::string key : {"t1", "t2", "t3"})
		{
			CAPTURE(key);
			bool value = false;
			CHECK(yaml::NodeToBool(root[key], value));
			CHECK(value == true);
		}
		for (const std::string key : {"f1", "f2", "f3"})
		{
			CAPTURE(key);
			bool value = true;
			CHECK(yaml::NodeToBool(root[key], value));
			CHECK(value == false);
		}
	}

	TEST_CASE("a number converts to a boolean that is true when the number is not zero")
	{
		const auto root = Parse("one: 1\nzero: 0\nhalf: 0.5\nzeroFloating: 0.0\n");

		bool value = false;
		CHECK(yaml::NodeToBool(root["one"], value));
		CHECK(value == true);
		CHECK(yaml::NodeToBool(root["zero"], value));
		CHECK(value == false);
		CHECK(yaml::NodeToBool(root["half"], value));
		CHECK(value == true);
		CHECK(yaml::NodeToBool(root["zeroFloating"], value));
		CHECK(value == false);
	}

	TEST_CASE("a boolean written as a quoted string converts to a boolean")
	{
		const auto root = Parse("quotedTrue: \"true\"\nquotedFalse: \"false\"\n");

		bool value = false;
		CHECK(yaml::NodeToBool(root["quotedTrue"], value));
		CHECK(value == true);
		CHECK(yaml::NodeToBool(root["quotedFalse"], value));
		CHECK(value == false);
	}

	TEST_CASE("a collection or an empty value is rejected as a boolean")
	{
		auto root = Parse("empty: ~\n"
						  "list: [true]\n"
						  "map: {a: true}\n");

		for (const std::string key : {"empty", "list", "map"})
		{
			CAPTURE(key);
			bool value = false;
			CHECK_FALSE(yaml::NodeToBool(root[key], value));
		}
	}

	TEST_CASE("a string that does not spell a boolean is rejected as a boolean")
	{
		// "yes" is a boolean in YAML 1.1 but a plain string in YAML 1.2.
		auto root = Parse("word: yes\n"
						  "quoted: \"maybe\"\n");

		for (const std::string key : {"word", "quoted"})
		{
			CAPTURE(key);
			bool value = false;
			CHECK_FALSE(yaml::NodeToBool(root[key], value));
		}
	}

	// ---------------------------------- missing keys ----------------------------------

	TEST_CASE("a missing key is rejected by every conversion")
	{
		auto root = Parse("present: 1\n");

		std::string text;
		int integer = 0;
		double floating = 0.0;
		bool boolean = false;
		CHECK_FALSE(yaml::NodeToString(root["missing"], text));
		CHECK_FALSE(yaml::NodeToInt(root["missing"], integer));
		CHECK_FALSE(yaml::NodeToDouble(root["missing"], floating));
		CHECK_FALSE(yaml::NodeToBool(root["missing"], boolean));
	}
}
