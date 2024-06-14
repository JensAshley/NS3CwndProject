/*
 * Copyright (c) 2015, IMDEA Networks Institute
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Hany Assasa <hany.assasa@gmail.com>
.*
 * This is a simple example to test TCP over 802.11n (with MPDU aggregation enabled).
 *
 * Network topology:
 *
 *   Ap    STA
 *   *      *
 *   |      |
 *   n1     n2
 *
 * In this example, an HT station sends TCP packets to the access point.
 * We report the total throughput received during a window of 100ms.
 * The user can specify the application data rate and choose the variant
 * of TCP i.e. congestion control algorithm to use.
 */

#include "ns3/applications-module.h"
#include "ns3/callback.h"
#include "ns3/core-module.h"
#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/log.h"
#include "ns3/mobility-helper.h"
#include "ns3/mobility-model.h"
#include "ns3/on-off-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/packet-sink.h"
#include "ns3/ssid.h"
#include "ns3/string.h"
#include "ns3/tcp-westwood-plus.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/click-internet-stack-helper.h"
#include "ns3/network-module.h"
#include <iostream>

NS_LOG_COMPONENT_DEFINE("wifi-tcp");

using namespace ns3;
uint32_t newVal;


class MyApp : public Application  {
	public:

		MyApp();
		virtual ~MyApp();

		void Setup(Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);

	private:
		virtual void StartApplication(void);
		virtual void StopApplication(void);

		void ScheduleTx(void);
		void SendPacket(void);

		Ptr<Socket>     m_socket;
		Address         m_peer;
		uint32_t        m_packetSize;
		uint32_t        m_nPackets;
		DataRate        m_dataRate;
		EventId         m_sendEvent;
		bool            m_running;
		uint32_t        m_packetsSent;
		uint32_t        m_cwndVal;
};

MyApp::MyApp()
	: m_socket(0),
	m_peer(),
	m_packetSize(0),
	m_nPackets(0),
	m_dataRate(0),
	m_sendEvent(),
	m_running(false),
	m_packetsSent(0),
	m_cwndVal(0){
	}

MyApp::~MyApp() {
	m_socket = 0;
}

void
MyApp::Setup(Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate) {
	m_socket = socket;
	m_peer = address;
	m_packetSize = packetSize;
	m_nPackets = nPackets;
	m_dataRate = dataRate;
}

void
MyApp::StartApplication(void) {
	m_running = true;
	m_packetsSent = 0;
	m_socket->Bind();
	m_socket->Connect(m_peer);
	SendPacket();
}

void
MyApp::StopApplication(void) {
	m_running = false;
	if(m_sendEvent.IsRunning()) {
		Simulator::Cancel(m_sendEvent);
	}

	if(m_socket) {
		m_socket->Close();
	}
}

void
MyApp::SendPacket(void) {
    Ptr<Packet> packet = Create<Packet>(m_packetSize);
	m_cwndVal = newVal;
	/* Set cwnd to the value you want and the socket will change the TTL of the packet. Click will then filter out packers with TTL that aren't 64 (default value) causing a packet drop */
	if(m_cwndVal >= 200000){  
	    m_socket->SetIpTtl(65);
	    m_socket->Send(packet);
	}
	else{
	    m_socket->SetIpTtl(64);
	    m_socket->Send(packet);
	}
	
	
	
	
  
	if(++m_packetsSent < m_nPackets) {
		ScheduleTx();
	}
}

void
MyApp::ScheduleTx(void) {
	if(m_running) {
		Time tNext(Seconds(m_packetSize * 8 / static_cast<double>(m_dataRate.GetBitRate())));
		m_sendEvent = Simulator::Schedule(tNext, &MyApp::SendPacket, this);
	}
}



static void
CwndChange(uint32_t oldCwnd, uint32_t newCwnd) {
	NS_LOG_UNCOND(Simulator::Now().GetSeconds() << "\t" << newCwnd);
	newVal = newCwnd;
	//std::cout << newVal << std::endl;
}


Ptr<PacketSink> sink;     //!< Pointer to the packet sink application
uint64_t lastTotalRx = 0; //!< The value of the last total received bytes


/**
 * Calculate the throughput
 */
void
CalculateThroughput()
{
    Time now = Simulator::Now(); /* Return the simulator's virtual time. */
    double cur = (sink->GetTotalRx() - lastTotalRx) * 8.0 /
                 1e5; /* Convert Application RX Packets to MBits. */
    std::cout << now.GetSeconds() << "s: \t" << cur << " Mbit/s" << std::endl;
    lastTotalRx = sink->GetTotalRx();
    Simulator::Schedule(MilliSeconds(100), &CalculateThroughput);
}



