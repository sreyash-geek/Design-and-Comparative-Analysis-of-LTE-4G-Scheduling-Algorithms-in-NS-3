/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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
 * Author: Manuel Requena <manuel.requena@cttc.es>
 *         Nicola Baldo <nbaldo@cttc.es>
 */

#include "ns3/config-store.h"
#include "ns3/core-module.h"
#include "ns3/lte-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/radio-environment-map-helper.h"
#include "ns3/channel-list.h"
#include "ns3/radio-bearer-stats-calculator.h"
#include "ns3/node-container.h"
#include "ns3/random-walk-2d-mobility-model.h"
#include "ns3/config-store.h"
#include <iomanip>
#include <string>
#include <fstream>
#include "ns3/lte-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/gnuplot.h"



using namespace ns3;

std::ofstream stream;
std::vector<double> timeV;
std::vector<double> th; 
uint32_t scheType = 0;
double speed = 0;

std::string Scheduler_Name(uint32_t scheType)
{
    if(scheType == 0){
        return "Proportional Fair";
    }
    if(scheType == 1){
        return "Round Robin";
    }
    if(scheType == 2){
        return "Max Throughput";
    }
    if(scheType == 3){
        return "BET";
    }   
    else{
        std::cout<<"Unable to identify scheduler";
        exit(1);
    } 
}

void stats(FlowMonitorHelper &f_help, bool info)
{
    std::string p;
    Ptr<Ipv4FlowClassifier> clf = DynamicCast<Ipv4FlowClassifier>(f_help.GetClassifier());	
	Ptr<FlowMonitor> mon = f_help.GetMonitor ();
	std::map < FlowId, FlowMonitor::FlowStats > stats = mon->GetFlowStats();
	double t_TimeReceiving;
	uint64_t t_Received, t_Dropped, t_BytesReceived;

    t_Received = 0, t_Dropped = 0, t_BytesReceived = 0; t_TimeReceiving = 0;
	for (std::map< FlowId, FlowMonitor::FlowStats>::iterator f = stats.begin(); f != stats.end(); f++)
	{
        t_Received += f->second.rxPackets;
	    t_Dropped += f->second.txPackets - f->second.rxPackets;
        t_BytesReceived += (double) f->second.rxBytes * 8;
	    t_TimeReceiving += f->second.timeLastRxPacket.GetSeconds ();
	    
	    Ipv4FlowClassifier::FiveTuple  t = clf->FindFlow(f->first);
	    
	    if (info) {
	        std::cout << "FlowID: " << f->first << " ( " << t.sourceAddress << ":" << t.sourcePort << " to "<< t.destinationAddress << ":" << t.destinationPort << ") " << std::endl;
	        std::cout << "Tx Bytes: " << f->second.txBytes << std::endl;
	        std::cout << "Rx Bytes: " << f->second.rxBytes << std::endl;
	        std::cout << "Time LastRxPacket: " << f->second.timeLastRxPacket.GetSeconds () << "s" << std::endl;
	        std::cout << "Packet Lost Ratio: " << ((double)f->second.txPackets-(double)f->second.rxPackets)/(double)f->second.txPackets << std::endl;
	        std::cout << "Throughput: " << ( ((double)f->second.rxBytes*8) / (f->second.timeLastRxPacket.GetSeconds ())/ 1000000.0 ) << "Mbps" << std::endl;
	    }

        th.push_back( ( ((double)f->second.rxBytes*8) / (f->second.timeLastRxPacket.GetSeconds ()) ));
	   }
    stream<< Scheduler_Name(scheType)  << ", " << speed << ", " << t_BytesReceived / t_TimeReceiving << "\n";
}

