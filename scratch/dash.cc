/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2016 Technische Universitaet Berlin
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
 */

// - TCP Stream server and user-defined number of clients connected with an AP
// - WiFi connection
// - Tracing of throughput, packet information is done in the client

#include <errno.h>
#include <ns3/buildings-module.h>
#include <ns3/config-store-module.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "ns3/applications-module.h"
#include "ns3/building-position-allocator.h"
#include "ns3/core-module.h"
#include "ns3/epc-helper.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/lte-helper.h"
#include "ns3/lte-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/object.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/point-to-point-module.h"

template <typename T>
std::string ToString(T val) {
  std::stringstream stream;
  stream << val;
  return stream.str();
}

using namespace ns3;
NS_LOG_COMPONENT_DEFINE("TcpStreamExample");

static void ACEvent(Ptr<ConstantAccelerationMobilityModel> cvmm, Vector &speed,
                    Vector &delta) {
  cvmm->SetVelocityAndAcceleration(speed, delta);
}
static void COEvent(Ptr<ConstantVelocityMobilityModel> cvmm, Vector &speed) {
  cvmm->SetVelocity(speed);
}

int main(int argc, char *argv[]) {
  LogComponentEnable("TcpStreamExample", LOG_LEVEL_INFO);
  LogComponentEnable("TcpStreamClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable("TcpStreamServerApplication", LOG_LEVEL_INFO);

  uint64_t segmentDuration = 10000000;
  uint32_t simulationId = 4;
  uint32_t numberOfClients = 20;  // limit by 40
  uint32_t numberOfEnbs = 1;      // 1
  uint32_t scenarioId = 0;
  std::string adaptationAlgo = "tobasco";  //
  std::string app_type = "Dash";           // Bulk sender | On-Off Sender | Dash
  double eNbTxPower = 43.0;                // 43
  int fading_model = 0;                    // 0 for etu, 1 for eva
  int load = 0;                            // 0 for low load, 1 for high load
  int rlc_mode = 3;                        // UM = 2; AM = 3
  int tx_mode = 2;
  int bandwidth = 75;
  std::string data_rate = "100Gbps";  // 100Gbps

  CommandLine cmd;
  cmd.Usage("Simulation of streaming with DASH.\n");
  cmd.AddValue("simulationId", "The simulation's index (for logging purposes)",
               simulationId);
  cmd.AddValue("numberOfClients", "The number of clients", numberOfClients);
  cmd.AddValue("numberOfEnbs", "The number of eNodeBs", numberOfEnbs);
  cmd.AddValue("scenarioId", "The Id of scenrio", scenarioId);
  cmd.AddValue("segmentDuration",
               "The duration of a video segment in microseconds",
               segmentDuration);
  cmd.AddValue("adaptationAlgo",
               "The adaptation algorithm that the client uses for the "
               "simulation[festive | tobasco | sara | tomato | "
               "constbitrateW/H/T/WH/C...]",
               adaptationAlgo);
  cmd.AddValue("app_type", "source model[Bulk | OnOff | Dash][defalt:Dash]",
               app_type);
  cmd.AddValue("eNbTxPower", "Tx Power of eNB(dBm)[default:43dBm]", eNbTxPower);
  cmd.AddValue("fading_model",
               "fading mode[0 = ETU3 | 1 = EVA60][default:EVA60]",
               fading_model);
  cmd.AddValue("load", "load scenario:[0 = low | 1 = high][deault:0]", load);
  cmd.AddValue("rlc_mode", "RLC mode[2 = UM | 3 = AM][default:3]", rlc_mode);
  cmd.AddValue("tx_mode",
               "TX mode[0 = SISO | 1 = MIMO_mode1 | 2=MIMO_mode2][default:0]",
               tx_mode);
  cmd.AddValue("BandWidth", "Dl bandwidth and Ul bandwidth[default=100]",
               bandwidth);
  cmd.AddValue("DataRate",
               "DataRate for PointToPoint(pgw->remoteHost)[Default=100Gbps]",
               data_rate);
  cmd.Parse(argc, argv);

  Config::SetDefault("ns3::LteSpectrumPhy::CtrlErrorModelEnabled",
                     BooleanValue(false));
  Config::SetDefault("ns3::LteSpectrumPhy::DataErrorModelEnabled",
                     BooleanValue(true));
  Config::SetDefault("ns3::LteEnbRrc::DefaultTransmissionMode",
                     UintegerValue(tx_mode));  // MIMO
  Config::SetDefault("ns3::LteEnbRrc::EpsBearerToRlcMapping",
                     EnumValue(rlc_mode));
  Config::SetDefault("ns3::LteEnbPhy::TxPower", DoubleValue(eNbTxPower));
  GlobalValue::Bind("ChecksumEnabled", BooleanValue(true));
  Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1446));
  Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(524288));
  Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(524288));

  ConfigStore input_config;
  input_config.ConfigureDefaults();
  cmd.Parse(argc, argv);

  Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();
  Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper>();
  lteHelper->SetEpcHelper(epcHelper);
  lteHelper->SetAttribute("FadingModel",
                          StringValue("ns3::TraceFadingLossModel"));
  lteHelper->SetAttribute("PathlossModel",
                          StringValue("ns3::Cost231PropagationLossModel"));
  std::ifstream ifTraceFile;
  std::string fading_trace_path;

  if (simulationId == 1) fading_model = 0;
  if (simulationId == 2) fading_model = 0;
  if (simulationId == 3) fading_model = 0;
  if (simulationId == 4) fading_model = 1;

  if (fading_model == 0)
    fading_trace_path =
        "../../src/lte/model/fading-traces/fading_trace_ETU_3kmph.fad";
  if (fading_model == 1)
    fading_trace_path =
        "../../src/lte/model/fading-traces/fading_trace_EVA_60kmph.fad";

  ifTraceFile.open(fading_trace_path.c_str(), std::ifstream::in);

  if (ifTraceFile.good())
    lteHelper->SetFadingModelAttribute("TraceFilename",
                                       StringValue(fading_trace_path.c_str()));
  else
    lteHelper->SetFadingModelAttribute(
        "TraceFilename", StringValue(fading_trace_path.substr(6).c_str()));

  lteHelper->SetFadingModelAttribute("TraceLength", TimeValue(Seconds(10.0)));
  lteHelper->SetFadingModelAttribute("SamplesNum", UintegerValue(10000));
  lteHelper->SetFadingModelAttribute("WindowSize", TimeValue(Seconds(0.5)));
  lteHelper->SetFadingModelAttribute("RbNum", UintegerValue(bandwidth));
  lteHelper->SetEnbDeviceAttribute("DlBandwidth", UintegerValue(bandwidth));
  lteHelper->SetEnbDeviceAttribute("UlBandwidth", UintegerValue(bandwidth));
  lteHelper->SetSchedulerType("ns3::PfFfMacScheduler");

  Ptr<Node> pgw = epcHelper->GetPgwNode();
  std::cout << "pgw Id:  " << pgw->GetId() << std::endl;
  NodeContainer remote_host_container;
  remote_host_container.Create(1);
  Ptr<Node> remote_host = remote_host_container.Get(0);
  InternetStackHelper internet;
  internet.Install(remote_host_container);
  std::cout << "remoteHost Id:  " << remote_host->GetId() << std::endl;
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute("DataRate", DataRateValue(DataRate(data_rate)));
  p2ph.SetDeviceAttribute("Mtu", UintegerValue(1500));
  p2ph.SetChannelAttribute("Delay", TimeValue(Seconds(0.001)));
  NetDeviceContainer internetDevices = p2ph.Install(pgw, remote_host);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase("1.0.0.0", "255.0.0.0");
  ipv4h.SetBase("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign(internetDevices);
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remote_host_static_routing =
      ipv4RoutingHelper.GetStaticRouting(remote_host->GetObject<Ipv4>());
  remote_host_static_routing->AddNetworkRouteTo(Ipv4Address("7.0.0.0"),
                                                Ipv4Mask("255.0.0.0"), 1);
  NodeContainer eNb_nodes;
  NodeContainer ue_nodes;
  Ipv4InterfaceContainer ueIpIface;
  ue_nodes.Create(numberOfClients);
  eNb_nodes.Create(numberOfEnbs);
  MobilityHelper enbMobility;
  Ptr<ListPositionAllocator> positionAlloc_eNB =
      CreateObject<ListPositionAllocator>();

  positionAlloc_eNB->Add(Vector(0, 0, 0));  // eNB_0

  enbMobility.SetPositionAllocator(positionAlloc_eNB);
  enbMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  enbMobility.Install(eNb_nodes);

  // create folder
  std::string dir = "mylogs/";
  std::string subdir = dir + adaptationAlgo + "/";
  std::string ssubdir = subdir + ToString(numberOfClients) + "/";
  const char *mylogsDir = (dir).c_str();
  mkdir(mylogsDir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  const char *algoDir = (subdir).c_str();
  mkdir(algoDir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  const char *logdir = (ssubdir).c_str();
  mkdir(logdir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

  std::ofstream clientPosLog;
  std::string clientPos =
      ssubdir + "sim" + ToString(simulationId) + "_" + "clientPos.txt";
  clientPosLog.open(clientPos.c_str());
  NS_ASSERT_MSG(clientPosLog.is_open(), "Couldn't open clientPosLog file");

  switch (simulationId) {
    case 1: {  //固定位置
      Ptr<ListPositionAllocator> positionAlloc =
          CreateObject<ListPositionAllocator>();
      Ptr<RandomDiscPositionAllocator> randPosAlloc =
          CreateObject<RandomDiscPositionAllocator>();
      for (uint i = 0; i < numberOfClients; i++) {
        Vector pos = Vector(30, 25, 0);
        positionAlloc->Add(pos);

        clientPosLog << ToString(pos.x) << ", " << ToString(pos.y) << ", "
                     << ToString(pos.z) << "\n";
        clientPosLog.flush();
      }
      MobilityHelper ueMobility_1;
      ueMobility_1.SetPositionAllocator(positionAlloc);
      ueMobility_1.SetMobilityModel("ns3::ConstantPositionMobilityModel");
      ueMobility_1.Install(ue_nodes);
      break;
    }
    case 2: {  //隨機走動
      MobilityHelper ueMobility_2;
      ueMobility_2.SetMobilityModel(
          "ns3::RandomWalk2dMobilityModel", "Mode", StringValue("Time"), "Time",
          StringValue("1s"), "Speed",
          StringValue("ns3::ConstantRandomVariable[Constant=0.83333]"),
          "Bounds", RectangleValue(Rectangle(40, 20, 40, 20)));
      for (uint i = 0; i < numberOfClients; i++) {
        ueMobility_2.Install(ue_nodes.Get(i));
      }
      break;
    }
    case 3: {  //步行穿小區
      MobilityHelper ueMobility_4;
      ueMobility_4.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
      ueMobility_4.SetPositionAllocator(
          "ns3::UniformDiscPositionAllocator", "X", DoubleValue(-90.0), "Y",
          DoubleValue(-10), "rho", DoubleValue(0));
      ueMobility_4.Install(ue_nodes);
      for (int64_t i = 0; i < ue_nodes.GetN(); i++) {
        Ptr<ConstantVelocityMobilityModel> cvmm =
            ue_nodes.Get(i)->GetObject<ConstantVelocityMobilityModel>();
        cvmm->SetVelocity(Vector(0.833, 0.0, 0.0));
        Simulator::Schedule(Seconds(180), &COEvent, cvmm,
                            Vector(-0.833, 0.0, 0.0));
      }
      break;
    }
    case 4: {  //車載穿小區
      MobilityHelper ueMobility_5;
      ueMobility_5.SetMobilityModel("ns3::ConstantAccelerationMobilityModel");
      ueMobility_5.SetPositionAllocator(
          "ns3::UniformDiscPositionAllocator", "X", DoubleValue(-90.0), "Y",
          DoubleValue(0.0), "rho", DoubleValue(0));
      ueMobility_5.Install(ue_nodes);
      for (int64_t i = 0; i < ue_nodes.GetN(); i++) {
        Ptr<ConstantAccelerationMobilityModel> cvmm =
            ue_nodes.Get(i)->GetObject<ConstantAccelerationMobilityModel>();
        if (scenarioId == 0) {
          cvmm->SetVelocityAndAcceleration(Vector(45, 0, 0), Vector(0, 0, 0));
          Simulator::Schedule(Seconds(2.0), &ACEvent, cvmm, Vector(0, 0, 0),
                              Vector(0, 0, 0));
        } else if (scenarioId == 6) {
          cvmm->SetVelocityAndAcceleration(Vector(6, 0, 0), Vector(0, 0, 0));
          Simulator::Schedule(Seconds(30.0), &ACEvent, cvmm, Vector(-6, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(60.0), &ACEvent, cvmm, Vector(6, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(90.0), &ACEvent, cvmm, Vector(-6, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(120.0), &ACEvent, cvmm, Vector(6, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(150.0), &ACEvent, cvmm, Vector(-6, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(180.0), &ACEvent, cvmm, Vector(6, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(210.0), &ACEvent, cvmm, Vector(-6, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(240.0), &ACEvent, cvmm, Vector(6, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(270.0), &ACEvent, cvmm, Vector(-6, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(300.0), &ACEvent, cvmm, Vector(6, 0, 0),
                              Vector(0, 0, 0));
        } else if (scenarioId == 12) {
          cvmm->SetVelocityAndAcceleration(Vector(12, 0, 0), Vector(0, 0, 0));
          Simulator::Schedule(Seconds(15), &ACEvent, cvmm, Vector(-12, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(30), &ACEvent, cvmm, Vector(12, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(45), &ACEvent, cvmm, Vector(-12, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(60), &ACEvent, cvmm, Vector(12, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(75), &ACEvent, cvmm, Vector(-12, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(90), &ACEvent, cvmm, Vector(12, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(105), &ACEvent, cvmm, Vector(-12, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(120), &ACEvent, cvmm, Vector(12, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(135), &ACEvent, cvmm, Vector(-12, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(150), &ACEvent, cvmm, Vector(12, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(165), &ACEvent, cvmm, Vector(-12, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(180), &ACEvent, cvmm, Vector(-12, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(195), &ACEvent, cvmm, Vector(12, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(210), &ACEvent, cvmm, Vector(-12, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(225), &ACEvent, cvmm, Vector(12, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(240), &ACEvent, cvmm, Vector(-12, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(255), &ACEvent, cvmm, Vector(12, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(270), &ACEvent, cvmm, Vector(-12, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(285), &ACEvent, cvmm, Vector(12, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(300), &ACEvent, cvmm, Vector(-12, 0, 0),
                              Vector(0, 0, 0));
        } else if (scenarioId == 18) {
          cvmm->SetVelocityAndAcceleration(Vector(18, 0, 0), Vector(0, 0, 0));
          Simulator::Schedule(Seconds(10), &ACEvent, cvmm, Vector(-18, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(20), &ACEvent, cvmm, Vector(18, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(30), &ACEvent, cvmm, Vector(-18, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(40), &ACEvent, cvmm, Vector(18, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(50), &ACEvent, cvmm, Vector(-18, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(60), &ACEvent, cvmm, Vector(18, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(70), &ACEvent, cvmm, Vector(-18, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(80), &ACEvent, cvmm, Vector(18, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(90), &ACEvent, cvmm, Vector(-18, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(100), &ACEvent, cvmm, Vector(18, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(110), &ACEvent, cvmm, Vector(-18, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(120), &ACEvent, cvmm, Vector(-18, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(130), &ACEvent, cvmm, Vector(18, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(140), &ACEvent, cvmm, Vector(-18, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(150), &ACEvent, cvmm, Vector(18, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(160), &ACEvent, cvmm, Vector(-18, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(170), &ACEvent, cvmm, Vector(18, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(180), &ACEvent, cvmm, Vector(-18, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(190), &ACEvent, cvmm, Vector(18, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(200), &ACEvent, cvmm, Vector(-18, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(210), &ACEvent, cvmm, Vector(18, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(220), &ACEvent, cvmm, Vector(-18, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(230), &ACEvent, cvmm, Vector(-18, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(240), &ACEvent, cvmm, Vector(18, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(250), &ACEvent, cvmm, Vector(-18, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(260), &ACEvent, cvmm, Vector(18, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(270), &ACEvent, cvmm, Vector(-18, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(280), &ACEvent, cvmm, Vector(18, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(290), &ACEvent, cvmm, Vector(-18, 0, 0),
                              Vector(0, 0, 0));
          Simulator::Schedule(Seconds(300), &ACEvent, cvmm, Vector(18, 0, 0),
                              Vector(0, 0, 0));
        } else {
          NS_ASSERT_MSG(scenarioId == 6 || scenarioId == 12 ||
                            scenarioId == 18 || scenarioId == 0,
                        "Invalid speed!");
        }
      }
      break;
    }
  }

  NetDeviceContainer eNb_devs = lteHelper->InstallEnbDevice(eNb_nodes);
  NetDeviceContainer ue_devs = lteHelper->InstallUeDevice(ue_nodes);

  internet.Install(ue_nodes);
  ueIpIface = epcHelper->AssignUeIpv4Address(NetDeviceContainer(ue_devs));

  lteHelper->AttachToClosestEnb(ue_devs, eNb_devs);

  for (uint32_t i = 0; i < ue_nodes.GetN(); i++) {
    Ptr<Node> uenode = ue_nodes.Get(i);
    Ptr<Ipv4StaticRouting> ue_static_routing =
        ipv4RoutingHelper.GetStaticRouting(uenode->GetObject<Ipv4>());
    ue_static_routing->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(),
                                       1);
  }
  lteHelper->EnableTraces();

  std::vector<std::pair<Ptr<Node>, std::string>> clients;
  for (NodeContainer::Iterator i = ue_nodes.Begin(); i != ue_nodes.End(); ++i) {
    std::pair<Ptr<Node>, std::string> client(*i, adaptationAlgo);
    clients.push_back(client);
  }
  if (app_type.compare("Dash") == 0) {
    const Ptr<PhyRxStatsCalculator> lte_phy_rx_stats =
        lteHelper->GetPhyRxStats();
    uint16_t port = 80;
    TcpStreamServerHelper serverHelper(port);
    ApplicationContainer serverApp =
        serverHelper.Install(remote_host_container.Get(0));
    serverApp.Start(Seconds(1.0));

    TcpStreamClientHelper clientHelper(internetIpIfaces.GetAddress(1), port,
                                       lte_phy_rx_stats);
    clientHelper.SetAttribute("SegmentDuration",
                              UintegerValue(segmentDuration));
    clientHelper.SetAttribute("NumberOfClients",
                              UintegerValue(numberOfClients));
    clientHelper.SetAttribute("SimulationId", UintegerValue(simulationId));

    ApplicationContainer clientApps = clientHelper.Install(clients);
    clientApps.Get(0)->SetStartTime(Seconds(2));
    clientApps.Get(0)->SetStopTime(Seconds(302));
    
    int *clientsNum;
    int numOfeNbs = 0;
    int intervaleNb = 0;
    int clientsNum_0[7] = {2,4,1,7,6,2,9};//25
    int intervalNum_0[7] = {0};
    int clientsNum_6[3] = {1,6,3};//8
    int clientsNum_12[6] = {2,4,1,6,8,3};//19
    int clientsNum_18[9] = {2,1,3,7,9,3,5,1,7};//30
    if (scenarioId == 6){
      clientsNum = clientsNum_6;
      numOfeNbs = 3;
      intervaleNb = 30;
    }else if(scenarioId == 12){
      clientsNum = clientsNum_12;
      numOfeNbs = 6;
      intervaleNb = 15;
    }else if(scenarioId == 18){
      clientsNum = clientsNum_18;
      numOfeNbs = 9;
      intervaleNb = 10;
    }else{
      for(int m=0;m<7;m++){
        intervalNum_0[m] = rand() % 15 + 5;
      }
      clientsNum = clientsNum_0;
      numOfeNbs = 0;
    }
    int client_id = 1;
    std::cout <<"scenarioId: " <<scenarioId<<std::endl;
    if (numOfeNbs != 0){
      //std::cout << "wrong!!!"<<std::endl;
      for(int i=0;i<numOfeNbs;i++){
        for (int j=0;j<clientsNum[i]-1;j++){
          if (i==0){
          clientApps.Get(client_id)->SetStartTime(Seconds(2));
          clientApps.Get(client_id)->SetStopTime(Seconds(intervaleNb));
          std::cout << client_id << "start at:" << 2 << "\t"
                  << "end at:" << intervaleNb
                  << std::endl;
          client_id++;
          }else{
          clientApps.Get(client_id)->SetStartTime(Seconds(i*intervaleNb));
          clientApps.Get(client_id)->SetStopTime(Seconds(i*intervaleNb+intervaleNb));
          std::cout << client_id << "start at:" << i*intervaleNb<< "\t"
                  << "end at:" << i*intervaleNb+intervaleNb
                  << std::endl;
          client_id++;
          }
        }
      }
    }else{
       int interTemp = 2;
       for(int i=0;i<7;i++){
        for (int j=0;j<clientsNum[i]-1;j++){
          if (i==0){
          clientApps.Get(client_id)->SetStartTime(Seconds(2));
          clientApps.Get(client_id)->SetStopTime(Seconds(2+intervalNum_0[0]));
          std::cout << client_id << "start at:" << 2 << "\t"
                  << "end at:" << 2+intervalNum_0[0]
                  << std::endl;
          client_id++;
          }else{
          clientApps.Get(client_id)->SetStartTime(Seconds(interTemp));
          clientApps.Get(client_id)->SetStopTime(Seconds(interTemp + intervalNum_0[i]));
          std::cout << client_id << "start at:" <<interTemp<< "\t"
                  << "end at:" << interTemp + intervalNum_0[i]
                  << std::endl;
          client_id++;
          }
        }
        interTemp = interTemp + intervalNum_0[i];
      }
    }
    /*==============================================================================
    if (scenarioId == 6) {
      srand((unsigned)time(NULL));
      const int eNbs = 3;
      std::vector<int> temp(15, 0);
      std::vector<std::vector<int>> randommap;
      for (int i = 0; i < eNbs; ++i) {
        int random = rand() % 15;
        for (int i = 0; i < random; ++i) {
          temp.at(i) = 1;
        }
        randommap.push_back(temp);
        for (auto &item : temp) {
          item = 0;
        }
      }
      for (int i = 0; i < 15; ++i) {
        for (int j = 0; j < eNbs; ++j) {
          std::cout << randommap[j][15 - i - 1] << " ";
        }
        std::cout << std::endl;
      }
      std::vector<std::pair<int, int>> timestamp;
      for (int col = 0; col < 15; ++col) {
        int row = 0;
        while (row < eNbs) {
          while (row < eNbs && randommap[row][col] == 0) {
            ++row;
          }
          int begin = row;
          while (row < eNbs && randommap[row][col] == 1) {
            ++row;
          }
          int end = row - 1;
          if (end >= begin) {
            timestamp.push_back(std::make_pair(begin, end + 1));
          }
        }
      }
      for (uint i = 1; i <= timestamp.size(); ++i) {
        clientApps.Get(i)->SetStartTime(
            Seconds(2 + timestamp.at(i - 1).first * 30));
        clientApps.Get(i)->SetStopTime(
            Seconds(2 + timestamp.at(i - 1).second * 30));
        std::cout << "start at:" << 2 + timestamp.at(i - 1).first * 30 << "\t"
                  << "end at:" << 2 + timestamp.at(i - 1).second * 30
                  << std::endl;
      }
    } else if (scenarioId == 12) {
      srand((unsigned)time(NULL));
      const int eNbs = 6;
      std::vector<int> temp(15, 0);
      std::vector<std::vector<int>> randommap;
      for (int i = 0; i < eNbs; ++i) {
        int random = rand() % 15;
        for (int i = 0; i < random; ++i) {
          temp.at(i) = 1;
        }
        randommap.push_back(temp);
        for (auto &item : temp) {
          item = 0;
        }
      }
      for (int i = 0; i < 15; ++i) {
        for (int j = 0; j < eNbs; ++j) {
          std::cout << randommap[j][15 - i - 1] << " ";
        }
        std::cout << std::endl;
      }
      std::vector<std::pair<int, int>> timestamp;
      for (int col = 0; col < 15; ++col) {
        int row = 0;
        while (row < eNbs) {
          while (row < eNbs && randommap[row][col] == 0) {
            ++row;
          }
          int begin = row;
          while (row < eNbs && randommap[row][col] == 1) {
            ++row;
          }
          int end = row - 1;
          if (end >= begin) {
            timestamp.push_back(std::make_pair(begin, end + 1));
          }
        }
      }
      for (uint i = 1; i <= timestamp.size(); ++i) {
        clientApps.Get(i)->SetStartTime(
            Seconds(2 + timestamp.at(i - 1).first * 15));
        clientApps.Get(i)->SetStopTime(
            Seconds(2 + timestamp.at(i - 1).second * 15));
        std::cout << "start at:" << 2 + timestamp.at(i - 1).first * 15 << "\t"
                  << "end at:" << 2 + timestamp.at(i - 1).second * 15
                  << std::endl;
      }
    } else if (scenarioId == 18) {
      srand((unsigned)time(NULL));
      const int eNbs = 9;
      std::vector<int> temp(15, 0);
      std::vector<std::vector<int>> randommap;
      for (int i = 0; i < eNbs; ++i) {
        int random = rand() % 15;
        for (int i = 0; i < random; ++i) {
          temp.at(i) = 1;
        }
        randommap.push_back(temp);
        for (auto &item : temp) {
          item = 0;
        }
      }
      for (int i = 0; i < 15; ++i) {
        for (int j = 0; j < eNbs; ++j) {
          std::cout << randommap[j][15 - i - 1] << " ";
        }
        std::cout << std::endl;
      }
      std::vector<std::pair<int, int>> timestamp;
      for (int col = 0; col < 15; ++col) {
        int row = 0;
        while (row < eNbs) {
          while (row < eNbs && randommap[row][col] == 0) {
            ++row;
          }
          int begin = row;
          while (row < eNbs && randommap[row][col] == 1) {
            ++row;
          }
          int end = row - 1;
          if (end >= begin) {
            timestamp.push_back(std::make_pair(begin, end + 1));
          }
        }
      }
      for (uint i = 1; i <= timestamp.size(); ++i) {
        clientApps.Get(i)->SetStartTime(
            Seconds(2 + timestamp.at(i - 1).first * 10));
        clientApps.Get(i)->SetStopTime(
            Seconds(2 + timestamp.at(i - 1).second * 10));
        std::cout << "start at:" << 2 + timestamp.at(i - 1).first * 10 << "\t"
                  << "end at:" << 2 + timestamp.at(i - 1).second * 10
                  << std::endl;
      }
    } else if (scenarioId == 0) {  // speed == 0
      srand((unsigned)time(NULL));
      const int eNbs = 9;
      std::vector<int> temp(15, 0);
      std::vector<std::vector<int>> randommap;
      for (int i = 0; i < eNbs; ++i) {
        int random = rand() % 15;
        for (int i = 0; i < random; ++i) {
          temp.at(i) = 1;
        }
        randommap.push_back(temp);
        for (auto &item : temp) {
          item = 0;
        }
      }
      for (int i = 0; i < 15; ++i) {
        for (int j = 0; j < eNbs; ++j) {
          std::cout << randommap[j][15 - i - 1] << " ";
        }
        std::cout << std::endl;
      }
      std::vector<std::pair<int, int>> timestamp;
      for (int col = 0; col < 15; ++col) {
        int row = 0;
        while (row < eNbs) {
          while (row < eNbs && randommap[row][col] == 0) {
            ++row;
          }
          int begin = row;
          while (row < eNbs && randommap[row][col] == 1) {
            ++row;
          }
          int end = row - 1;
          if (end >= begin) {
            timestamp.push_back(std::make_pair(begin, end + 1));
          }
        }
      }
      for (uint i = 1; i <= timestamp.size(); ++i) {
        clientApps.Get(i)->SetStartTime(
            Seconds(2 + timestamp.at(i - 1).first * 10));
        clientApps.Get(i)->SetStopTime(
            Seconds(2 + timestamp.at(i - 1).second * 10));
        std::cout << "start at:" << 2 + timestamp.at(i - 1).first * 10 << "\t"
                  << "end at:" << 2 + timestamp.at(i - 1).second * 10
                  << std::endl;
      }
    } else {
      NS_ASSERT_MSG(scenarioId == 6 || scenarioId == 12 || scenarioId == 18 ||
                        scenarioId == 0,
                    "Invalid speed!");
    }
    ===========================================================================*/
    NS_LOG_INFO("Run Simulation.");
    NS_LOG_INFO("Sim:   " << simulationId
                          << "   Clients:   " << numberOfClients);
    Simulator::Stop(Seconds(121));
    Simulator::Run();
    Simulator::Destroy();
    NS_LOG_INFO("Done.");
  }
  return 0;
}