int
main(int argc, char* argv[])
{
    uint32_t payloadSize = 1472;           /* Transport layer payload size in bytes. */
    std::string dataRate = "100Mbps";      /* Application layer datarate. */
    std::string tcpVariant = "TcpNewReno"; /* TCP variant type. */
    std::string phyRate = "HtMcs7";        /* Physical layer bitrate. */
    double simulationTime = 10;            /* Simulation time in seconds. */
    bool pcapTracing = true;              /* PCAP Tracing is enabled or not. */
    std::string clickConfigFolder = "src/click/examples";
    /* Command line argument parser setup. */
    CommandLine cmd(__FILE__);
    cmd.AddValue("payloadSize", "Payload size in bytes", payloadSize);
    cmd.AddValue("dataRate", "Application data ate", dataRate);
    cmd.AddValue("tcpVariant",
                 "Transport protocol to use: TcpNewReno, "
                 "TcpHybla, TcpHighSpeed, TcpHtcp, TcpVegas, TcpScalable, TcpVeno, "
                 "TcpBic, TcpYeah, TcpIllinois, TcpWestwood, TcpWestwoodPlus, TcpLedbat ",
                 tcpVariant);
    cmd.AddValue("phyRate", "Physical layer bitrate", phyRate);
    cmd.AddValue("simulationTime", "Simulation time in seconds", simulationTime);
    cmd.AddValue("pcap", "Enable/disable PCAP Tracing", pcapTracing);
    cmd.Parse(argc, argv);

    tcpVariant = std::string("ns3::") + tcpVariant;
    // Select TCP variant
    TypeId tcpTid;
    NS_ABORT_MSG_UNLESS(TypeId::LookupByNameFailSafe(tcpVariant, &tcpTid),
                        "TypeId " << tcpVariant << " not found");
    Config::SetDefault("ns3::TcpL4Protocol::SocketType",
                       TypeIdValue(TypeId::LookupByName(tcpVariant)));

    /* Configure TCP Options */
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(payloadSize));
    /* These two commands have an affect on the infinite growth Congestion window problem but not sure how to utilize it correctly as it seems like the results arent exacly what you wanted*/
    Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(10000000)); 
    Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(10000000));

    WifiMacHelper wifiMac;
    WifiHelper wifiHelper;
    wifiHelper.SetStandard(WIFI_STANDARD_80211n);

    /* Set up Legacy Channel */
    YansWifiChannelHelper wifiChannel;
    wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    wifiChannel.AddPropagationLoss("ns3::FriisPropagationLossModel", "Frequency", DoubleValue(5e9));

    /* Setup Physical Layer */
    YansWifiPhyHelper wifiPhy;
    wifiPhy.SetChannel(wifiChannel.Create());
    //wifiPhy.SetErrorRateModel("ns3::YansErrorRateModel");   
    wifiHelper.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                       "DataMode",
                                       StringValue(phyRate),
                                       "ControlMode",
                                       StringValue("HtMcs0"));

    NodeContainer networkNodes;
    networkNodes.Create(2);
    Ptr<Node> apWifiNode = networkNodes.Get(0);
    Ptr<Node> staWifiNode = networkNodes.Get(1);

    /* Configure AP */
    Ssid ssid = Ssid("network");
    wifiMac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));

    NetDeviceContainer apDevice;
    apDevice = wifiHelper.Install(wifiPhy, wifiMac, apWifiNode);

    /* Configure STA */
    wifiMac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid));

    NetDeviceContainer staDevices;
    staDevices = wifiHelper.Install(wifiPhy, wifiMac, staWifiNode);

    /* Mobility model */
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    positionAlloc->Add(Vector(5.0, 1.0, 0.0));

    mobility.SetPositionAllocator(positionAlloc);
        mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(apWifiNode);
    mobility.Install(staWifiNode);
    
    /* Sets up the Click Router on Station node */
    ClickInternetStackHelper click;
    click.SetClickFile(networkNodes.Get(1), clickConfigFolder + "/nsclick-wifi-single-interface.click");
    click.SetRoutingTableElement(networkNodes.Get(1), "rt");
    click.Install(networkNodes.Get(1));
    
    /* Internet stack */
    InternetStackHelper stack;
    stack.Install(networkNodes);

    Ipv4AddressHelper address;
    address.SetBase("172.16.1.0", "255.255.255.0");
    Ipv4InterfaceContainer apInterface;
    apInterface = address.Assign(apDevice);
    Ipv4InterfaceContainer staInterface;
    staInterface = address.Assign(staDevices);

    /* Populate routing table */
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    /* Install TCP Receiver on the access point */
    Address sinkAddress (InetSocketAddress (apInterface.GetAddress (0), 9));
    PacketSinkHelper sinkHelper("ns3::TcpSocketFactory",
    InetSocketAddress(Ipv4Address::GetAny(), 9));
    ApplicationContainer sinkApp = sinkHelper.Install(apWifiNode);
    sink = StaticCast<PacketSink>(sinkApp.Get(0));
    
    /* Setup Custom TCP sender app */
    Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (staWifiNode, TcpSocketFactory::GetTypeId ());
	ns3TcpSocket->TraceConnectWithoutContext ("CongestionWindow", MakeCallback (&CwndChange));
    Ptr<MyApp> app = CreateObject<MyApp> ();
	app->Setup (ns3TcpSocket, sinkAddress, payloadSize, 100000, DataRate (dataRate));
	staWifiNode->AddApplication (app);
    

    /* Start Applications */
    sinkApp.Start(Seconds(0.0));
    app->SetStartTime (Seconds (1.));
    app->SetStopTime (Seconds (simulationTime + 1));
    
    //Simulator::Schedule(Seconds(1.1), &CalculateThroughput);
    
    /* Enable Traces */
    if (pcapTracing)
    {
        wifiPhy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
        wifiPhy.EnablePcap("AccessPoint", apDevice);
        wifiPhy.EnablePcap("Station", staDevices);
    }

    /* Start Simulation */
    Simulator::Stop(Seconds(simulationTime + 1));
    Simulator::Run();
    //std::cout << newVal << std::endl;
    double averageThroughput = ((sink->GetTotalRx() * 8) / (1e6 * simulationTime));

    Simulator::Destroy();

    
    std::cout << "\nAverage throughput: " << averageThroughput << " Mbit/s" << std::endl;
    return 0;
}
