#include "can_socketcan_linux.h"

#ifdef __linux__

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <cstring>
#include <cerrno>
#include <chrono>

SocketCanIface::SocketCanIface() = default;

SocketCanIface::~SocketCanIface() {
    close();
}

bool SocketCanIface::open(const CanConfig& cfg) {
    if (m_fd >= 0) {
        m_lastError = "Already open";
        return false;
    }

    m_fd = ::socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (m_fd < 0) {
        m_lastError = std::string("CAN socket creation failed: ") + strerror(errno);
        return false;
    }

    struct ifreq ifr{};
    std::strncpy(ifr.ifr_name, cfg.channel.c_str(), IFNAMSIZ - 1);
    if (::ioctl(m_fd, SIOCGIFINDEX, &ifr) < 0) {
        m_lastError = std::string("CAN interface '") + cfg.channel
                    + "' not found: " + strerror(errno);
        ::close(m_fd);
        m_fd = -1;
        return false;
    }

    struct sockaddr_can addr{};
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (::bind(m_fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        m_lastError = std::string("CAN bind failed: ") + strerror(errno);
        ::close(m_fd);
        m_fd = -1;
        return false;
    }

    // Set non-blocking for recv with poll
    int flags = ::fcntl(m_fd, F_GETFL, 0);
    ::fcntl(m_fd, F_SETFL, flags | O_NONBLOCK);

    m_lastError.clear();
    return true;
}

void SocketCanIface::close() {
    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
    }
}

bool SocketCanIface::isOpen() const {
    return m_fd >= 0;
}

bool SocketCanIface::send(const CanFrame& frame) {
    if (m_fd < 0) {
        m_lastError = "Not open";
        return false;
    }

    struct can_frame cf{};
    cf.can_id = frame.id;
    if (frame.isExtended) cf.can_id |= CAN_EFF_FLAG;
    if (frame.isRTR)      cf.can_id |= CAN_RTR_FLAG;
    cf.can_dlc = frame.dlc;
    std::memcpy(cf.data, frame.data, frame.dlc);

    ssize_t nbytes = ::write(m_fd, &cf, sizeof(cf));
    if (nbytes != sizeof(cf)) {
        m_lastError = std::string("CAN write failed: ") + strerror(errno);
        return false;
    }
    return true;
}

bool SocketCanIface::recv(CanFrame& out, int timeout_ms) {
    if (m_fd < 0) return false;

    struct pollfd pfd{};
    pfd.fd = m_fd;
    pfd.events = POLLIN;
    int ret = ::poll(&pfd, 1, timeout_ms);

    if (ret <= 0) return false;  // timeout or error
    if (!(pfd.revents & POLLIN)) return false;

    struct can_frame cf{};
    ssize_t nbytes = ::read(m_fd, &cf, sizeof(cf));
    if (nbytes != sizeof(cf)) return false;

    out.id = cf.can_id & CAN_EFF_MASK;
    out.dlc = cf.can_dlc;
    std::memcpy(out.data, cf.data, cf.can_dlc);
    out.isExtended = (cf.can_id & CAN_EFF_FLAG) != 0;
    out.isRTR = (cf.can_id & CAN_RTR_FLAG) != 0;

    auto now = std::chrono::steady_clock::now();
    out.timestamp = std::chrono::duration<double>(now.time_since_epoch()).count();

    return true;
}

#endif // __linux__
