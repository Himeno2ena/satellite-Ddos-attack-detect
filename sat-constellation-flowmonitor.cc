/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014 Magister Solutions
 * Modified by: [Your Name] - Added APT attack simulation and enhanced flow monitoring
 */

#include "ns3/applications-module.h"
#include "ns3/config-store-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/satellite-module.h"
#include "ns3/traffic-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-flow-classifier.h"
#include "ns3/flow-monitor.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/tcp-hybla.h"
#include "ns3/tcp-vegas.h"
#include <fstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("sat-constellation-flowmonitor");

int main(int argc, char* argv[])
{
    uint32_t packetSize = 512;
    std::string interval = "20ms";
    std::string scenarioFolder = "constellation-eutelsat-geo-2-sats-isls";
    bool enableAptAttack = true;

    Ptr<SimulationHelper> simulationHelper =
        CreateObject<SimulationHelper>("example-constellation");

    CommandLine cmd;
    cmd.AddValue("packetSize", "Size of constant packet (bytes)", packetSize);
    cmd.AddValue("interval", "Interval to sent packets in seconds (e.g. (1s))", interval);
    cmd.AddValue("scenarioFolder", "Scenario folder", scenarioFolder);
    cmd.AddValue("enableAptAttack", "Enable APT attack simulation (true/false)", enableAptAttack);
    simulationHelper->AddDefaultUiArguments(cmd);
    cmd.Parse(argc, argv);

    

    //其他
    Config::SetDefault("ns3::SatConf::ForwardLinkRegenerationMode",
                       EnumValue(SatEnums::REGENERATION_NETWORK));
    Config::SetDefault("ns3::SatConf::ReturnLinkRegenerationMode",
                       EnumValue(SatEnums::REGENERATION_NETWORK));
    Config::SetDefault("ns3::SatOrbiterFeederPhy::QueueSize", UintegerValue(100000));
    Config::SetDefault("ns3::SatOrbiterUserPhy::QueueSize", UintegerValue(100000));
    Config::SetDefault("ns3::PointToPointIslHelper::IslDataRate",
                       DataRateValue(DataRate("100Mb/s")));
    Config::SetDefault("ns3::SatSGP4MobilityModel::UpdatePositionEachRequest", BooleanValue(false));
    Config::SetDefault("ns3::SatSGP4MobilityModel::UpdatePositionPeriod", TimeValue(Seconds(10)));
    Config::SetDefault("ns3::SatHelper::GwUsers", UintegerValue(30));
    Config::SetDefault("ns3::SatGwMac::SendNcrBroadcast", BooleanValue(false));
    Config::SetDefault("ns3::SatHelper::BeamNetworkAddress", Ipv4AddressValue("20.1.0.0"));
    Config::SetDefault("ns3::SatHelper::GwNetworkAddress", Ipv4AddressValue("10.1.0.0"));
    Config::SetDefault("ns3::SatHelper::UtNetworkAddress", Ipv4AddressValue("250.1.0.0"));
    Config::SetDefault("ns3::SatBbFrameConf::AcmEnabled", BooleanValue(true));
    Config::SetDefault("ns3::SatEnvVariables::EnableSimulationOutputOverwrite", BooleanValue(true));
    Config::SetDefault("ns3::SatHelper::PacketTraceEnabled", BooleanValue(true));

    simulationHelper->LoadScenario(scenarioFolder);
    simulationHelper->SetSimulationTime(Seconds(30));

    std::set<uint32_t> beamSetAll = {1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,
                                     16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
                                     31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45,
                                     46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
                                     61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72};

    std::set<uint32_t> beamSet = {43, 30};
    std::set<uint32_t> beamSetTelesat = {1, 43, 60, 64};
    if (scenarioFolder == "constellation-telesat-351-sats") {
        simulationHelper->SetBeamSet(beamSetTelesat);
    } else {
        simulationHelper->SetBeamSet(beamSet);
    }
    simulationHelper->SetUserCountPerUt(30);//用户数量

    LogComponentEnable("sat-constellation-flowmonitor", LOG_LEVEL_INFO);
    LogComponentEnable("FlowMonitor", LOG_LEVEL_DEBUG);
    simulationHelper->CreateSatScenario();

    Singleton<SatTopology>::Get()->PrintTopology(std::cout);
    Singleton<SatIdMapper>::Get()->ShowIslMap();

    Config::SetDefault("ns3::CbrApplication::Interval", StringValue(interval));
    Config::SetDefault("ns3::CbrApplication::PacketSize", UintegerValue(packetSize));
    Time startTime = Seconds(1.0);
    Time stopTime = Seconds(5.0);
    Time startDelay = Seconds(0.0);

    NodeContainer gws = Singleton<SatTopology>::Get()->GetGwNodes();
    NodeContainer uts = Singleton<SatTopology>::Get()->GetUtNodes();
    NodeContainer gwUsers = Singleton<SatTopology>::Get()->GetGwUserNodes();
    NodeContainer utUsers = Singleton<SatTopology>::Get()->GetUtUserNodes(uts);

    Ptr<SatTrafficHelper> trafficHelper = simulationHelper->GetTrafficHelper();
    uint16_t basePort = 9000;
    uint16_t numPorts = 5;
    for (uint16_t portOffset = 0; portOffset < numPorts; ++portOffset) {
        trafficHelper->AddCbrTraffic(SatTrafficHelper::FWD_LINK,
                                     SatTrafficHelper::TCP,
                                     Time(interval),
                                     packetSize,
                                     gwUsers,
                                     utUsers,
                                     startTime,
                                     stopTime,
                                     startDelay,
                                     basePort + portOffset);
        trafficHelper->AddCbrTraffic(SatTrafficHelper::RTN_LINK,
                                     SatTrafficHelper::TCP,
                                     Time(interval),
                                     packetSize,
                                     gwUsers,
                                     utUsers,
                                     startTime,
                                     stopTime,
                                     startDelay,
                                     basePort + portOffset);
    }

    if (enableAptAttack) {
    // 确保UT节点存在
    if (uts.GetN() < 3) {
        NS_FATAL_ERROR("Not enough UT nodes (need at least 3)");
    }

    // 选择有效节点
    Ptr<Node> attacker = utUsers.Get(0); // userID 5
    Ptr<Node> mainVictim = utUsers.Get(10); // userID 15
    
    // 验证节点有效性
    if (!attacker || !mainVictim) {
        NS_FATAL_ERROR("Invalid node selection");
    }

    Ptr<AptAttackApplication> app = CreateObject<AptAttackApplication>();

    // 安装网络栈
    InternetStackHelper internet;
    internet.Install(attacker);
    
    // 设置攻击目标
    Ptr<Ipv4> victimIpv4 = mainVictim->GetObject<Ipv4>();
    if (!victimIpv4 || victimIpv4->GetNInterfaces() < 2) {
        NS_FATAL_ERROR("Victim node has no valid IP stack");
    }
    
    Ipv4Address victimAddr = victimIpv4->GetAddress(1, 0).GetLocal();
    
    // 创建攻击应用
    app->SetPrimaryTarget(victimAddr);
    
    // 添加其他目标
    std::vector<Ipv4Address> targets = {victimAddr};
    for (int i = 1; i <= 3; i++) {
        if (utUsers.GetN() > static_cast<uint32_t>(10 + i)) {
            Ptr<Node> extraVictim = utUsers.Get(10+i);
            Ptr<Ipv4> ipv4 = extraVictim->GetObject<Ipv4>();
            if (ipv4 && ipv4->GetNInterfaces() > 1) {
                targets.push_back(ipv4->GetAddress(1, 0).GetLocal());
            }
        }
    }
    app->SetTargets(targets);

    // 流统计验证
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.GetMonitor();
    // 配置攻击参数
    app->SetAttribute("Interval", TimeValue(MilliSeconds(100)));
    app->SetAttribute("PacketSize", UintegerValue(512));
    app->SetAttribute("Protocol", UintegerValue(1));//0=UDP,1=TCP,2=ICMP
    app->SetAttribute("TargetPort", UintegerValue(8080)); 

    attacker->AddApplication(app);
    app->SetStartTime(Seconds(2.0));
    app->SetStopTime(Seconds(3.2));
}


    NS_LOG_INFO("--- sat-constellation-flowmonitor ---");
    NS_LOG_INFO("  PacketSize: " << packetSize);
    NS_LOG_INFO("  Interval: " << interval);
    NS_LOG_INFO("  APT Attack: " << (enableAptAttack ? "Enabled" : "Disabled"));

    Config::SetDefault("ns3::ConfigStore::Filename", StringValue("output-attributes.xml"));
    Config::SetDefault("ns3::ConfigStore::FileFormat", StringValue("Xml"));
    Config::SetDefault("ns3::ConfigStore::Mode", StringValue("Save"));
    ConfigStore outputConfig;
    outputConfig.ConfigureDefaults();

    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    simulationHelper->RunSimulation();

    std::string flowmonFilename = enableAptAttack ? "attack-tcp.xml" : "normal-tcp.xml";
    monitor->SerializeToXmlFile(flowmonFilename, true, true);
    NS_LOG_INFO("Flow monitor data saved to " << flowmonFilename);

    return 0;
}