/*
 * E_TCPAssignment.cpp
 *
 *  Created on: 2014. 11. 20.
 *      Author: Keunhong Lee
 */

#include <E/E_Common.hpp>
#include <E/Networking/E_Host.hpp>
#include <E/Networking/E_Networking.hpp>
#include <cerrno>
#include <E/Networking/E_Packet.hpp>
#include <E/Networking/E_NetworkUtil.hpp>
#include "TCPAssignment.hpp"

namespace E
{

TCPAssignment::TCPAssignment(Host* host) : HostModule("TCP", host),
		NetworkModule(this->getHostModuleName(), host->getNetworkSystem()),
		SystemCallInterface(AF_INET, IPPROTO_TCP, host),
		NetworkLog(host->getNetworkSystem()),
		TimerModule(host->getSystem())
{
    // Create a process table.
    PTable ptable;
    this->ptable = ptable;
    
    // Create an address space.
    ASpace aspace;
    this->aspace = aspace;
}

TCPAssignment::~TCPAssignment()
{
    // Clear the process table.
    PTable::iterator p_iter;
    for (p_iter = ptable.begin(); p_iter != ptable.end(); p_iter++)
    {
        DTable& dtable = p_iter->second;
        dtable.clear();
    }
    ptable.clear();
    
    // Clear the used-port space.
    aspace.clear();
}

void TCPAssignment::initialize()
{
    
}

void TCPAssignment::finalize()
{
    
}

void TCPAssignment::systemCallback(UUID syscallUUID, int pid, const SystemCallParameter& param)
{
	switch(param.syscallNumber)
	{
	case SOCKET:
    {
		int fd = this->syscall_socket(pid, param.param1_int, param.param3_int);
        returnSystemCall(syscallUUID, fd);
        break;
    }
	case CLOSE:
    {
		int n = this->syscall_close(pid, param.param1_int);
		returnSystemCall(syscallUUID, n);
        break;
    }
	case READ:
		//this->syscall_read(syscallUUID, pid, param.param1_int, param.param2_ptr, param.param3_int);
		break;
	case WRITE:
		//this->syscall_write(syscallUUID, pid, param.param1_int, param.param2_ptr, param.param3_int);
		break;
	case CONNECT:
		//this->syscall_connect(syscallUUID, pid, param.param1_int,
		//		static_cast<struct sockaddr*>(param.param2_ptr), (socklen_t)param.param3_int);
		break;
	case LISTEN:
		//this->syscall_listen(syscallUUID, pid, param.param1_int, param.param2_int);
		break;
	case ACCEPT:
		//this->syscall_accept(syscallUUID, pid, param.param1_int,
		//		static_cast<struct sockaddr*>(param.param2_ptr),
		//		static_cast<socklen_t*>(param.param3_ptr));
		break;
	case BIND:
    {
		int n = this->syscall_bind(pid, param.param1_int,
                        static_cast<struct sockaddr_in *>(param.param2_ptr),
                        (socklen_t) param.param3_int);
        returnSystemCall(syscallUUID, n);
		break;
    }
	case GETSOCKNAME:
    {
		int n = this->syscall_getsockname(pid, param.param1_int,
                        static_cast<struct sockaddr_in *>(param.param2_ptr),
                        static_cast<socklen_t*>(param.param3_ptr));
        returnSystemCall(syscallUUID, n);
		break;
    }
	case GETPEERNAME:
		//this->syscall_getpeername(syscallUUID, pid, param.param1_int,
		//		static_cast<struct sockaddr *>(param.param2_ptr),
		//		static_cast<socklen_t*>(param.param3_ptr));
		break;
	default:
		assert(0);
	}
}

void TCPAssignment::packetArrived(std::string fromModule, Packet* packet)
{

}

void TCPAssignment::timerCallback(void* payload)
{

}

int TCPAssignment::syscall_socket(int pid, int domain, int protocol)
{
    // If the process hasn't call socket, create a dtable for it.
    if (ptable.find(pid) == ptable.end())
    {
        DTable dtable;
        ptable.insert(std::pair<int, DTable>(pid, dtable));
    }
    DTable& dtable = ptable.find(pid)->second;
    
    // Create a fd and a socket, then insert their pair into the dtable.
    int fd = createFileDescriptor(pid);
    Socket sock(domain, protocol);
    dtable.insert(std::pair<int, Socket>(fd, sock));
    
    return fd;
}

int TCPAssignment::syscall_close(int pid, int fd)
{
    // If the process hasn't call socket, it should't call close.
    if (ptable.find(pid) == ptable.end())
        return -1;
    
    // Find the socket from its descriptor table.
    DTable& dtable = ptable.find(pid)->second;
    DTable::iterator iter = dtable.find(fd);
    
    // Check fd validity.
    if (iter == dtable.end())
        return -1;
    
    // Free the address if bounded.
    if (iter->second.is_bound())
        aspace.erase(iter->second.get_saddr());
    
    // Remove socket from dtable.
    dtable.erase(iter);
                
    // Free the descriptor.
    removeFileDescriptor(pid, fd);
    
    return 0;
}

int TCPAssignment::syscall_bind(int pid, int sockfd, const struct sockaddr_in *addr, socklen_t addrlen)
{
    // If the process hasn't call socket, error.
    if (ptable.find(pid) == ptable.end())
        return -1;
        
    // Find the descriptor table.
    DTable& dtable = ptable.find(pid)->second;
    
    // Check fd validity.
    if (dtable.find(sockfd) == dtable.end())
        return -1;
    
    // If the socket is already bounded, error.
    Socket& sock = dtable.find(sockfd)->second;
    if (sock.is_bound())
        return -1;
        
    // Check if the port is priviliged port.
    in_port_t port = ntohs(addr->sin_port);
    if (port < 1024 || port > 49151)
        return -1;
        
    // Check if the address is using.
    ASpace::iterator iter;
    for (iter = aspace.begin(); iter != aspace.end(); iter++)
        if (iter->first == addr->sin_addr.s_addr || iter->first == 0)
            if (iter->second == addr->sin_port)
                return -1;
        
    // Set socket address and add the port into the upspace.
    sock.set_name(addr, addrlen);
    aspace.insert(sock.get_saddr());
    
    return 0;
}

int TCPAssignment::syscall_getsockname(int pid, int sockfd, sockaddr_in *addr, socklen_t *addrlen)
{
    // If the process hasn't call socket, error.
    if (ptable.find(pid) == ptable.end())
        return -1;
        
    // Find the descriptor table.
    DTable& dtable = ptable.find(pid)->second;
    
    // Check fd validity.
    if (dtable.find(sockfd) == dtable.end())
        return -1;
        
    // If the socket is not bounded, error.
    Socket& sock = dtable.find(sockfd)->second;
    if (!sock.is_bound())
        return -1;
        
    // Get socket name.
    return sock.get_name(addr, addrlen);
}


}
