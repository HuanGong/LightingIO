//
// Created by gh on 18-9-16.
//
#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <atomic>
#include "../loader/file_loader.h"

#define CATCH_CONFIG_MAIN //only once
#include <catch/catch.hpp>

class DummyDelegate : public component::sl::ReaderDelegate {
public:
	void OnFinish(int code) {
		std::cout << "finish load with code:" << code << std::endl;
	}
	void OnReadData(const std::string& data) {
		std::cout << "got data:" << data << std::endl;
	}
};

TEST_CASE("json_test", "[test josn lib]") {
	std::ifstream i("./config_test.json");
	if (!i.is_open()) {
		std::cout << "not open file" << std::endl;
		return;
	}

	component::sl::Json	config = component::sl::Json::parse(i);
	std::cout << "config[name]" << config["name"] << std::endl;
	component::sl::Json another = config;
	another["name"] = "after_modify";
	std::cout << "anoter[name]" << another["name"] << std::endl;
	std::cout << "config[name]" << config["name"] << std::endl;

	component::sl::Json another2 = std::move(config);
	std::cout << "anoter2[name]" << another["name"] << std::endl;
	std::cout << "config[name]" << config["name"] << std::endl;
}

TEST_CASE("filereader", "[test file loader]") {
	std::ifstream i("./config_test.json");
	if (!i.is_open()) {
		std::cout << "not open file" << std::endl;
		return;
	}
	DummyDelegate delegate;

	component::sl::Json	config = component::sl::Json::parse(i);

	component::sl::Json reader_config = config["source"]["loader"];
	std::cout << std::setw(4) << reader_config << std::endl;

	component::sl::FileLoader fileReader(&delegate, reader_config);
	fileReader.Initialize();
}

