#include <iostream>
#include <string>
#include <sstream>  
#include <sys/socket.h>
#include <sqlite3.h>
#include <iomanip>
#include <unistd.h> // For sleep
#include "database.h" 
#include "database.cpp"

extern std::atomic<bool> shutdownRequested; // defined globally
extern sqlite3* db;

// Declaration of command processing functions
void processTestCommand(int clientSocket, const std::string& command);
void processLoginCommand(Database* db, int clientSocket, const std::string& command);
void processLogoutCommand(Database* db, int clientSocket, const std::string& command);
void processWhoCommand(Database* db, int clientSocket, const std::string& clientIp);
void processListCommand(Database* db, int clientSocket, const std::string& command);
void processLookupCommand(Database* db, int clientSocket, const std::string& command);
void processBuyCommand(sqlite3* db, int clientSocket, const std::string& command);
void processSellCommand(sqlite3* db, int clientSocket, const std::string& command);
void processDepositCommand(Database* db, int clientSocket, const std::string& command);
void processBalanceCommand(Database* db, int clientSocket, const std::string& command);
void processShutdownCommand(Database* db, int clientSocket, const std::string& command);
void processQuitCommand(Database* db, int clientSocket, const std::string& command);


// Checks if the given string starts with the specified prefix
bool startsWith(const std::string& fullString, const std::string& prefix) {
    if (fullString.length() < prefix.length()) {
        return false;
    }
    return std::equal(prefix.begin(), prefix.end(), fullString.begin());
}

// Sends a message to the client through the specified socket
void sendMessage(int clientSocket, const std::string& message) {
    ssize_t bytesSent = send(clientSocket, message.c_str(), message.size(), 0);
    if (bytesSent < 0) {
        std::cerr << "Failed to send message to client." << std::endl;
    }
}




// Test Command
// TEST
void processTestCommand(int clientSocket, const std::string& command) {
        sendMessage(clientSocket, "200 OK TESTING\n");
}

// Login Command
// LOGIN user_name user_pass
void processLoginCommand(Database* db, int clientSocket, const std::string& command) {
    std::istringstream iss(command);
    std::string cmd, userID, password;
    iss >> cmd >> userID >> password; // Parse the command

    // Verify if user exists and is already logged in
    std::string sql = "SELECT ID, password, logged_in FROM Users WHERE user_name = ?";
    sqlite3_stmt* stmt;
    int userId = -1;
    std::string dbPassword;
    int loggedIn = 0;

    if (sqlite3_prepare_v2(db->getDb(), sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, userID.c_str(), -1, SQLITE_STATIC);
        
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            userId = sqlite3_column_int(stmt, 0);
            dbPassword = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            loggedIn = sqlite3_column_int(stmt, 2);
        }
        sqlite3_finalize(stmt);
    } else {
        sendMessage(clientSocket, "500 Internal Server Error\n");
        return;
    }

    if (loggedIn) {
        sendMessage(clientSocket, "403 User already logged in\n");
        return;
    }

    if (userId != -1 && password == dbPassword) {
        // User is not logged in and credentials are correct; proceed to update logged_in status to 1
        std::string updateLoginStatus = "UPDATE Users SET logged_in = 1 WHERE ID = ?";
        if (sqlite3_prepare_v2(db->getDb(), updateLoginStatus.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, userId);
            if (sqlite3_step(stmt) == SQLITE_DONE) {
                sendMessage(clientSocket, "200 OK\n");
            } else {
                sendMessage(clientSocket, "500 Internal Server Error: Could not update login status\n");
            }
            sqlite3_finalize(stmt);
        }
    } else {
        sendMessage(clientSocket, "403 Wrong UserID or Password\n");
    }
}

