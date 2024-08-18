#pragma once

#ifdef __DAOCC__
#   define DAO_META(...) __VA_ARGS__
#else
#   define DAO_META(...)
#endif

#define SQL_NAME(id) DAO_META([simcoe::sql_name(id)])
#define SQL_SEQUENCE(id) DAO_META([[simcoe::sql_sequence(id)]])
#define SQL_COLUMN(...) DAO_META([[simcoe::sql_column(__VA_ARGS__)]])
#define SQL_QUERY() DAO_META([[simcoe::sql_query]])
#define SQL_UPDATE() DAO_META([[simcoe::sql_update]])
#define SQL(...) DAO_META(= delete(#__VA_ARGS__))

SQL_NAME("simcoe.projects")
class Project {
    SQL_COLUMN(primary = true)
    int mId;

    SQL_COLUMN(unique = true)
    std::string mName;

    SQL_COLUMN(unique = true)
    std::string mDescription;
public:
    SQL_QUERY()
    static std::optional<Project> byId(int id) SQL(SELECT * FROM this WHERE id = :id);
};
