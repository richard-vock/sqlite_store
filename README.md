# SQLite-based Document/Object Store

This is a proof-of-concept libray implementing a small subset of [MongoDBs](https://mongodb.com) collections
using [SQLite](https://www.sqlite.org) and its [JSON1](https://www.sqlite.org/json1.html) extension.

That means collections are SQL tables with one column (called "data") and JSON1 is used to query, insert and update
JSON documents (implemented using [Niels Lohmann's JSON library](https://github.com/nlohmann)) using an interface
that is designed after MongoDB.

This also means that it is not efficient in any way and should only be used in applications where efficiency is
secondary.

## Installation

## Building

## Synopsis

Initialize the database and create the corresponding file if it does not exist:

    #include <sqlite_store/object_store.hxx>
    
    int main(int, char**)
    {
        object_store store("data.sqlite");

        // ...
    }

Create a collection (if it does not exist) and an index on a single field:

    auto collection = store.collection("test");
    collection->add_index("name");

Insert data:

    using json = nlohmann::json;

    // ...

    json::data data = {
        { "age", 42 },
        { "name", "Paul" },
        { "is_admin", true },
    };

    collection->insert(data);

Queries support the query object syntax of MongoDB and are used to find or update data:

    json query = {
        {
            "name", {
                {
                    "$in", {"Richard", "Paul"}
                }
            }
        },
        {
            "age", {
                {
                    "$gt", 33
                }
            }
        },
        {
            "admin", true
        }
    };

    std::vector<json> docs = collection->find(query);

Updates have `multi` and `upsert` flags that match MongoDB's semantics. Note that every document inserted
has an Object ID called `_id` (unless present at insertion) that matches MongoDBs `ObjectId` (with timestamp
and the rest being all potentially deterministic random bytes).

    json update = {
        {
            "$set", {
                {"name", "Pauline"},
            }
        },
    };
    collection->update(query, update, {.multi = true, .upsert = false});

    data["age"] = 55;
    data["name"] = "Herbert";
    data["admin"] = false;
    // if multi == false the query *must* include an ObjectId, hence the multi = true here
    collection->update(query, data, {.multi = true, .upsert = true});

Removing objects only support the query document syntax of MongoDB:

    collection->remove({
        { "age", 42 }
    });
