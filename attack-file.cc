#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/attack-file.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("AptAttackModule");

AptAttackApplication::AptAttackApplication()
    : m_primaryTarget(Ipv4Address::GetZero()),
      m_peerAddress(Ipv4Address::GetZero()),
      m_packetSize(512),
      m_targetPort(ATTACK_TARGET_PORT),
      m_interval(Seconds(1.0)),
      m_running(false),
      m_sendEvent(),
      m_targetList()
{
    NS_LOG_FUNCTION(this);
}

AptAttackApplication::~AptAttackApplication()
{
    NS_LOG_FUNCTION(this);
    m_targetList.clear();
}

TypeId AptAttackApplication::GetTypeId(void)
{
    static TypeId tid = TypeId("ns3::AptAttackApplication")
        .SetParent<Application>()
        .SetGroupName("Applications")
        .AddConstructor<AptAttackApplication>()
        .AddAttribute("RemoteAddress", "The destination IP address",
                     Ipv4AddressValue(),
                     MakeIpv4AddressAccessor(&AptAttackApplication::m_peerAddress),
                     MakeIpv4AddressChecker())
        .AddAttribute("PacketSize", "The size of packets sent",
                     UintegerValue(512),
                     MakeUintegerAccessor(&AptAttackApplication::m_packetSize),
                     MakeUintegerChecker<uint32_t>())
        .AddAttribute("Interval", "The time to wait between packets",
                     TimeValue(Seconds(1.0)),
                     MakeTimeAccessor(&AptAttackApplication::m_interval),
                     MakeTimeChecker())
        .AddAttribute("TargetPort", "The destination port",
                     UintegerValue(ATTACK_TARGET_PORT),
                     MakeUintegerAccessor(&AptAttackApplication::m_targetPort),
                     MakeUintegerChecker<uint16_t>())
        .AddAttribute("Protocol", "Attack protocol (0=UDP,1=TCP,2=ICMP)",
                     UintegerValue(0),
                     MakeUintegerAccessor(&AptAttackApplication::m_protocol),
                     MakeUintegerChecker<uint32_t>());
    return tid;
}

void AptAttackApplication::SetPrimaryTarget(Ipv4Address address)
{
    m_primaryTarget = address;
    m_peerAddress = address;
}

void AptAttackApplication::SetTargets(const std::vector<Ipv4Address>& targets)
{
    m_targetList = targets;
    if (!m_targetList.empty() && m_primaryTarget == Ipv4Address::GetZero()) {
        m_primaryTarget = m_targetList[0];
    }
}

void AptAttackApplication::DoDispose()
{
    NS_LOG_FUNCTION(this);
    Application::DoDispose();
}

void AptAttackApplication::StartApplication() {
    m_running = true;
    ScheduleNextAttack();
}

void AptAttackApplication::StopApplication()
{
    NS_LOG_FUNCTION(this);
    m_running = false;
    Simulator::Cancel(m_sendEvent);
}

void AptAttackApplication::ScheduleNextAttack()
{
    NS_LOG_FUNCTION(this);
    if (m_running) {
        m_sendEvent = Simulator::Schedule(m_interval, &AptAttackApplication::LaunchAttack, this);
    }
}

// ========== 攻击核心实现 ==========
void AptAttackApplication::SendTcpSyn()
{
    Ptr<Socket> socket = Socket::CreateSocket(GetNode(), TcpSocketFactory::GetTypeId());
    socket->SetAttribute("MinRto", TimeValue(MilliSeconds(1))); // 设置短超时
    
    if (socket->Bind() == -1) {
        NS_LOG_WARN("TCP Bind failed");
        return;
    }

    Ptr<Packet> packet = Create<Packet>(m_packetSize);
    TcpHeader tcpHeader;
    tcpHeader.SetFlags(TcpHeader::SYN);
    tcpHeader.SetSourcePort(rand() % 65535); // 随机源端口
    packet->AddHeader(tcpHeader);

    socket->Connect(InetSocketAddress(m_peerAddress, m_targetPort));
    socket->Send(packet);
    socket->Close();
}

void AptAttackApplication::SendIcmpEcho()
{
    Ptr<Socket> socket = Socket::CreateSocket(GetNode(), Ipv4RawSocketFactory::GetTypeId());
    socket->Bind(InetSocketAddress(Ipv4Address::GetAny(), 0));

    Icmpv4Echo echo;
    echo.SetSequenceNumber(rand() % 65535);
    Ptr<Packet> packet = Create<Packet>(m_packetSize);
    packet->AddHeader(echo);

    socket->SendTo(packet, 0, InetSocketAddress(m_peerAddress, 0));
    socket->Close();
}

void AptAttackApplication::SimulateDenialOfService()
{
    for (int i = 0; i < 100 && m_running; i++) {
        Ptr<Socket> socket = Socket::CreateSocket(GetNode(), UdpSocketFactory::GetTypeId());
        if (socket->Bind() == -1) {
            NS_LOG_WARN("UDP Bind failed for iteration " << i);
            continue;
        }
        socket->Connect(InetSocketAddress(m_peerAddress, m_targetPort));
        
        Ptr<Packet> packet = Create<Packet>(m_packetSize);
        socket->Send(packet);
        socket->Close();//建立新的flow
    }
}

void AptAttackApplication::LaunchAttack()
{
    NS_LOG_FUNCTION(this);
    
    switch (m_protocol) {
        case UDP_FLOOD:
            NS_LOG_INFO("UDP Flood to " << m_peerAddress);
            SimulateDenialOfService();
            break;
            
        case TCP_SYN_FLOOD:
            NS_LOG_INFO("TCP SYN Flood to " << m_peerAddress);
            for (int i = 0; i < 50; ++i) {
                SendTcpSyn();
            }
            break;
            
        case ICMP_FLOOD:
            NS_LOG_INFO("ICMP Flood to " << m_peerAddress);
            for (int i = 0; i < 50; ++i) {
                SendIcmpEcho();
            }
            break;
            
        default:
            NS_LOG_WARN("Unsupported protocol: " << m_protocol);
            break;
    }
    ScheduleNextAttack();
}

// ========== 连接回调 ==========
void AptAttackApplication::ConnectionSucceeded(Ptr<Socket> socket) {
    NS_LOG_DEBUG("Connection succeeded (unexpected for short-lived sockets)");
}
void AptAttackApplication::ConnectionFailed(Ptr<Socket> socket) {
    NS_LOG_DEBUG("Connection failed (expected for SYN flood)");
}

} // namespace ns3