Project Documentation: Redis Custom Implementation
Overview of Concepts and Architecture
This project consolidates advanced knowledge in backend systems and networks, structured in the following topics:

Technical Fundamentals
Networks and Sockets: Implementation of client-server communication via TCP/IP and Socket Programming.

Parallel Execution: Use of Concurrency, Multithreading, and Mutex to ensure secure data access.

Data Structures: Organization of information in memory using Hash Tables and Vectors.

Protocol and Parsing: Message processing using the RESP (Redis Serialization Protocol).

Persistence and Signals: File manipulation for disk storage and system signal handling.

Design and Logic: Application of the Singleton pattern, command processing, and use of bitwise operators for efficiency.

Tooling: Extensive use of standard libraries.

Software Structure
RedisServer Class: Main manager of server connectivity and listening.

RedisDatabase Class: Database storage and control engine.

RedisCommandHandler Class: Module for translating and executing user instructions.

Functionalities and Practical Applications
Infrastructure Commands
PING: Used as a readiness test to confirm that the server is responding before starting operations.

ECHO: Used to validate network integrity by repeating the message sent by the client.

FLUSHALL: Completely removes stored content, allowing for total cache or environment clearing.

Key and Value Management
SET and GET: Basic tools for storing and retrieving data, such as user sessions or system configurations.

KEYS: Allows listing and identifying existing keys through search patterns.

TYPE: Checks the nature of the data (string, list, or hash) stored under a specific key.

DEL and UNLINK: Commands for removing obsolete entries or clearing memory.

EXPIRE: Sets a timer for the automatic deletion of temporary data.

RENAME: Changes the name of a key without affecting its value.

List Structures
LPUSH and RPUSH: Adding elements at the beginning or end, common in task queuing systems.

LPOP and RPOP: Dequeuing items from the ends of a list.

LGET and LINDEX: Retrieving all items or a specific element based on its index.

LLEN: Querying the total number of items in a queue.

LREM and LSET: Removing specific values ​​or updating items at specific positions.

Hash Manipulation (Objects)
HSET and HMSET: Creating records with multiple fields, such as a complete user profile.

HGET and HGETALL: Reading individual fields or all key-value pairs of an object.

HEXISTS: Confirmation of whether a specific attribute exists within a hash structure.

HDEL: Deletion of specific properties from a record.

HKEYS, HVALS, and HLEN: Tools for listing keys, values, or counting the total number of attributes in a hash.

Suggested Testing Procedures
To validate the implementation, the user should first start the server executable by specifying a network port and then connect using a compatible command-line tool.

Tests should cover verifying the PING command for a PONG response, creating and retrieving text keys, managing queues using list commands, and building complex objects with hash commands. It is also recommended to test the EXPIRE command, waiting for the defined lifetime to confirm that the data disappears as expected.