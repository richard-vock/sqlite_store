#include <object_collection.hxx>

#include <chrono>
#include <stdexcept>
#include <fmt/format.h>

#include <sqlite_modern_cpp.h>

using nlohmann::json;

namespace {

std::string
format_query(json query) {
    std::string result;
    bool first = true;
    for (auto [key, value] : query.items()) {
        if (!std::exchange(first, false)) {
            result += " and ";
        }

        std::string fetch = fmt::format("json_extract(data, '$.{}')", key);
        auto format_eq = [&](json v) {
            if (value.is_boolean()) {
                return fmt::format("{} {}", fetch, v.get<bool>() ? "> 0" : "= 0");
            }
            return fmt::format("{} = {}", fetch, v.dump());
        };
        auto format_neq = [&](json v) {
            if (value.is_boolean()) {
                return fmt::format("{} {}", fetch, v.get<bool>() ? "= 0" : "> 0");
            }
            return fmt::format("{} != {}", fetch, v.dump());
        };

        if (value.is_string() || value.is_number() || value.is_boolean()) {
            result += format_eq(value);
        }

        if (value.is_object()) {
            if (value.contains("$eq")) {
                result += fmt::format("{} = {}", fetch, value["$eq"].dump());
            }
            if (value.contains("$ne")) {
                result += fmt::format("{} != {}", fetch, value["$ne"].dump());
            }
            if (value.contains("$gt")) {
                result += fmt::format("{} > {}", fetch, value["$gt"].dump());
            }
            if (value.contains("$gte")) {
                result += fmt::format("{} >= {}", fetch, value["$gte"].dump());
            }
            if (value.contains("$lt")) {
                result += fmt::format("{} < {}", fetch, value["$lt"].dump());
            }
            if (value.contains("$lte")) {
                result += fmt::format("{} <= {}", fetch, value["$lte"].dump());
            }
            if (value.contains("$not") && value["$not"].is_object()) {
                result += fmt::format("!({})", format_query(value["$not"]));
            }
            if (value.contains("$in") && value["$in"].is_array()) {
                result += "(";
                bool cmp_first = true;
                for (auto cmp : value["$in"]) {
                    if (!std::exchange(cmp_first, false)) {
                        result += " or ";
                    }
                    result += format_eq(cmp);
                }
                result += ")";
            }
            if (value.contains("$nin") && value["$nin"].is_array()) {
                result += "(";
                bool cmp_first = true;
                for (auto cmp : value["$in"]) {
                    if (!std::exchange(cmp_first, false)) {
                        result += " and ";
                    }
                    result += format_neq(cmp);
                }
                result += ")";
            }
            if (value.contains("$or") && value["$or"].is_array()) {
                result += "(";
                bool cmp_first = true;
                for (auto && sub_query : value["$or"]) {
                    if (!std::exchange(cmp_first, false)) {
                        result += " or ";
                    }
                    result += fmt::format("({})", format_query(sub_query));
                }
                result += ")";
            }
            if (value.contains("$and") && value["$and"].is_array()) {
                result += "(";
                bool cmp_first = true;
                for (auto && sub_query : value["$and"]) {
                    if (!std::exchange(cmp_first, false)) {
                        result += " and ";
                    }
                    result += fmt::format("({})", format_query(sub_query));
                }
                result += ")";
            }
        }
    }

    return result;
}

inline std::vector<json>
query(std::shared_ptr<sqlite::database> db,
      std::string const& q)
{
    std::vector<json> results;
    try {
        (*db) << q >> [&](std::string data) {
            results.emplace_back(json::parse(data));
        };
    } catch(sqlite::sqlite_exception& e) {
        std::cerr << e.get_code() << ": " << e.what() << " during "
                  << e.get_sql() << "\n";
    }
    return results;
}

inline uint32_t
query_count(std::shared_ptr<sqlite::database> db,
            std::string const& q)
{
    uint32_t count = 0;
    try {
        (*db) << q >> [&](uint32_t n) {
            count = n;
        };
    } catch(sqlite::sqlite_exception& e) {
        std::cerr << e.get_code() << ": " << e.what() << " during "
                  << e.get_sql() << "\n";
    }
    return count;
}

} // anonymous namespace

object_collection::object_collection(std::shared_ptr<sqlite::database> db,
                                     std::string const& name,
                                     std::optional<uint64_t> rng_seed) :
    db_(db),
    name_(name),
    rng_(rng_seed.value_or(std::random_device()()))
{
    query(db_, fmt::format(R"query(
        create table if not exists {} (data TEXT);
    )query", name));
}

object_collection::~object_collection() = default;

void
object_collection::add_index(std::string const& index)
{
    query(db_, fmt::format(R"query(
        create index if not exists {0}_name on {0}(json_extract(data, '$.{1}'));
    )query", name_, index));
}

std::vector<json>
object_collection::find(json q)
{
    return query(db_, fmt::format(R"query(
        select * from {} where {};
    )query", name_, format_query(q)));
}

void
object_collection::insert(json data)
{
    if (!data.contains("_id")) {
        data["_id"] = create_object_id();
    }
    query(db_, fmt::format(R"query(
        insert into {} values ('{}');
    )query", name_, data.dump()));
}

void
object_collection::update(json q,
                          json doc,
                          update_options const& opts)
{
    if (!opts.multi && !q.contains("_id")) {
        throw std::invalid_argument("Query object must include an _id unless multi == true");
    }

    std::string set_expr = doc.contains("$set")
        ? fmt::format("set data = json_patch(data, '{}')", doc["$set"].dump())
        : fmt::format("set data = '{}'", doc.dump());

    std::string search = format_query(q);

    if (opts.upsert) {
        std::string count_query = fmt::format(R"query(
            select count(data) from {} where {}
        )query", name_, search);
        uint32_t count = query_count(db_, count_query);
        if (!count) {
            // no matches, insert instead
            insert(doc);
            return;
        }
    }
    std::string full_query = fmt::format("update {} {} where {}", name_, set_expr, search);
    query(db_, full_query);

    return;
}

void
object_collection::remove(json q)
{
    query(db_, fmt::format(R"query(
        delete from {}
        where {}
    )query", name_, format_query(q)));
}

std::string
object_collection::create_object_id()
{
    return fmt::format("{:8x}{:0>16x}",
                       std::chrono::seconds(std::time(NULL)).count(),
                       rng_());
}
