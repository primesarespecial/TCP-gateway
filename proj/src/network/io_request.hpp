#pragma once

// Forward declaration to prevent circular includes
class Connection;

struct IoRequest {
    enum class Type {
        Accept,
        Receive,
        Send
    };

    Type type;
    Connection* connection; // Pointer back to the client that owns this request
};