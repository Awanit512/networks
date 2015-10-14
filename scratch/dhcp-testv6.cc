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
 */

// Network topology:
//      (client)      (server and gw)
//        MN ========== Router 
//

#include <fstream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/dhcpv6-module.h"


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Dhcpv6ServiceTest");


  
/*void ConfigureUdpServer(void)
{
  Ptr<Dhcpv6Client> dhcpClient = MN.Get(0)->GetApplication(0)->GetObject<Dhcpv6Client>();
  Ipv4Address serveraddr = dhcpClient->GetOption_1();
  NS_LOG_INFO ("DHCPV6 server is at=" << serveraddr);
  Ptr<UdpClient> udpClient = MN.Get(0)->GetApplication(1)->GetObject<UdpClient>();
  udpClient->SetRemote(serveraddr, 4000);
}*/
  
int
main (int argc, char *argv[])
{
//
// Enable logging 
//
//  LogComponentEnable ("Dhcpv6ServiceTest", LOG_LEVEL_INFO);

  LogComponentEnable ("Dhcpv6Client", LOG_LEVEL_INFO);
  LogComponentEnable ("Dhcpv6Server", LOG_LEVEL_INFO);
  
  LogComponentEnable ("UdpClient", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpServer", LOG_LEVEL_INFO);


 CommandLine cmd;
 cmd.Parse (argc, argv);

NodeContainer MN;
NodeContainer Router;
  
//
// Explicitly create the nodes required by the topology (depicted above).
//
  NS_LOG_INFO ("Create nodes.");

  MN.Create(2);
  Router.Create(1);
  
  NodeContainer net(MN, Router);
    
  NS_LOG_INFO ("Create channels.");
//
// Explicitly create the channels required by the topology (shown above).
//
  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", DataRateValue (DataRate (5000000)));
  csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));
  csma.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  NetDeviceContainer dev_net = csma.Install(net);

//
// We've got the "hardware" in place.  Now we need to add IP addresses.
//
  InternetStackHelper tcpip;
  tcpip.Install(MN);
  tcpip.Install(Router);


  //MN configuration: i/f create + setup
  Ptr<Ipv6> ipv6MN = net.Get(0)->GetObject<Ipv6> (); 
  uint32_t ifIndex = ipv6MN->AddInterface (dev_net.Get(0));
  //ipv6MN->AddAddress (ifIndex, Ipv6InterfaceAddress (Ipv6Address ("2002:0:6::11"), Ipv6Prefix (64)));
  ipv6MN->SetForwarding(ifIndex, true);
  ipv6MN->SetUp (ifIndex); 

  ipv6MN = net.Get(1)->GetObject<Ipv6> (); 
   ifIndex = ipv6MN->AddInterface (dev_net.Get(1));
  //ipv6MN->AddAddress (ifIndex, Ipv6InterfaceAddress (Ipv6Address ("2002:0:6::11"), Ipv6Prefix (64)));
  ipv6MN->SetForwarding(ifIndex, true);
  ipv6MN->SetUp (ifIndex); 
  
  //Router configuration: i/f create + setup
  Ptr<Ipv6> ipv6Router = net.Get(2)->GetObject<Ipv6> (); 
  ifIndex = ipv6Router->AddInterface (dev_net.Get(2));
  /*ipv6Router->AddAddress (ifIndex, Ipv6InterfaceAddress (Ipv6Address ("172.30.0.1"), Ipv4Mask ("/0"))); //workaround (to support undirected broadcast in ns-3.12.1)!!!!!*/
  ipv6Router->AddAddress (ifIndex, Ipv6InterfaceAddress (Ipv6Address ("2002:0:6::1"), Ipv6Prefix (64)));
  ipv6Router->SetForwarding(ifIndex, true);
  ipv6Router->SetUp (ifIndex); 




  NS_LOG_INFO ("Create Applications.");
//
// Create the network and install related service modules on all nodes.
//
  uint16_t port = 547;
  Dhcpv6ServerHelper dhcp_server(Ipv6Address("2002:0:6::1"), Ipv6Prefix(64), Ipv6Address("2002:0:6::1"), port);
  ApplicationContainer ap_dhcp_server = dhcp_server.Install(Router.Get(0));
  ap_dhcp_server.Start (Seconds (1.0)); 
  ap_dhcp_server.Stop (Seconds (10.0));

  Dhcpv6ClientHelper dhcp_client(port);
  ApplicationContainer ap_dhcp_client = dhcp_client.Install(MN.Get(0));
  ap_dhcp_client.Start (Seconds (1.0)); 
  ap_dhcp_client.Stop (Seconds (10.0));

Dhcpv6ClientHelper dhcp_client1(port);
  ApplicationContainer ap_dhcp_client1 = dhcp_client1.Install(MN.Get(1));
  ap_dhcp_client1.Start (Seconds (1.0)); 
  ap_dhcp_client1.Stop (Seconds (10.0));


 
 /* port = 4000;
  UdpServerHelper server (port);
  ApplicationContainer apps = server.Install (Router.Get (0));
  apps.Start (Seconds (1.0));
  apps.Stop (Seconds (10.0));

  uint32_t MaxPacketSize = 1024;
  Time interPacketInterval = Seconds (0.05);
  uint32_t maxPacketCount = 320;
  
  //alternativelly, the option_1 (=Udp Server addr) may be read by UdpClient through DHCPV6_IPC_PORT;
  Simulator::Schedule (Seconds (2.0), &ConfigureUdpServer); 
  
  UdpClientHelper client (Ipv4Address::GetAny(), port);
  client.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
  client.SetAttribute ("Interval", TimeValue (interPacketInterval));
  client.SetAttribute ("PacketSize", UintegerValue (MaxPacketSize));
  apps = client.Install (MN.Get (0));
  apps.Start (Seconds (3.0));
  apps.Stop (Seconds (10.0));*/

 
 
  Simulator::Stop (Seconds (40.0)); 
//
// Now, do the actual simulation.
//
  NS_LOG_INFO ("Run Simulation.");
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");
}
