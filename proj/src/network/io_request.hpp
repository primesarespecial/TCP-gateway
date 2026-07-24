#pragma once

class Connection;

struct IoRequest {
    enum class Type {
        Accept,
        Receive,
        Send
    };

    Type type;
    Connection* connection; 
};
