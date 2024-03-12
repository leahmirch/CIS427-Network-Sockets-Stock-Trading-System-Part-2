#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include <string>

// Class declaration for Database handling
class Database {
public:
    // Constructor: Opens the database, creates tables if they don't exist, and inserts preset data
    Database(const std::string& dbPath);

    // Destructor: Closes the database
    ~Database();

    // Function to execute SQL queries
    bool execute(const std::string& sql);

    // Getter function to return the SQLite database object
    sqlite3* getDb() const { return db; }

    // Additional getter function to return the SQLite database object (same as getDb)
    sqlite3* getDbConnection() const { return db; }

private:
    sqlite3* db; // SQLite database object

    // Function to create database tables
    void createTables();

    // Function to insert preset data into database tables
    void insertPresetData();
};

#endif // DATABASE_H
