#include "database.h"
#include <iostream>

// Constructor: Opens the database, creates tables if they don't exist, and inserts preset data
Database::Database(const std::string& dbPath) : db(nullptr) {
    // Open the database
    if (sqlite3_open(dbPath.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Error opening database: " << sqlite3_errmsg(db) << std::endl;
    } else {
        std::cout << "Database opened successfully." << std::endl;
        // Create tables and insert preset data
        createTables();
        insertPresetData();
    }
}

// Destructor: Closes the database
Database::~Database() {
    if (db) {
        sqlite3_close(db);
        std::cout << "Database closed." << std::endl;
    }
}

// Function to create database tables
void Database::createTables() {
    // SQL statements to create tables
    const char* createUserTableSQL =
        "CREATE TABLE IF NOT EXISTS Users ("
        "ID INTEGER PRIMARY KEY AUTOINCREMENT, "
        "user_name TEXT NOT NULL UNIQUE, "
        "password TEXT NOT NULL, "
        "usd_balance DOUBLE NOT NULL, "
        "root INTEGER NOT NULL, "
        "logged_in INTEGER NOT NULL DEFAULT 0);"; 

    const char* createStocksTableSQL =
    "CREATE TABLE IF NOT EXISTS Stocks ("
        "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
        "stock_symbol TEXT NOT NULL UNIQUE,"
        "stock_name TEXT NOT NULL,"
        "stock_price DOUBLE NOT NULL,"
        "stock_available INTEGER NOT NULL);";

    const char* createUserStocksTableSQL =
        "CREATE TABLE IF NOT EXISTS UserStocks ("
        "ID INTEGER PRIMARY KEY AUTOINCREMENT, "
        "user_id INTEGER NOT NULL, "
        "stock_id INTEGER NOT NULL, "
        "quantity_owned INTEGER NOT NULL, "
        "FOREIGN KEY(user_id) REFERENCES Users(ID), "
        "FOREIGN KEY(stock_id) REFERENCES Stocks(ID));";

    // Execute SQL statements to create tables
    execute(createUserTableSQL);
    execute(createStocksTableSQL);
    execute(createUserStocksTableSQL);
}

// Function to insert preset data into database tables
void Database::insertPresetData() {
    // SQL statements to insert preset data
    const char* insertPresetUsersSQL =
        "INSERT OR IGNORE INTO Users (user_name, password, usd_balance, root, logged_in) VALUES "
        "('root', 'root01', 10000.0, 1, 0),"
        "('mary', 'mary01', 200.0, 0, 0),"
        "('john', 'john01', 150.0, 0, 0),"
        "('moe', 'moe01', 100.0, 0, 0);";

    const char* insertPresetStocksSQL =
        "INSERT OR IGNORE INTO Stocks (stock_symbol, stock_name, stock_price, stock_available) VALUES "
        "('AAPL', 'Apple Inc.', 50.00, 10),"
        "('GOOGL', 'Google LLC', 18.00, 10),"
        "('AMZN', 'Amazon.com', 14.00, 20),"
        "('MSFT', 'Microsoft Corp', 20.00, 5);";

    execute(insertPresetUsersSQL);
    execute(insertPresetStocksSQL);

    const char* insertPresetUserStocksSQL =
        "INSERT OR IGNORE INTO UserStocks (user_id, stock_id, quantity_owned) VALUES "
        "(4, 1, 2),"  // moe owns 2 AAPL
        "(2, 2, 3),"  // mary owns 3 GOOGL
        "(2, 1, 1),"  // mary owns 1 AAPL
        "(3, 3, 5),"  // john owns 5 AMZN
        "(4, 4, 4);"; // moe owns 4 MSFT

    // Execute SQL statements to insert preset data
    execute(insertPresetUserStocksSQL);
}

// Function to execute SQL queries
bool Database::execute(const std::string& sql) {
    char* errMsg = nullptr;
    // Execute SQL query
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        // Print error message if query execution fails
        std::cerr << "SQL Error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}