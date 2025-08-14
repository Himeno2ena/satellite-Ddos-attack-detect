#ifndef APT_ATTACK_H
#define APT_ATTACK_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "ns3/tcp-header.h"
#include "ns3/icmpv4.h"
#include <vector>

namespace ns3 {

#define ATTACK_TARGET_PORT 8080

enum AttackProtocol {
    UDP_FLOOD = 0,
    TCP_SYN_FLOOD = 1,
    ICMP_FLOOD = 2
};

class AptAttackApplication : public Application
{
public:
    static TypeId GetTypeId(void);
    AptAttackApplication();
    virtual ~AptAttackApplication();

    void SetPrimaryTarget(Ipv4Address address);
    void SetTargets(const std::vector<Ipv4Address>& targets);

protected:
    virtual void DoDispose(void);

private:
    void SendTcpSyn();
    void SendIcmpEcho();
    void ConnectionSucceeded(Ptr<Socket> socket);
    void ConnectionFailed(Ptr<Socket> socket);
    virtual void StartApplication(void);
    virtual void StopApplication(void);
    void ScheduleNextAttack();
    void LaunchAttack();
    void SimulateDenialOfService();
    void SimulateMultiTargetFlood();

    Ipv4Address     m_primaryTarget;
    Ipv4Address     m_peerAddress;
    uint32_t        m_packetSize;
    uint16_t        m_targetPort;
    Time            m_interval;
    bool            m_running;
    EventId         m_sendEvent;
    AttackProtocol  m_protocol;
    std::vector<Ipv4Address> m_targetList;
};

} // namespace ns3
#endif