// Logout Command
// LOGOUT user_name
void processLogoutCommand(Database* db, int clientSocket, const std::string& command) {
    std::istringstream iss(command);
    std::string cmd, userID;
    iss >> cmd >> userID;

    // Check if the user exists and is currently logged in
    std::string sqlCheckLoggedIn = "SELECT logged_in FROM Users WHERE user_name = '" + userID + "';";
    sqlite3_stmt* stmt;
    int loggedIn = -1; // Use -1 as uninitialized state

    if (sqlite3_prepare_v2(db->getDb(), sqlCheckLoggedIn.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            loggedIn = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }

    if (loggedIn == 0) {
        sendMessage(clientSocket, "403 User already logged out or does not exist\n");
        return;
    } else if (loggedIn == -1) {
        sendMessage(clientSocket, "403 User does not exist\n");
        return;
    }

    // If user is logged in, proceed to log them out
    if (loggedIn == 1) {
        std::string sql = "UPDATE Users SET logged_in = 0 WHERE user_name = '" + userID + "';";
        if (db->execute(sql)) {
            sendMessage(clientSocket, "200 OK\n");
        } else {
            sendMessage(clientSocket, "500 Internal Server Error\n");
        }
    }
}

// WHO Command
// WHO
void processWhoCommand(Database* db, int clientSocket, const std::string& clientIp) {
    // Check if the requesting user is root and logged in
    std::string checkRootSql = "SELECT user_name FROM Users WHERE root = 1 AND logged_in = 1";
    sqlite3_stmt* stmt;
    bool isRootLoggedIn = false;

    if (sqlite3_prepare_v2(db->getDb(), checkRootSql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            isRootLoggedIn = true;
        }
        sqlite3_finalize(stmt);
    }

    if (!isRootLoggedIn) {
        sendMessage(clientSocket, "403 Forbidden: Only root can execute WHO command.\n");
        return;
    }

    // Retrieve list of logged-in users
    std::string getUsersSql = "SELECT user_name FROM Users WHERE logged_in = 1";
    std::stringstream userList;

    if (sqlite3_prepare_v2(db->getDb(), getUsersSql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        bool first = true;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            if (!first) {
                userList << std::endl;
            }
            const char* userName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            userList << userName << " " << clientIp;
            first = false;
        }
        sqlite3_finalize(stmt);
    }

    if (userList.str().empty()) {
        sendMessage(clientSocket, "200 OK\nNo active users.\n");
    } else {
        std::string response = "200 OK\nThe list of the active users: \n" + userList.str() + "\n";
        sendMessage(clientSocket, response);
    }
}

// LIST Command
// LIST
void processListCommand(Database* db, int clientSocket, const std::string& command) {
    std::string userName; // To store the user's name
    {
        // Check if anyone is logged in
        std::string checkLoggedInSql = "SELECT user_name FROM Users WHERE logged_in = 1";
        sqlite3_stmt* stmt;
        bool isLoggedIn = false;

        if (sqlite3_prepare_v2(db->getDb(), checkLoggedInSql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                userName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                isLoggedIn = true;
            }
            sqlite3_finalize(stmt);
        }

        if (!isLoggedIn) {
            sendMessage(clientSocket, "403 Forbidden: No one is logged in.\n");
            return;
        }
    }

    std::string getListSql;
    if (userName == "root") {
        // Retrieve all stocks for root, including owner information
        getListSql = "SELECT s.ID, s.stock_symbol, us.quantity_owned, u.user_name "
                    "FROM UserStocks us "
                    "JOIN Stocks s ON us.stock_id = s.ID "
                    "JOIN Users u ON us.user_id = u.ID";
    } else {
        // Retrieve only the requesting user's stocks
        getListSql = "SELECT s.ID, s.stock_symbol, us.quantity_owned "
                    "FROM UserStocks us "
                    "JOIN Stocks s ON us.stock_id = s.ID "
                    "JOIN Users u ON us.user_id = u.ID "
                    "WHERE u.user_name = ?";
    }

    sqlite3_stmt* stmt;
    std::stringstream recordList;

    if (sqlite3_prepare_v2(db->getDb(), getListSql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        if (userName != "root") {
            sqlite3_bind_text(stmt, 1, userName.c_str(), -1, SQLITE_STATIC);
        }

        bool first = true;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            if (!first) {
                recordList << std::endl;
            }
            int stockID = sqlite3_column_int(stmt, 0);
            const char* symbol = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            int quantityOwned = sqlite3_column_int(stmt, 2);
            if (userName == "root") {
                const char* owner = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
                recordList << stockID << " " << symbol << ": " << owner << " owns " << quantityOwned << " share(s)";
            } else {
                recordList << stockID << " " << symbol << ": owns " << quantityOwned << " share(s)";
            }
            first = false;
        }
        sqlite3_finalize(stmt);
    }

    if (recordList.str().empty()) {
        sendMessage(clientSocket, "200 OK\nNo records found.\n");
    } else {
        std::string response = "200 OK\nThe list of records in the Stock database";
        if (userName != "root") {
            response += " for " + userName + ":\n";
        } else {
            response += ":\n";
        }
        response += recordList.str() + "\n";
        sendMessage(clientSocket, response);
    }
}

// LOOKUP Command
// LOOKUP stock_name
void processLookupCommand(Database* db, int clientSocket, const std::string& command) {
    std::string userName;
    // Extract the stock symbol from the command
    std::istringstream iss(command);
    std::string lookupCmd, stockSymbol;
    iss >> lookupCmd >> stockSymbol;

    // Check if anyone is logged in
    std::string loggedInUserSql = "SELECT user_name FROM Users WHERE logged_in = 1";
    sqlite3_stmt* stmt;
    bool isLoggedIn = false;

    if (sqlite3_prepare_v2(db->getDb(), loggedInUserSql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            userName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            isLoggedIn = true;
        }
        sqlite3_finalize(stmt);
    }

    if (!isLoggedIn) {
        sendMessage(clientSocket, "403 Forbidden: No one is logged in.\n");
        return;
    }

    // Lookup the stock symbol for the logged-in user
    std::string lookupSql = "SELECT s.ID, s.stock_symbol, us.quantity_owned "
                            "FROM UserStocks us "
                            "JOIN Stocks s ON us.stock_id = s.ID "
                            "JOIN Users u ON us.user_id = u.ID "
                            "WHERE u.user_name = ? AND s.stock_symbol LIKE ?";
    std::stringstream result;

    if (sqlite3_prepare_v2(db->getDb(), lookupSql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, userName.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, ("%" + stockSymbol + "%").c_str(), -1, SQLITE_STATIC);

        bool found = false;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int stockID = sqlite3_column_int(stmt, 0);
            const char* symbol = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            int quantityOwned = sqlite3_column_int(stmt, 2);
            result << stockID << " " << symbol << ": owns " << quantityOwned << " share(s)\n";
            found = true;
        }
        sqlite3_finalize(stmt);

        if (!found) {
            sendMessage(clientSocket, "404 Your search did not match any records.\n");
        } else {
            sendMessage(clientSocket, "200 OK\nFound matches:\n" + result.str());
        }
    } else {
        sendMessage(clientSocket, "500 Internal Server Error\n");
    }
}

