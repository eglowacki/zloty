#include "Network\EndPoints.h"
#include "StringHelpers.h"


//---------------------------------------------------------------------------------------------------------------------
boost::asio::ip::tcp::endpoint yaget::network::CreateEndPoint(const std::string& networkAddress, boost::system::error_code& ec)
{
    Strings ipPortAddress = {"127.0.0.1", "25000"};

    const auto splitTokens = conv::Split(networkAddress, ":", true);
    if (splitTokens.size() == 1)
    {
        ipPortAddress = splitTokens;
        ipPortAddress.push_back(std::to_string(25000));
    }
    else if (splitTokens.size() == 2)
    {
        ipPortAddress = splitTokens;
    }

    const auto address = boost::asio::ip::make_address(ipPortAddress[0], ec);
    if (ec)
    {
        return {};
    }

    const auto port = conv::Convertor<boost::asio::ip::port_type>::FromString(ipPortAddress[1].c_str());
    boost::asio::ip::tcp::endpoint endPoint{address, port};

    return endPoint;
}
