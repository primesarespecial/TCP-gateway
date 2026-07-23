#pragma once
#include <cstdint>

class Connection;

struct IoRequest
{
    enum class Type : uint8_t
    {
        Accept,
        Receive,
        Send
    };

    Type type;
    Connection* connection = nullptr;
};