// BUY Command
// BUY MSFT 2 john
void processBuyCommand(sqlite3* db, int clientSocket, const std::string& command) {
    std::istringstream iss(command);
    std::string cmd, stockSymbol, userName;
    int stockAmount;
    iss >> cmd >> stockSymbol >> stockAmount >> userName;

    // Check if user is logged in
    std::string userLoggedInSql = "SELECT logged_in FROM Users WHERE user_name = ?";
    sqlite3_stmt* stmtLoggedIn;
    if (sqlite3_prepare_v2(db, userLoggedInSql.c_str(), -1, &stmtLoggedIn, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmtLoggedIn, 1, userName.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(stmtLoggedIn) == SQLITE_ROW) {
            int loggedIn = sqlite3_column_int(stmtLoggedIn, 0);
            if (loggedIn != 1) {
                sendMessage(clientSocket, "403 User not logged in\n");
                sqlite3_finalize(stmtLoggedIn);
                return;
            }
        } else {
            sendMessage(clientSocket, "404 User not found\n");
            sqlite3_finalize(stmtLoggedIn);
            return;
        }
        sqlite3_finalize(stmtLoggedIn);
    } else {
        sendMessage(clientSocket, "500 Internal Server Error\n");
        return;
    }

    char *errMsg = nullptr;
    sqlite3_stmt *stmt = nullptr;
    const char *sql;
    int rc;

    // Begin transaction
    if (sqlite3_exec(db, "BEGIN;", nullptr, nullptr, &errMsg) != SQLITE_OK) {
        sendMessage(clientSocket, "500 Internal Server Error: Failed to start transaction.\n");
        if (errMsg) sqlite3_free(errMsg);
        return;
    }

    // 1. Check user balance
    double balance = 0.0;
    sql = "SELECT usd_balance FROM Users WHERE user_name = ?";
    if ((rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr)) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, userName.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            balance = sqlite3_column_double(stmt, 0);
        } else {
            sqlite3_finalize(stmt);
            sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
            sendMessage(clientSocket, "404 User not found\n");
            return;
        }
    } else {
        sqlite3_finalize(stmt);
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        sendMessage(clientSocket, "500 Internal Server Error: Failed to query user balance.\n");
        return;
    }
    sqlite3_finalize(stmt);

    // 2. Get stock info (price and availability)
    double stockPrice = 0.0;
    int availableStocks = 0;
    sql = "SELECT stock_price, stock_available FROM Stocks WHERE stock_symbol = ?";
    if ((rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr)) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, stockSymbol.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            stockPrice = sqlite3_column_double(stmt, 0);
            availableStocks = sqlite3_column_int(stmt, 1);
        } else {
            sqlite3_finalize(stmt);
            sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
            sendMessage(clientSocket, "404 Stock not found\n");
            return;
        }
    } else {
        sqlite3_finalize(stmt);
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        sendMessage(clientSocket, "500 Internal Server Error: Failed to query stock info.\n");
        return;
    }
    sqlite3_finalize(stmt);

    // 3. Validate transaction feasibility
    double totalCost = stockPrice * stockAmount;
    if (balance < totalCost) {
        sendMessage(clientSocket, "402 Insufficient funds\n");
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return;
    }
    if (availableStocks < stockAmount) {
        sendMessage(clientSocket, "400 Not enough stock available\n");
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return;
    }

    // 4. Update user balance
    sql = "UPDATE Users SET usd_balance = usd_balance - ? WHERE user_name = ?";
    if ((rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr)) == SQLITE_OK) {
        sqlite3_bind_double(stmt, 1, totalCost);
        sqlite3_bind_text(stmt, 2, userName.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            sendMessage(clientSocket, "500 Internal Server Error: Failed to update user balance.\n");
            sqlite3_finalize(stmt);
            sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
            return;
        }
    } else {
        sqlite3_finalize(stmt);
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        sendMessage(clientSocket, "500 Internal Server Error: Failed to prepare balance update.\n");
        return;
    }
    sqlite3_finalize(stmt);

    // 5. Update stock availability
    sql = "UPDATE Stocks SET stock_available = stock_available - ? WHERE stock_symbol = ?";
    if ((rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr)) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, stockAmount);
        sqlite3_bind_text(stmt, 2, stockSymbol.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            sendMessage(clientSocket, "500 Internal Server Error: Failed to update stock availability.\n");
            sqlite3_finalize(stmt);
            sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
            return;
        }
    } else {
        sqlite3_finalize(stmt);
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        sendMessage(clientSocket, "500 Internal Server Error: Failed to prepare stock update.\n");
        return;
    }
    sqlite3_finalize(stmt);

    // 6. Update or insert into UserStocks
    int userId = -1, stockId = -1, currentQuantity = 0;
    bool stockExists = false;
    
    // Get user ID and stock ID
    sql = "SELECT ID FROM Users WHERE user_name = ?";
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, userName.c_str(), -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        userId = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    
    sql = "SELECT ID FROM Stocks WHERE stock_symbol = ?";
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, stockSymbol.c_str(), -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        stockId = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    
    // Check if the user already owns the stock
    sql = "SELECT quantity_owned FROM UserStocks WHERE user_id = ? AND stock_id = ?";
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_int(stmt, 2, stockId);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        currentQuantity = sqlite3_column_int(stmt, 0);
        stockExists = true;
    }
    sqlite3_finalize(stmt);
    
    // Update or insert based on existence
    if (stockExists) {
        sql = "UPDATE UserStocks SET quantity_owned = quantity_owned + ? WHERE user_id = ? AND stock_id = ?";
        sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, stockAmount);
        sqlite3_bind_int(stmt, 2, userId);
        sqlite3_bind_int(stmt, 3, stockId);
    } else {
        sql = "INSERT INTO UserStocks (user_id, stock_id, quantity_owned) VALUES (?, ?, ?)";
        sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, userId);
        sqlite3_bind_int(stmt, 2, stockId);
        sqlite3_bind_int(stmt, 3, stockAmount);
    }
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sendMessage(clientSocket, "500 Internal Server Error: Failed to update user stocks.\n");
        sqlite3_finalize(stmt);
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return;
    }
    sqlite3_finalize(stmt);

    // Commit transaction
    if (sqlite3_exec(db, "COMMIT;", nullptr, nullptr, &errMsg) != SQLITE_OK) {
        sendMessage(clientSocket, "500 Internal Server Error: Failed to commit transaction.\n");
        if (errMsg) sqlite3_free(errMsg);
        return;
    }

    // Success message
    std::ostringstream oss;
    oss << "200 OK\nBUY: " << userName << " bought " << stockAmount << " " << stockSymbol << "\nTotal cost: $" << std::fixed << std::setprecision(2) << (stockPrice * stockAmount) << std::endl;
    sendMessage(clientSocket, oss.str());

}