void CdfThroughputTrace()
{
    std::sort (th.begin (), th.end ());
    // Calculate the CDF and add it to the dataset
    std::ofstream stream;
    stream.open("cdfThroughput.csv", std::ios_base::app);
    double cdf = 0.0;
    for (double & throughput : th)
     {
       cdf += 1.0 / th.size ();
       stream << Scheduler_Name(scheType) << ", " << throughput <<", " << cdf << "\n";
     }
}
// A function to calculate the throughput and save it to the vector
void CalculateThroughput (FlowMonitorHelper *fh)
{
    Ptr<Ipv4FlowClassifier> cls = DynamicCast<Ipv4FlowClassifier>(fh->GetClassifier());
    Ptr<FlowMonitor> mon = fh->GetMonitor ();
    // Get a map of flow ID and flow statistics
    std::map<FlowId, FlowMonitor::FlowStats> stats = mon->GetFlowStats ();

    // Loop through the flows and calculate their throughput
  
    for(std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin ();  iter != stats.end (); ++iter){    
        FlowId id = iter->first;
        FlowMonitor::FlowStats st = iter->second;
        Ipv4FlowClassifier::FiveTuple t = cls->FindFlow (id);
        std::cout<<"bytes"<<st.rxBytes<<std::endl;
        double throughput = (st.rxBytes * 8.0) / (st.timeLastRxPacket.GetSeconds () - st.timeFirstRxPacket.GetSeconds ()) / 1000000.0;
        double now = Simulator::Now().GetSeconds();
        std::cout << now << "s: Flow " << id << " (" << t.sourceAddress << ":" << t.sourcePort << " to " << t.destinationAddress << ":" << t.destinationPort << ")" << std::endl;
        std::cout << "Throughput: " << throughput << " Mbps" << std::endl;        
        th.push_back (throughput);
        timeV.push_back(now);
        // break;
        
    }

    Simulator::Schedule (MilliSeconds(500), &CalculateThroughput, fh);

}

