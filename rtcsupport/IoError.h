#ifndef IOERROR_H
#define IOERROR_H

#include <exception>

enum class Status
{
    Ok,
    Fail,
    SendFail,
    ReceiveFail,
    ReceivedLessThanExpected,
    ReadUnderflow,
    WriteOverflow
};

class IoError : std::exception
{
    Status _status;

public:
    IoError(Status status) :
        _status(status)
    {
    }

    IoError(const IoError& other)
        : std::exception(other)
    {
        _status = other._status;
    }

    Status Value() const
    {
        return _status;
    }
};

#endif // IOERROR_H

