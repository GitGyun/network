/*
 * E_TCPAssignment.hpp
 *
 *  Created on: 2014. 11. 20.
 *      Author: Keunhong Lee
 */

#ifndef E_TCPASSIGNMENT_HPP_
#define E_TCPASSIGNMENT_HPP_


#include <E/Networking/E_Networking.hpp>
#include <E/Networking/E_Host.hpp>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <netinet/in.h>


#include <E/E_TimerModule.hpp>

namespace E
{

class Socket
{
    int domain;
    int protocol;
    bool bound; // whether bounded or not.
    socklen_t namelen;
    struct sockaddr_in name;
    
public:
    Socket(int d, int p) : domain(d), protocol(p), bound(false) {}
    
    bool is_bound() {return bound;}
    
    void set_name(const struct sockaddr_in *addr, socklen_t addrlen) // binds an address.
    {
        name = *addr;
        namelen = addrlen;
        bound = true;
    }
    
    int get_name(struct sockaddr_in *addr, socklen_t *addrlen) // returns bound address.
    {
        if (*addrlen < 0)
            return -1;
        
        *addr = name;
        *addrlen = namelen;
        return 0;
    }
    
    std::pair<uint32_t, in_port_t> get_saddr() // returns bound socket address.
    {
        assert(bound);
        return std::pair<uint32_t, in_port_t>(name.sin_addr.s_addr, name.sin_port);
    }
};

using DTable = std::unordered_map<int, Socket>; // Descriptor table.
using PTable = std::unordered_map<int, std::unordered_map<int, Socket>>; // Process table.
using SAddr = std::pair<uint32_t, in_port_t>; // Socket Address.
using ASpace = std::unordered_set<std::pair<uint32_t, in_port_t>>; // (Used) Address Space.

class TCPAssignment : public HostModule, public NetworkModule, public SystemCallInterface, private NetworkLog, private TimerModule
{
private:
    // Part 1.
    PTable ptable; // Global process table.
    ASpace aspace; // Global Address Space.
    
    int syscall_socket(int pid, int family, int protocol);
    int syscall_close(int pid, int fd);
    int syscall_bind(int pid, int sockfd, const struct sockaddr_in *addr, socklen_t addrlen);
    int syscall_getsockname(int pid, int sockfd, sockaddr_in *addr, socklen_t *addrlen);
    
private:
	virtual void timerCallback(void* payload) final;

public:
	TCPAssignment(Host* host);
	virtual void initialize();
	virtual void finalize();
	virtual ~TCPAssignment();
protected:
	virtual void systemCallback(UUID syscallUUID, int pid, const SystemCallParameter& param) final;
	virtual void packetArrived(std::string fromModule, Packet* packet) final;
};

class TCPAssignmentProvider
{
private:
	TCPAssignmentProvider() {}
	~TCPAssignmentProvider() {}
public:
	static HostModule* allocate(Host* host) { return new TCPAssignment(host); }
};

}


#endif /* E_TCPASSIGNMENT_HPP_ */
