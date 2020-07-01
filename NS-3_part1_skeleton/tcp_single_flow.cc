/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 * TODO Update this header
 */

#include <fstream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TCPSingleFlow");

// ===========================================================================
//
//         node 0                 node 1
//   +----------------+    +----------------+
//   |    ns-3 TCP    |    |    ns-3 TCP    |
//   +----------------+    +----------------+
//   |    192.168.1.1    |    |    192.168.1.2    |
//   +----------------+    +----------------+
//   | point-to-point |    | point-to-point |
//   +----------------+    +----------------+
//           |                     |
//           +---------------------+
//                10 Mbps, X ms, Y errors
// ===========================================================================
//
class MyApp : public Application {
public:

    MyApp();

    virtual ~MyApp();

    void Setup(Ptr <Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);

private:
    virtual void StartApplication(void);

    virtual void StopApplication(void);

    void ScheduleTx(void);

    void SendPacket(void);

    Ptr <Socket> m_socket;
    Address m_peer;
    uint32_t m_packetSize;
    uint32_t m_nPackets;
    DataRate m_dataRate;
    EventId m_sendEvent;
    bool m_running;
    uint32_t m_packetsSent;
};

MyApp::MyApp()
        : m_socket(0),
          m_peer(),
          m_packetSize(0),
          m_nPackets(0),
          m_dataRate(0),
          m_sendEvent(),
          m_running(false),
          m_packetsSent(0) {
}

MyApp::~MyApp() {
    m_socket = 0;
}

void
MyApp::Setup(Ptr <Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate) {
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

    if (m_sendEvent.IsRunning()) {
        Simulator::Cancel(m_sendEvent);
    }

    if (m_socket) {
        m_socket->Close();
    }
}

void
MyApp::SendPacket(void) {
    Ptr <Packet> packet = Create<Packet>(m_packetSize);
    m_socket->Send(packet);

    if (m_nPackets == 0 || (++m_packetsSent < m_nPackets)) {
        ScheduleTx();
    }
}

void
MyApp::ScheduleTx(void) {
    if (m_running) {
        Time tNext(Seconds(m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate())));
        m_sendEvent = Simulator::Schedule(tNext, &MyApp::SendPacket, this);
    }
}

// Trace functions
static void
CwndChange(Ptr <OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd) {
    *stream->GetStream() << Simulator::Now().GetSeconds() << "\t" << oldCwnd << "\t" << newCwnd << std::endl;
}

static void
RxDrop(Ptr <OutputStreamWrapper> stream, Ptr<const Packet> p) {
    *stream->GetStream() << Simulator::Now().GetSeconds() << std::endl;
}

int
main(int argc, char *argv[]) {
    uint32_t run = 0;
    std::string experiment_name = "base";
    std::string tcp_variant = "TcpNewReno";

    CommandLine cmd;
    cmd.AddValue("expname", "Experiment name used as basename for trace files.", experiment_name);
    cmd.AddValue("run", "Run index (for setting repeatable seeds)", run);

    // < ---- TODO Bonus Task 7.1.3: set tcp variant, error rate and delay via command line ---- >

    cmd.Parse(argc, argv);

    SeedManager::SetSeed(1);
    SeedManager::SetRun(run);

    /* -------- TOPOLOGY -------- */
    // Create nodes
    NodeContainer nodes;
    nodes.Create(2);

    PointToPointHelper pointToPoint;
    // Set the Data rate and link delay
    // < ---- TODO  Task 7.1.1: Add topology ---- >

    // Create the link between the two nodes
    NetDeviceContainer devices;
    // < ---- TODO  Task 7.1.1: Add topology ---- >

    // Add failure model to the link
    Ptr <RateErrorModel> em = CreateObject<RateErrorModel>();
    // < ---- TODO  Task 7.1.1: Set error rate ---- >

    /* -------- END TOPOLOGY -------- */

    /* -------- IP and transport layer ------- */

    // Install IP addresses and set TCP version
    std::stringstream tcp_variant_fullname;
    tcp_variant_fullname << "ns3::" << tcp_variant;
    // < ---- TODO  Task 7.1.2: Set TCP version ---- >

    InternetStackHelper stack;
    stack.Install(nodes);

    // Set IP addresses
    Ipv4AddressHelper address;
    address.SetBase("192.168.1.0", "255.255.255.252");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    // Install transport layer
    uint16_t sinkPort = 8080;
    Address sinkAddress(InetSocketAddress(interfaces.GetAddress(1), sinkPort));
    PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), sinkPort));
    ApplicationContainer sinkApps = packetSinkHelper.Install(nodes.Get(1));
    sinkApps.Start(Seconds(0.));
    sinkApps.Stop(Seconds(30.));

    Ptr <Socket> ns3TcpSocket = Socket::CreateSocket(nodes.Get(0), TcpSocketFactory::GetTypeId());
    /* -------- END IP and transport layer ------- */

    /* -------- Application -------- */
    Ptr <MyApp> app = CreateObject<MyApp>();
    // < ---- TODO  Task 7.1.2: Setup the application ---- >
    nodes.Get(0)->AddApplication(app);

    app->SetStartTime(Seconds(1.));
    app->SetStopTime(Seconds(30.));

    /* -------- Traces and Output -------- */

    // Connect traces and write to file
    std::stringstream fname_cwnd;
    fname_cwnd << "single_tcp_" << experiment_name << ".cwnd";
    AsciiTraceHelper asciiTraceHelper;
    Ptr <OutputStreamWrapper> streamCwnd = asciiTraceHelper.CreateFileStream(fname_cwnd.str());

    // Connect the trace for the congestion window of the socket to method CwndChange
    // < ---- TODO  Task 7.1.2: Connect trace for congestion window ---- >

    std::stringstream fname_rxdrop;
    fname_rxdrop << "single_tcp_" << experiment_name << ".rxdrop";
    Ptr <OutputStreamWrapper> streamRxDrop = asciiTraceHelper.CreateFileStream(fname_rxdrop.str());
    // Connect the trace for the receive errors of the receiving NetDevice to method RxDrop
    // < ---- TODO  Task 7.1.2: Connect trace for packet drops ---- >

    // Run simulation
    // < ---- TODO  Task 7.1.4: Run the simulation ---- >
    Simulator::Destroy();

    return 0;
}

