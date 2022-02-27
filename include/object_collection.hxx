#pragma once

#include <string>
#include <optional>
#include <memory>
#include <random>

#include <nlohmann/json.hpp>

namespace sqlite {
    class database;
} // namespace sqlite

class object_collection {
public:
    struct update_options {
        bool multi = false;
        bool upsert = false;
    };

public:
    object_collection(std::shared_ptr<sqlite::database> db,
                      std::string const& name,
                      std::optional<uint64_t> rng_seed = std::nullopt);

    virtual ~object_collection();

    void
    add_index(std::string const& index);

    std::vector<nlohmann::json>
    find(nlohmann::json query);

    void
    insert(nlohmann::json data);

    void
    update(nlohmann::json query,
           nlohmann::json doc,
           update_options const& opts);

    void
    remove(nlohmann::json query);

    std::string
    create_object_id();

protected:
    std::shared_ptr<sqlite::database> db_;
    std::string name_;
    std::mt19937_64 rng_;
};