// SELL Command
// SELL MSFT 2 john
void processSellCommand(sqlite3* db, int clientSocket, const std::string& command) {
    std::istringstream iss(command);
    std::string cmd, stockSymbol, userName;
    int stockAmount;
    iss >> cmd >> stockSymbol >> stockAmount >> userName;

    // Check if user is logged in
    std::string userLoggedInSql = "SELECT logged_in FROM Users WHERE user_name = ?";
    sqlite3_stmt* stmtLoggedIn;
    if (sqlite3_prepare_v2(db, userLoggedInSql.c_str(), -1, &stmtLoggedIn, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmtLoggedIn, 1, userName.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(stmtLoggedIn) == SQLITE_ROW) {
            int loggedIn = sqlite3_column_int(stmtLoggedIn, 0);
            if (loggedIn != 1) {
                sendMessage(clientSocket, "403 User not logged in\n");
                sqlite3_finalize(stmtLoggedIn);
                return;
            }
        } else {
            sendMessage(clientSocket, "404 User not found\n");
            sqlite3_finalize(stmtLoggedIn);
            return;
        }
        sqlite3_finalize(stmtLoggedIn);
    } else {
        sendMessage(clientSocket, "500 Internal Server Error\n");
        return;
    }

    char *errMsg = nullptr;
    sqlite3_stmt *stmt = nullptr;
    const char *sql;
    int rc;

    // Begin transaction
    if (sqlite3_exec(db, "BEGIN;", nullptr, nullptr, &errMsg) != SQLITE_OK) {
        sendMessage(clientSocket, "500 Internal Server Error: Failed to start transaction.\n");
        if (errMsg) sqlite3_free(errMsg);
        return;
    }

    // 1. Verify the user owns enough of the specified stock
    int userId = -1, stockId = -1, currentQuantity = 0;
    double stockPrice = 0.0;

    // Get user ID
    sql = "SELECT ID FROM Users WHERE user_name = ?";
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, userName.c_str(), -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        userId = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);

    // Get stock ID and current price
    sql = "SELECT ID, stock_price FROM Stocks WHERE stock_symbol = ?";
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, stockSymbol.c_str(), -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        stockId = sqlite3_column_int(stmt, 0);
        stockPrice = sqlite3_column_double(stmt, 1);
    }
    sqlite3_finalize(stmt);

    // Check if the user already owns the stock and how much
    sql = "SELECT quantity_owned FROM UserStocks WHERE user_id = ? AND stock_id = ?";
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_int(stmt, 2, stockId);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        currentQuantity = sqlite3_column_int(stmt, 0);
        if (currentQuantity < stockAmount) {
            sendMessage(clientSocket, "403 Not enough stock owned to sell.\n");
            sqlite3_finalize(stmt);
            sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
            return;
        }
    } else {
        sendMessage(clientSocket, "404 User or stock not found.\n");
        sqlite3_finalize(stmt);
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return;
    }
    sqlite3_finalize(stmt);
    
    // 3. Calculate total sale value and update user balance
    double totalSaleValue = stockPrice * stockAmount;
    sql = "UPDATE Users SET usd_balance = usd_balance + ? WHERE ID = ?";
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_double(stmt, 1, totalSaleValue);
    sqlite3_bind_int(stmt, 2, userId);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sendMessage(clientSocket, "500 Internal Server Error: Failed to update user balance.\n");
        sqlite3_finalize(stmt);
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return;
    }
    sqlite3_finalize(stmt);
    
    // 4. Decrease the quantity of the stock owned
    sql = "UPDATE UserStocks SET quantity_owned = quantity_owned - ? WHERE user_id = ? AND stock_id = ?";
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, stockAmount);
    sqlite3_bind_int(stmt, 2, userId);
    sqlite3_bind_int(stmt, 3, stockId);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sendMessage(clientSocket, "500 Internal Server Error: Failed to update stock ownership.\n");
        sqlite3_finalize(stmt);
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return;
    }
    sqlite3_finalize(stmt);

    // Commit transaction
    if (sqlite3_exec(db, "COMMIT;", nullptr, nullptr, &errMsg) != SQLITE_OK) {
        sendMessage(clientSocket, "500 Internal Server Error: Failed to commit transaction.\n");
        if (errMsg) sqlite3_free(errMsg);
        return;
    }

    // Success message
    std::ostringstream oss;
    oss << "200 OK\nSELL: " << userName << " sold " << stockAmount << " " << stockSymbol << "\nTotal sale value: $" << std::fixed << std::setprecision(2) << totalSaleValue << std::endl;
    sendMessage(clientSocket, oss.str());
}

