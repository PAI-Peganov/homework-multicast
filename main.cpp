#include <cstdlib>
#include <iostream>

#include "config.hpp"
#include "serial.hpp"
#include "udp.hpp"

namespace
{

void print_usage(std::string_view basename)
{
    std::cout << "Usage: " << basename << " <CONFIG_FILE>\n";
}

void print_prompt()
{
    std::cout << "Enter message: ";
}

using namespace multicast;

class Client
{
public:
    virtual bool send(std::string_view) const
    {
        return false;
    }
};

using Clients = std::vector<std::unique_ptr<Client>>;

class ClientUdp : public Client
{
    const udp::Socket* socket_;
    const sockaddr_in address_;

public:
    ClientUdp(const udp::Socket* socket, const sockaddr_in& address)
        : socket_(socket), address_(address)
    {
    }

    bool send(std::string_view message) const override
    {
        return udp::send(*socket_, address_, message);
    }
};

class ClientSerial : public Client
{
    const serial::Device device_;

public:
    ClientSerial(std::string_view device) : device_(serial::Device(device))
    {
    }

    bool send(std::string_view message) const override
    {
        return serial::send(device_, message);
    }
};

} // namespace

int main(int argc, char* argv[])
{
    if (argc == 2 && std::string_view(argv[1]) == "-h")
    {
        print_usage(argv[0]);
        return EXIT_SUCCESS;
    }

    if (argc < 2)
    {
        std::cerr << "Not enough arguments!\n";
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    try
    {
        using namespace multicast;
        Config config{argv[1]};
        udp::Socket socket{};
        Clients clients{};

        for ([[maybe_unused]] const auto& entry : config.GetUdpEntries())
        {
            clients.push_back(
                std::make_unique<ClientUdp>(&socket, entry.address));
        }

        for ([[maybe_unused]] const auto& entry : config.GetSerialEntries())
        {
            clients.push_back(std::make_unique<ClientSerial>(entry.device));
        }

        std::string input{};
        while (print_prompt(), std::getline(std::cin, input))
        {
            input.push_back('\n');
            for (const auto& client : clients)
            {
                if (!client->send(input))
                {
                    client->send(input);
                }
            }
        }
        return EXIT_SUCCESS;
    }
    catch (const std::exception& err)
    {
        std::cerr << "Multicast failure: " << err.what() << '\n';
    }
    return EXIT_FAILURE;
}
