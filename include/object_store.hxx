#pragma once

#include <string>
#include <memory>
#include <nlohmann/json.hpp>

#include "object_collection.hxx"

class object_store {
public:
    object_store(std::string const& filename);

    virtual ~object_store();

    std::shared_ptr<object_collection>
    collection(std::string const& name);

protected:
    std::shared_ptr<sqlite::database> db_;
    std::unordered_map<std::string, std::shared_ptr<object_collection>> collections_;
};
