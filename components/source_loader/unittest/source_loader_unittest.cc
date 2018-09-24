//
// Created by gh on 18-9-16.
//
#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <atomic>
#include "components/source_loader/source_impl/name_struct_source.h"
#include "../loader/file_loader.h"

#define CATCH_CONFIG_MAIN //only once
#include <catch/catch.hpp>


class Person {
public:
	std::string name;
	int32_t     age;
	component::sl::Json        ext;
};
void to_json(component::sl::Json& j, const Person& person) {
	j["name"] = person.name;
	j["age"] = person.age;
	j["info"] = person.ext;
}
void from_json(const component::sl::Json& j, Person& persion) {
	persion.name = j["name"];
	persion.age = j["age"];
	persion.ext = j["info"];
}

TEST_CASE("json_test", "[test josn lib]") {
	component::sl::Json empty_j;
	std::cout << "empty_j type:" << empty_j.type_name() << std::endl;

	try {
		component::sl::Json j = empty_j.at("aaaa");
		std::cout << j.empty() << std::endl;
	} catch(...) {
		std::cout << "can't get aaaa" << std::endl;
	}

	component::sl::Json bad_json = component::sl::Json::parse("a{a , bc}", nullptr, false);
	std::cout << bad_json.type_name() << " " << bad_json.is_discarded() << std::endl;
}

TEST_CASE("filereader", "[test file loader]") {
	std::ifstream i("./config_test.json");
	if (!i.is_open()) {
		std::cout << "not open file" << std::endl;
		return;
	}
	class DummyDelegate : public component::sl::LoaderDelegate {
	public:
		void OnFinish(int code) {
			std::cout << "finish load with code:" << code << std::endl;
		}
		void OnReadData(const std::string& data) {
			std::cout << "got data:" << data << std::endl;
		}
	};
	DummyDelegate delegate;

	component::sl::Json	config = component::sl::Json::parse(i);

	component::sl::Json reader_config = config["source"]["loader"];
	std::cout << std::setw(4) << reader_config << std::endl;

	component::sl::FileLoader fileReader(&delegate, reader_config);
	fileReader.Initialize();
}

TEST_CASE("name_struct", "[test namestruct source]") {
//	std::ifstream i("./config_test.json");
//	if (!i.is_open()) {
//		std::cout << "not open file" << std::endl;
//		return;
//	}
	component::sl::Json config = R"({
  "name": "source_person_info",
  "namespace": "source",
  "source": {
    "loader": {
      "type": "file",
      "resources": [{"path": "./data_file_test.txt"}]
    },
    "parser": {
      "type": "column",
      "delimiter":"#",
      "primary": "name",
      "content_mode": "line",
      "ignore_header_line":false,
      "header": ["name","age","info","ext"],
      "schemes": [
        {
          "name": "name",
          "type": "string",
          "default": "",
          "delimiter": "",
          "allow_null": false
        },
        {
          "name": "age",
          "type": "int",
          "default": 5,
          "delimiter": "",
          "allow_null": true
        },
        {
          "name": "info",
          "type": "json",
          "default": {},
          "delimiter": "",
          "allow_null": true
        }
      ]
    },
    "reloader": {
      "mode": "interval",
      "interval": 3000
    }
  }
	})"_json;
	component::sl::NameStructSource<Person> source(config["source"]);
	REQUIRE(source.Initialize());

	source.StartLoad();
	const Person& p = source.GetByName("gonghuan");
	std::cout << "name:" << p.name << " age:" << p.age << " info:" << p.ext << std::endl;
}
