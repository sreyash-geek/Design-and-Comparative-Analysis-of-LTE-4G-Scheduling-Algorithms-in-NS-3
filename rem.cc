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
#include "ns3/spectrum-module.h"
#include <ns3/buildings-helper.h>
// #include "ns3/gtk-config-store.h"

using namespace ns3;

int
main(int argc, char* argv[])
{
    CommandLine cmd(__FILE__);
    cmd.Parse(argc, argv);

    // to save a template default attribute file run it like this:
    // ./ns3 run src/lte/examples/lena-first-sim --command-template="%s
    // --ns3::ConfigStore::Filename=input-defaults.txt --ns3::ConfigStore::Mode=Save
    // --ns3::ConfigStore::FileFormat=RawText"
    //
    // to load a previously created default attribute file
    // ./ns3 run src/lte/examples/lena-first-sim --command-template="%s
    // --ns3::ConfigStore::Filename=input-defaults.txt --ns3::ConfigStore::Mode=Load
    // --ns3::ConfigStore::FileFormat=RawText"

    ConfigStore inputConfig;
    inputConfig.ConfigureDefaults();

    // Parse again so you can override default values from the command line
    cmd.Parse(argc, argv);

    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();


    // Uncomment to enable logging
    // lteHelper->EnableLogComponents ();

    // Create Nodes: eNodeB and UE
    Config::SetDefault("ns3::LteEnbPhy::TxPower", DoubleValue(30));
    NodeContainer enbNodes;
    NodeContainer ueNodes;
    enbNodes.Create(4);
    ueNodes.Create(4);
    int enbDist = 1000;
    Ptr<ListPositionAllocator> enbpositionAlloc = CreateObject<ListPositionAllocator> ();
    enbpositionAlloc->Add (Vector(0, 0, 0)); 
    enbpositionAlloc->Add (Vector(enbDist, 0, 0));
    enbpositionAlloc->Add (Vector(0, enbDist, 0));
    enbpositionAlloc->Add (Vector(enbDist, enbDist, 0));

    // Install Mobility Model
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.SetPositionAllocator(enbpositionAlloc);
    mobility.Install(enbNodes);
    BuildingsHelper::Install(enbNodes);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(ueNodes);
    BuildingsHelper::Install(ueNodes);

    // Create Devices and install them in the Nodes (eNB and UE)
    NetDeviceContainer enbDevs;
    NetDeviceContainer ueDevs;
    // Default scheduler is PF, uncomment to use RR
    lteHelper->SetSchedulerType ("ns3::RrFfMacScheduler");

    enbDevs = lteHelper->InstallEnbDevice(enbNodes);
    ueDevs = lteHelper->InstallUeDevice(ueNodes);

    // Attach a UE to a eNB
    lteHelper->Attach(ueDevs.Get(0), enbDevs.Get(0));
    lteHelper->Attach(ueDevs.Get(1), enbDevs.Get(1));
    lteHelper->Attach(ueDevs.Get(2), enbDevs.Get(2));
    lteHelper->Attach(ueDevs.Get(3), enbDevs.Get(3));

    // Activate an EPS bearer
    EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;
    EpsBearer bearer(q);
    lteHelper->ActivateDataRadioBearer(ueDevs, bearer);

    // Configure Radio Environment Map (REM) output
    // for LTE-only simulations always use /ChannelList/0 which is the downlink channel
    Ptr<RadioEnvironmentMapHelper> remHelper = CreateObject<RadioEnvironmentMapHelper>();

    remHelper = CreateObject<RadioEnvironmentMapHelper>();
    remHelper->SetAttribute("Channel", PointerValue(lteHelper->GetDownlinkSpectrumChannel()));
    remHelper->SetAttribute("OutputFile", StringValue("rem.out"));
    remHelper->SetAttribute("XMin", DoubleValue(-10000.0));
    remHelper->SetAttribute("XMax", DoubleValue(10000.0));
    remHelper->SetAttribute("XRes", UintegerValue(500));
    remHelper->SetAttribute("YMin", DoubleValue(-10000.0));
    remHelper->SetAttribute("YMax", DoubleValue(10000.0));
    remHelper->SetAttribute("YRes", UintegerValue(500));
    remHelper->SetAttribute("Z", DoubleValue(0.0));
    remHelper->SetAttribute("UseDataChannel", BooleanValue(true));
    remHelper->SetAttribute("RbId", IntegerValue(-1));
    remHelper->Install();


    Simulator::Run();

    Simulator::Destroy();
    return 0;
}