// DEPOSIT Command
// DEPOSIT 100 john
void processDepositCommand(Database* db, int clientSocket, const std::string& command) {
    std::istringstream iss(command);
    std::string cmd, userID;
    double amount;
    iss >> cmd >> amount >> userID;

    // First, check if the amount is valid
    if (amount <= 0) {
        sendMessage(clientSocket, "400 Bad Request: Invalid amount\n");
        return;
    }

    // Check if user exists and is logged in
    std::string checkUserSql = "SELECT logged_in FROM Users WHERE user_name = ?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db->getDb(), checkUserSql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, userID.c_str(), -1, SQLITE_STATIC);

        int loggedIn = 0;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            loggedIn = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);

        // If the user is not logged in, don't proceed with the deposit
        if (loggedIn != 1) {
            sendMessage(clientSocket, "403 Forbidden: User not logged in or does not exist\n");
            return;
        }
    } else {
        sendMessage(clientSocket, "500 Internal Server Error: Could not check user login status\n");
        return;
    }

    // Proceed with updating the user's balance
    std::string updateBalanceSql = "UPDATE Users SET usd_balance = usd_balance + ? WHERE user_name = ?";
    if (sqlite3_prepare_v2(db->getDb(), updateBalanceSql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_double(stmt, 1, amount);
        sqlite3_bind_text(stmt, 2, userID.c_str(), -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) == SQLITE_DONE) {
            // Retrieve updated balance to include in the success message
            std::string getBalanceSql = "SELECT usd_balance FROM Users WHERE user_name = ?";
            if (sqlite3_prepare_v2(db->getDb(), getBalanceSql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_text(stmt, 1, userID.c_str(), -1, SQLITE_STATIC);
                if (sqlite3_step(stmt) == SQLITE_ROW) {
                    double newBalance = sqlite3_column_double(stmt, 0);
                    std::ostringstream oss;
                    oss << "Deposit successful. \n" << userID << "'s new balance: $" << std::fixed << std::setprecision(2) << newBalance << "\n";
                    sendMessage(clientSocket, oss.str());
                } else {
                    sendMessage(clientSocket, "500 Internal Server Error: Could not retrieve updated balance\n");
                }
            } else {
                sendMessage(clientSocket, "500 Internal Server Error: Could not prepare statement to retrieve balance\n");
            }
        } else {
            sendMessage(clientSocket, "500 Internal Server Error: Could not update balance\n");
        }
        sqlite3_finalize(stmt);
    } else {
        sendMessage(clientSocket, "500 Internal Server Error: Could not prepare statement to update balance\n");
    }
}

