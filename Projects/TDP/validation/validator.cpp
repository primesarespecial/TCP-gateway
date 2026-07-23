#include "validator.hpp"

bool Validator::validate(const Order& order)
{
    if (order.orderId == 0)
        return false;

    if (order.quantity == 0)
        return false;

    if (order.price == 0)
        return false;

    if (order.symbol == 0)
        return false;

    if (seenOrders_.contains(order.orderId))
        return false;

    seenOrders_.insert(order.orderId);

    return true;
}