int
main(int argc, char* argv[])
{
    double enbDist = 1000.0;
    uint32_t numUes = 5;
    // double simTime = 10.0;
    uint32_t seeds = 21;
    bool fullBufferFlag;

    CommandLine cmd(__FILE__);
    cmd.AddValue("scheType", "Scheduler to be used (PF=0,RR=1,MT=2,BATS=3)", scheType);
    cmd.AddValue("speed", "speed of the UE's", speed);
    cmd.AddValue("RngRun", "seed value", seeds);
    cmd.AddValue("fullBufferFlag", "specify if the buffer is full or not", fullBufferFlag);
    cmd.Parse(argc, argv);

    ConfigStore inputConfig;
    inputConfig.ConfigureDefaults();

    // parse again so you can override default values from the command line
    cmd.Parse(argc, argv);

    // determine the string tag that identifies this simulation run
    // this tag is then appended to all filenames

    UintegerValue runValue;
    GlobalValue::GetValueByName("RngRun", runValue);
    RngSeedManager::SetSeed(seeds);

    std::string schedulerM; 
    
    if(scheType == 0){
        schedulerM = "ns3::PfFfMacScheduler";
    }
    if(scheType == 1){
        schedulerM = "ns3::RrFfMacScheduler";
    }
    if(scheType == 2){
        schedulerM = "ns3::FdMtFfMacScheduler";
    }
    if(scheType == 3){
        schedulerM = "ns3::TdBetFfMacScheduler";
    }
   
    // Create Nodes: eNodeB and UE
    NodeContainer enbNodes;
    enbNodes.Create(4);
    
    NodeContainer ueNodes;
    ueNodes.Create(4*numUes);

    NodeContainer ueNodes1;
    NodeContainer ueNodes2;
    NodeContainer ueNodes3;
    NodeContainer ueNodes4;

    for (int i = 0; i < 5; i++) {
        ueNodes1.Add(ueNodes.Get(i));
        ueNodes2.Add(ueNodes.Get(i+5));
        ueNodes3.Add(ueNodes.Get(i+10));
        ueNodes4.Add(ueNodes.Get(i+15));
    }

    

    // Position of eNBs
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    positionAlloc->Add(Vector(enbDist, 0.0, 0.0));
    positionAlloc->Add(Vector(0.0, enbDist, 0.0));
    positionAlloc->Add(Vector(enbDist, enbDist, 0.0));
    
    MobilityHelper enbMobility;
    enbMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    enbMobility.SetPositionAllocator(positionAlloc);
    enbMobility.Install(enbNodes);

    ObjectFactory positionAllocator;
    Ptr<PositionAllocator> positionAlloc2;
    // Position of UEs attached to eNB 1
    positionAllocator.SetTypeId("ns3::RandomDiscPositionAllocator");
    positionAllocator.Set("X", DoubleValue(0.0));
    positionAllocator.Set("Y", DoubleValue(0.0));
    positionAllocator.Set("Rho", StringValue("ns3::UniformRandomVariable[Min=0|Max=500]"));
    
    positionAlloc2 = positionAllocator.Create()->GetObject<PositionAllocator>();
    enbMobility.SetPositionAllocator(positionAlloc2);
    
    enbMobility.Install(ueNodes1);

    positionAllocator.SetTypeId("ns3::RandomDiscPositionAllocator");
    positionAllocator.Set("X", DoubleValue(enbDist));
    positionAllocator.Set("Y", DoubleValue(0.0));
    positionAllocator.Set("Rho", StringValue("ns3::UniformRandomVariable[Min=0|Max=500]"));
    
    positionAlloc2 = positionAllocator.Create()->GetObject<PositionAllocator>();
    enbMobility.SetPositionAllocator(positionAlloc2);
    
    enbMobility.Install(ueNodes2);
    // Position of UEs attached to eNB 3
    positionAllocator.SetTypeId("ns3::RandomDiscPositionAllocator");
    positionAllocator.Set("X", DoubleValue(0.0));
    positionAllocator.Set("Y", DoubleValue(enbDist));
    positionAllocator.Set("Rho", StringValue("ns3::UniformRandomVariable[Min=0|Max=500]"));
    
    positionAlloc2 = positionAllocator.Create()->GetObject<PositionAllocator>();
    enbMobility.SetPositionAllocator(positionAlloc2);
    
    enbMobility.Install(ueNodes3);
    
    // Position of UEs attached to eNB 4
    positionAllocator.SetTypeId("ns3::RandomDiscPositionAllocator");
    positionAllocator.Set("X", DoubleValue(enbDist));
    positionAllocator.Set("Y", DoubleValue(enbDist));
    positionAllocator.Set("Rho", StringValue("ns3::UniformRandomVariable[Min=0|Max=500]"));
    
    positionAlloc2 = positionAllocator.Create()->GetObject<PositionAllocator>();
    enbMobility.SetPositionAllocator(positionAlloc2);
    
    enbMobility.Install(ueNodes4);
    
    
    
    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();
    Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> ();
    lteHelper->SetEpcHelper (epcHelper);

    //Setting Pathlossmodel,handover algorithm and scheduler
    lteHelper->SetAttribute("PathlossModel", StringValue("ns3::FriisSpectrumPropagationLossModel"));
    lteHelper->SetHandoverAlgorithmType ("ns3::A3RsrpHandoverAlgorithm");
    lteHelper->SetSchedulerType(schedulerM);


    //Create pgw
    Ptr<Node> pgw = epcHelper->GetPgwNode();

    //Create a remote host
    NodeContainer rh;
    rh.Create(1);
    Ptr<Node> rhp = rh.Get (0);
    InternetStackHelper internet;
    internet.Install(rh);


    // internet.Install (ueNodes);

    //Connecting pgw and remote host and assigning ip's
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("1Gb/s")));
    p2p.SetDeviceAttribute ("Mtu", UintegerValue (1500));
    p2p.SetChannelAttribute ("Delay", TimeValue (Seconds (0.001)));
    NetDeviceContainer id = p2p.Install(pgw, rhp);

    Ipv4AddressHelper ipv4h;
    ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (id);

    Ipv4StaticRoutingHelper ipv4rout;
    Ptr<Ipv4StaticRouting> rhStaticRouting = ipv4rout.GetStaticRouting (rhp->GetObject<Ipv4> ());
    rhStaticRouting->AddNetworkRouteTo(Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

    // Create Devices and install them in the Nodes (eNB and UE)
    NetDeviceContainer enbDevs;
    NetDeviceContainer ueDevs;
    // NetDeviceContainer ueDevs2;
    // NetDeviceContainer ueDevs3;
    // NetDeviceContainer ueDevs4;

    enbDevs = lteHelper->InstallEnbDevice(enbNodes);
    ueDevs = lteHelper->InstallUeDevice(ueNodes);
    // ueDevs2 = lteHelper->InstallUeDevice(ueNodes2);
    // ueDevs2 = lteHelper->InstallUeDevice(ueNodes3);
    // ueDevs2 = lteHelper->InstallUeDevice(ueNodes4);
	
    // // Attach UEs to a eNB
    // lteHelper->Attach(ueDevs1, enbDevs.Get(0));
    // lteHelper->Attach(ueDevs2, enbDevs.Get(1));
    // lteHelper->Attach(ueDevs3, enbDevs.Get(2));
    // lteHelper->Attach(ueDevs4, enbDevs.Get(3));
    std::cout << "\n No. of UE nodes " << ueNodes.GetN() << "\n"; //temp
    
    Config::SetDefault("ns3::LteEnbPhy::TxPower", DoubleValue(30));

    //Configure enb's 
    for(uint32_t k = 0; k < enbDevs.GetN(); k++)
    {
        enbDevs.Get(k)->GetObject<LteEnbNetDevice>()->GetPhy()->SetTxPower(30.0); //Tx Power
        enbDevs.Get(k)->GetObject<LteEnbNetDevice>()->SetDlBandwidth(50); //DL RBs
        enbDevs.Get(k)->GetObject<LteEnbNetDevice>()->SetUlBandwidth(50); //UL RBs
    }

    //Installing Ip stacks
    internet.Install (ueNodes);


    Ipv4InterfaceContainer ueIpIface;
    ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueDevs));
    lteHelper->AddX2Interface(enbNodes);

    std::cout << "\n UE IpAddress: " << ueIpIface.GetAddress(0) << "\n"; //temp
    std::cout << "\n RH IpAddress: " << internetIpIfaces.GetAddress(0) << "\n"; //temp
    
    
    // configuring routing between ue's
    for (uint32_t e = 0; e < ueNodes.GetN (); ++e)
    {
        Ptr<Node> ue = ueNodes.Get (e);
        Ptr<Ipv4StaticRouting> ueRouting = ipv4rout.GetStaticRouting(ue->GetObject<Ipv4> ());
        ueRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }
    std::cout << "\n Default Router Address of UE's: " << epcHelper->GetUeDefaultGatewayAddress () << "\n"; //temp

    lteHelper->AttachToClosestEnb(ueDevs, enbDevs);

 
    // Install and start applications on UEs and remote host
    
    uint32_t dlPort = 1121;

    ApplicationContainer client;
    ApplicationContainer server;

    for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
        PacketSinkHelper sink("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), dlPort));
        server.Add(sink.Install(ueNodes.Get(u)));
        
        UdpClientHelper dlc(ueIpIface.GetAddress(u), dlPort);
        
        if(fullBufferFlag)
        {
            dlc.SetAttribute("Interval", TimeValue(MilliSeconds(1)));
            dlc.SetAttribute("PacketSize", UintegerValue(1500));
            client.Add(dlc.Install(rh));
        }
        else
        {
            dlc.SetAttribute("MaxPackets", UintegerValue(100000));
            dlc.SetAttribute("Interval", TimeValue(MilliSeconds(10)));
            dlc.SetAttribute("PacketSize", UintegerValue(1500));
            client.Add(dlc.Install(rh));
        }
        
    }


    std::cout << "\n No. of Server Apps: " << server.GetN() << "\n"; 
    server.Get(0)->GetNode()->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();

    std::cout << "\n Ip Address of Server App: " << server.Get(5)->GetNode()->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal() << "\n"; //temp 

    //starting app and stoping applications
    server.Start (Seconds (0.01));
    client.Start (Seconds (0.01));
    p2p.EnablePcapAll("NWSassign2");

    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> mon;
    mon = flowmon.Install(ueNodes);
    mon = flowmon.Install(rh);
    
   
    Simulator::Stop(Seconds(10));

    lteHelper->EnableTraces();

    //Avg Throughput Trace
    stream.open("avg-throupghut-trace-nonbuffer.csv", std::ios_base::app);
    stream << "schType, speed, totalThroughPut\n";
    // enableCdfThroughPutTrace(serverApps);
    for (uint32_t i = 0; i < server.GetN (); i++)
    {
      Ptr<PacketSink> sink = DynamicCast<PacketSink> (server.Get (i));
    }

    Simulator::ScheduleDestroy (&CdfThroughputTrace);
    
    
    // enableThroughPutTraceSingle(flowmon);
    Simulator::Schedule (MilliSeconds(1.0), &CalculateThroughput, &flowmon);
    Simulator::ScheduleDestroy (&ThroughputTrace);   

    Ptr<RadioEnvironmentMapHelper> remHelper;    
    remHelper = CreateObject<RadioEnvironmentMapHelper>();
    remHelper->SetAttribute("Channel", PointerValue(lteHelper->GetDownlinkSpectrumChannel()));
    // remHelper->SetAttribute("ChannelPath", StringValue("/ChannelList/0"));
    remHelper->SetAttribute("OutputFile", StringValue("rem.out"));
    remHelper->SetAttribute("XMin", DoubleValue(-2000.0));
    remHelper->SetAttribute("XMax", DoubleValue(5000.0));
    remHelper->SetAttribute("XRes", UintegerValue(500));
    remHelper->SetAttribute("YMin", DoubleValue(-2000.0));
    remHelper->SetAttribute("YMax", DoubleValue(5000.0));
    remHelper->SetAttribute("YRes", UintegerValue(500));
    remHelper->SetAttribute("Z", DoubleValue(0.0));
    remHelper->SetAttribute("UseDataChannel", BooleanValue(true));
    remHelper->SetAttribute("RbId", IntegerValue(-1));
    remHelper->Install();
 
    Simulator::Run();
    stats(flowmon, true);
    Simulator::Destroy();
    return 0;
}
