## CIS427 Winter 2024 Programming Assignment 2 : Network Sockets - Stock Trading System

### Student Names and Emails
- Leah Mirch (lmirch@umich.edu)

### Youtube DEMO Link
Here is the link to the demo: [YouTube Demo Link Placeholder]

### Introduction
In this second project for CIS 427, I implemented a Stock Trading System within a network socket framework. Utilizing C++ for the main application development and SQLite for database management, I achieved a seamless integration between client and server components over TCP/IP. This Unix-based system allows for simultaneous connections from multiple clients, offering a variety of stock trading operations such as BUY, SELL, LIST, BALANCE, SHUTDOWN, QUIT, LOGIN, LOGOUT, WHO, LOOKUP, and DEPOSIT. By leveraging network programming techniques and employing a multi-threaded server architecture, I ensured efficient handling of concurrent client requests, demonstrating robustness and scalability of our system.

### Running Instructions
#### Running the Server on the UMD server:
```bash
# Connect to UMD VPN
# SSH into the server
ssh lmirch@login.umd.umich.edu
# Enter your password and authenticate with Duo

# Navigate to the project directory
cd /home/l/lmirch/Private/pa2LeahMirch/

# Clean, compile and run the server
make clean
make
make runserver
```
Image of runserver setup in vs code terminal:

<img width="666" alt="Screenshot 2024-03-12 at 12 20 06 PM" src="https://github.com/leahmirch/PA2LeahMirch/assets/146135148/83ff35d1-be83-4253-9f9d-012e4182070e">

#### Running the Client on the UMD Server
```bash
# Connect to UMD VPN
# SSH into the server
ssh lmirch@login.umd.umich.edu
# Enter your password and authenticate with Duo

# Navigate to the project directory
cd /home/l/lmirch/Private/pa2LeahMirch/

# Run the client (specify the mode: local, direct IP, or login portal)
make runclientl  # for local testing
make runclientu # for direct UMD IP
make runclientum # for login.umd portal
```
Images of different runclient setups in vs code terminal:

<img width="666" alt="Screenshot 2024-03-12 at 12 20 19 PM" src="https://github.com/leahmirch/PA2LeahMirch/assets/146135148/0e3c3771-15a0-468e-8084-a24e9249d02b">
<img width="666" alt="Screenshot 2024-03-12 at 12 22 13 PM" src="https://github.com/leahmirch/PA2LeahMirch/assets/146135148/a065f8c0-ec04-471f-b037-10c7ea9433ab">
<img width="666" alt="Screenshot 2024-03-12 at 12 20 27 PM" src="https://github.com/leahmirch/PA2LeahMirch/assets/146135148/be4e705b-a245-4527-bdc1-c12c4ae00141">

### Each Student's Role
- Leah Mirch: Sole contributor to this project, responsible for designing, implementing, and testing all aspects of the Stock Trading System. This includes the server and client components, database management, and ensuring concurrency and network communication.

### Implementation Details
- **Select() Statement**: Utilized to monitor multiple file descriptors, awaiting the availability of one or more for reading, writing, or error detection. This approach facilitates efficient handling of concurrent client connections and server responsiveness.
- **Multithreading**: Employed to handle client requests in parallel, enhancing the server's capacity to manage multiple client sessions simultaneously. Each client connection is assigned to a thread, ensuring isolated handling of requests and responses.
- **Concurrency Management**: A thread pool model is adopted to limit the maximum number of concurrent client connections, optimizing resource utilization and server performance. This mechanism is crucial for maintaining system stability and preventing server overload.
- **Database Integration**: SQLite is used for persisting user and stock data, supporting transactions like BUY, SELL, and BALANCE. Database operations are thread-safe, ensuring data integrity and consistency across multiple client interactions.
- **Networking**: The system uses TCP sockets for reliable data transmission between clients and the server. This choice guarantees that data packets are delivered in order and without loss, which is essential for the accuracy of financial transactions.
- **Protocol Specification Compliance**: The client-server communication adheres to a predefined protocol, outlining the format and sequence of messages exchanged. This strict protocol adherence ensures that commands and responses are correctly interpreted and processed.

### Commands Implemented
This Stock Trading System supports a variety of commands, allowing users to perform stock trading operations, manage their accounts, and interact with the server. Below is a detailed explanation of each command:

- **BUY**: Allows a user to purchase a specified quantity of stocks at a given price. The command format is `BUY <stock_symbol> <quantity> <user_name>`. It checks if the user has enough balance and if the specified stock is available in the required quantity before executing the purchase.
- **SELL**: Enables a user to sell a specified quantity of stocks at a given price. The command format is `SELL <stock_symbol> <quantity> <user_name>`. The system verifies the user owns enough of the stock to sell and processes the sale, updating the user's balance accordingly.
- **LIST**: Lists all available stocks in the system, including their symbols, available quantities, and current prices. This command does not require any parameters and provides an overview of the stock market available to the user.
- **BALANCE**: Checks the current balance and stock holdings of a specified user. The command format is `BALANCE <user_name>`. It returns the user's cash balance along with the stocks they own and the quantity of each.
- **SHUTDOWN**: Initiates a graceful shutdown of the server. This command can only be executed by a user with administrative privileges (e.g., root). It ensures all pending operations are completed before shutting down the server to prevent data loss or corruption.
- **QUIT**: Disconnects the client from the server. This command can be used by a user to terminate their session. The server closes the socket connection associated with the client but continues running to serve other clients.
- **LOGIN**: Allows a user to log into their account using their username and password. The command format is `LOGIN <user_name> <password>`. Upon successful authentication, the user gains access to their account and can perform transactions.
- **LOGOUT**: Logs a user out of the system, ending their session. The command format is `LOGOUT <user_name>`. It ensures the user is logged out properly and their session is terminated securely.
- **WHO**: Lists all users currently logged into the system. This command is typically restricted to administrative users and provides information on active sessions, including the usernames and IP addresses of connected clients.
- **LOOKUP**: Allows users to search for stocks based on their symbol or name. The command format is `LOOKUP <stock_name_or_symbol>`. It returns information about the stock, including its symbol, name, current price, and availability.
- **DEPOSIT**: Enables users to deposit funds into their account. The command format is `DEPOSIT <amount> <user_name>`. It updates the user's cash balance, allowing them to buy more stocks.

These commands constitute the core functionality of the Stock Trading System, offering users a comprehensive set of tools for online stock trading, account management, and interaction with the trading platform.

### Project Evaluation Criteria
- **Select() Statement**: Demonstrated through the efficient management of multiple socket connections, ensuring the server can concurrently monitor and respond to client and server socket activities.
- **Multithreading**: Evident in the server's ability to handle each client request in a separate thread, promoting parallel processing and improved response times.
- **Stability and Error Handling**: The application runs without causing server crashes, robustly handling errors and exceptional conditions in client-server communication.
- **Code Comments and Logging**: The codebase is well-documented with comments explaining the functionality and logic of critical sections. Server activity logging provides insights into operations, aiding in debugging and monitoring.

### Additional Information
This project emphasizes the application of networking and concurrency concepts in a practical, real-world scenario. The implementation showcases the integration of socket programming, multi-threading, and database management to create a functional and responsive Stock Trading System.

Github Repository: [Github Link Placeholder] - This repository documents the development process, including code commits, progress tracking, and collaboration. It serves as a central platform for code management and version control, facilitating a structured and efficient development workflow.
