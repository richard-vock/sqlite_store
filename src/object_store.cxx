#include <object_store.hxx>

#include <sqlite_modern_cpp.h>

using nlohmann::json;

object_store::object_store(std::string const& filename)
{
    db_ = std::make_shared<sqlite::database>(filename);
}

object_store::~object_store() = default;

std::shared_ptr<object_collection>
object_store::collection(std::string const& name)
{
    if (!collections_.contains(name)) {
        collections_[name] = std::make_shared<object_collection>(db_, name);
    }
    return collections_[name];
}