// BALANCE Command
// BALANCE john
void processBalanceCommand(Database* db, int clientSocket, const std::string& command) {
    std::istringstream iss(command);
    std::string cmd, userID;
    iss >> cmd >> userID;

    // Check if user exists and is logged in
    std::string checkUserSql = "SELECT logged_in, usd_balance FROM Users WHERE user_name = ?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db->getDb(), checkUserSql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, userID.c_str(), -1, SQLITE_STATIC);

        int loggedIn = 0;
        double balance = 0.0;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            loggedIn = sqlite3_column_int(stmt, 0);
            balance = sqlite3_column_double(stmt, 1);
        }
        sqlite3_finalize(stmt);

        if (loggedIn != 1) {
            sendMessage(clientSocket, "403 Forbidden: User not logged in or does not exist\n");
            return;
        } else {
            std::ostringstream oss;
            oss << "200 OK\nBalance for user " << userID << ": $" << std::fixed << std::setprecision(2) << balance << "\n";
            sendMessage(clientSocket, oss.str());
        }
    } else {
        sendMessage(clientSocket, "500 Internal Server Error: Could not check user login status\n");
        return;
    }
}

// SHUTDOWN Command
// SHUTDOWN
void processShutdownCommand(Database* db, int clientSocket, const std::string& command) {
   // Check if the requesting user is root and logged in
    std::string checkRootSql = "SELECT user_name FROM Users WHERE root = 1 AND logged_in = 1";
    sqlite3_stmt* stmt;
    bool isRootLoggedIn = false;

    if (sqlite3_prepare_v2(db->getDb(), checkRootSql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            isRootLoggedIn = true;
        }
        sqlite3_finalize(stmt);
    }

    if (!isRootLoggedIn) {
        sendMessage(clientSocket, "403 Forbidden: Only root can execute SHUTDOWN command.\n");
        return;
    }

    sendMessage(clientSocket, "200 OK\n* Server shutting down *\n");

    // Signal all threads to start cleanup
    shutdownRequested.store(true);

    // Wait for a period to allow threads to complete their cleanup
    std::cout << "Waiting for threads to finish cleanup..." << std::endl;
    sleep(10); // Wait for 10 seconds
    
    std::cout << "Goodbye! \n" << std::endl;

    exit(0);
}

// QUIT Command
// QUIT
void processQuitCommand(Database* db, int clientSocket, const std::string& command) {
    sendMessage(clientSocket, "200 OK\nReceived command: QUIT");
    close(clientSocket);
}
