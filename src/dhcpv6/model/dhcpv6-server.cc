/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 *  Copyright (c) 2011 UPB
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
 * Author: Radu Lupu <rlupu@elcom.pub.ro>
 */


#include "ns3/log.h"
#include "ns3/ipv6-address.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/config.h"
#include "dhcpv6-server.h"
#include "dhcpv6-header.h"
#include <ns3/ipv6.h>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Dhcpv6Server");
NS_OBJECT_ENSURE_REGISTERED (Dhcpv6Server);


TypeId
Dhcpv6Server::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Dhcpv6Server")
    .SetParent<Application> ()
    .AddConstructor<Dhcpv6Server> ()
    .AddAttribute ("Port",
                   "Port on which we listen for incoming packets.",
                   UintegerValue (547),
                   MakeUintegerAccessor (&Dhcpv6Server::m_port),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("PoolAddresses",
                   "Pool of addresses to provide on request.",
                   Ipv6AddressValue (),
                   MakeIpv6AddressAccessor (&Dhcpv6Server::m_poolAddress),
                   MakeIpv6AddressChecker ())
    .AddAttribute ("PoolPrefix",
                   "Prefix of the pool of addresses.",
                   Ipv6PrefixValue (),
                   MakeIpv6PrefixAccessor (&Dhcpv6Server::m_poolMask),
                   MakeIpv6PrefixChecker ())
    .AddAttribute ("Option_1",
                   "Here of type IPv4 address. (e.g. gw addr or other server addr)",
                   Ipv6AddressValue (),
                   MakeIpv6AddressAccessor (&Dhcpv6Server::m_option1),
                   MakeIpv6AddressChecker ())
  ;
  return tid;
}

Dhcpv6Server::Dhcpv6Server ()
{
  NS_LOG_FUNCTION (this);
  m_IDhost = 1;
  m_state = WORKING;
}

Dhcpv6Server::~Dhcpv6Server ()
{
  NS_LOG_FUNCTION (this);
}

void
Dhcpv6Server::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

uint128_t Dhcpv6Server::getdiff(Ipv6Address a, Ipv6Address b)
{
uint8_t buffa[16], buffb[16];
int i;
a.GetBytes(buffa);
b.GetBytes(buffb);
uint128_t diff, ba=0, bb=0;
for(i=0;i<=15;i++)
{
ba=((ba*256)+buffa[i]);
bb=((bb*256)+buffb[i]);
}
diff=ba-bb;
return diff;
}


void Dhcpv6Server::StartApplication(void){
  NS_LOG_FUNCTION (this);
    
  if (m_socket == 0)
    {
    uint128_t addrIndex;
    int32_t ifIndex;
    
    //dhcpv6add the DHCPV6 local address to the leased addresses list, if it is defined!
    Ptr<Ipv6> ipv6 = GetNode()->GetObject<Ipv6> (); 
    ifIndex = ipv6->GetInterfaceForPrefix(m_poolAddress, m_poolMask);
    if(ifIndex >= 0){
        for(addrIndex = 0; addrIndex < ipv6->GetNAddresses(ifIndex); addrIndex++){
            if((ipv6->GetAddress(ifIndex, addrIndex).GetAddress().CombinePrefix(m_poolMask).IsEqual(m_poolAddress.CombinePrefix(m_poolMask)))){
                uint128_t id = getdiff(ipv6->GetAddress(ifIndex, addrIndex).GetAddress(), m_poolAddress);
                m_leasedAddresses.push_back(std::make_pair (id, 0xff));//set infinite GRANTED_LEASED_TIME for my address
                break;
            }
        }
    }
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      Inet6SocketAddress local = Inet6SocketAddress (Ipv6Address::GetAny(), m_port);
      m_socket->SetAllowBroadcast (true);
//      Ptr<Ipv4> ipv4 = GetNode()->GetObject<Ipv4> (); 
//      m_socket->BindToNetDevice(ipv4->GetNetDevice(ipv4->GetInterfaceForPrefix(Ipv4Address("172.30.0.1"), Ipv4Mask("/24"))));
//      m_socket->BindToNetDevice(ipv4->GetNetDevice(2));
      m_socket->Bind (local);
    }
    m_socket->SetRecvCallback(MakeCallback(&Dhcpv6Server::NetHandler, this)); 
    m_expiredEvent = Simulator::Schedule (Seconds (LEASE_TIMEOUT), &Dhcpv6Server::TimerHandler, this);
}


void Dhcpv6Server::StopApplication (){
  NS_LOG_FUNCTION (this);

  if (m_socket != 0)
    {
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }
    Simulator::Remove(m_expiredEvent);
}

void Dhcpv6Server::NetHandler (Ptr<Socket> socket){
   NS_LOG_FUNCTION (this << socket);
   RunEfsm();
}

void Dhcpv6Server::TimerHandler(){
    //setup no of timer out events and release of unsolicited addresses from the list!
    int a=1;
    std::list<std::pair<uint128_t, uint8_t> >::iterator i;
    for (i = m_leasedAddresses.begin (); i != m_leasedAddresses.end (); i++, a++){   
        //update the addr state 
        if(i->second <= 1){
            m_leasedAddresses.erase(i--);    //release this addr
            NS_LOG_INFO ("[node " << GetNode()->GetId() << "]  "<< "Address removed!"<< a);
            continue;
        }
        if(i->second != 0xff) i->second--;
    }
    NS_LOG_INFO ("[node " << GetNode()->GetId() << "]  "<< "Address leased state expired!");
    m_expiredEvent = Simulator::Schedule (Seconds (LEASE_TIMEOUT), &Dhcpv6Server::TimerHandler, this);
}

void Dhcpv6Server::dhcpv6add(Ipv6Address a, uint128_t b, uint8_t r[])
{
uint8_t by[16];
int i;
uint128_t addr=0, res;
a.GetBytes(by);
for (i=0;i<16;i++)
for(i=0;i<16;i++)
addr=addr*256+by[i];
res=addr+b;
for(i=15;i>=0;i--)
{
r[i]=res%256;
res/=256;
}
}


void Dhcpv6Server::RunEfsm(void){
    Dhcpv6Header header, new_header;  
    Address from;
    Ptr<Packet> packet = 0;
    Ipv6Address address;
    Mac48Address source;
    
    switch(m_state){
    case WORKING:
        uint8_t addr[16];
        packet = Create<Packet> (DHCPV6_HEADER_LENGTH);    //header (no payload added)
        packet = m_socket->RecvFrom (from);
        packet->RemoveHeader(header);
        source = header.GetChaddr();
        
        if(header.GetType() == DHCPV6SOLICIT){
            NS_LOG_INFO ("[node " << GetNode()->GetId() << "]  "<< "Trace RX: DHCPV6 SOLICIT from: " << Inet6SocketAddress::ConvertFrom (from).GetIpv6 () << 
            " source port: " <<  Inet6SocketAddress::ConvertFrom (from).GetPort ());

            //figure out a new address to be leased
            for(;;){//if no free addr found, stay here for_ever
                std::list<std::pair<uint128_t, uint8_t> >::const_iterator i;
                for (i = m_leasedAddresses.begin (); i != m_leasedAddresses.end (); i++){   
                    //check wheather the addr is busy
                    if(i->first == m_IDhost){
                        m_IDhost = m_IDhost % (uint128_t)(pow(2, 128 - m_poolMask.GetPrefixLength())-1) + 1;
                        break;
                    }
                }
                if(i == m_leasedAddresses.end ()){
                //free addr found => dhcpv6add to the list and set the lifetime 
                    dhcpv6add(m_poolAddress, m_IDhost, addr);
                    m_leasedAddresses.push_back(std::make_pair (m_IDhost, GRANTED_LEASE_TIME));
                    address.Set(addr); 
//                    NS_LOG_INFO ("leased address is=" << address);
    
                    m_IDhost = m_IDhost % (uint128_t)(pow(2, 128 - m_poolMask.GetPrefixLength())-1) + 1;
                    break;
                }
            }
//            packet->RemoveAllPacketTags ();
//            packet->RemoveAllByteTags ();
            packet = Create<Packet> (DHCPV6_HEADER_LENGTH);
            new_header.SetType (DHCPV6ADVERTISE);
            new_header.SetChaddr(source);
            new_header.SetYiaddr(address);
            new_header.SetOption_1(m_option1);
            packet->AddHeader (new_header);

            if ((m_socket->SendTo(packet, 0, Inet6SocketAddress(Inet6SocketAddress::ConvertFrom (from).GetIpv6 (), Inet6SocketAddress::ConvertFrom (from).GetPort ()))) >= 0){
                NS_LOG_INFO ("[node " << GetNode()->GetId() << "]  "<< "Trace TX: DHCPV6 ADVERTISE");
            }else{
                NS_LOG_INFO ("[node " << GetNode()->GetId() << "]  "<< "Error while sending DHCPV6 ADVERTISE");
            }
        }
        
        if(header.GetType() == DHCPV6REQ){
            uint8_t addr[16]; 
            Ipv6Address address; 

            if (packet->GetSize () > 0){
                //packet->CopyData ((uint8_t*)&addr, sizeof(addr));
                address=header.GetOption_1();
                address.GetBytes(addr);

                NS_LOG_INFO ("[node " << GetNode()->GetId() << "]  "<< "Trace RX: DHCPV6 REQUEST from: " << Inet6SocketAddress::ConvertFrom (from).GetIpv6 () << 
                             " source port: " <<  Inet6SocketAddress::ConvertFrom (from).GetPort () << " refreshed addr is =" << address);
            
                            source = header.GetChaddr();
                std::list<std::pair<uint128_t, uint8_t> >::iterator i;
                for (i = m_leasedAddresses.begin (); i != m_leasedAddresses.end (); i++){   
                    //update the lifetime of this addr
                    if(getdiff(address, m_poolAddress) == i->first){
                        (i->second)++;
                         packet = Create<Packet> (DHCPV6_HEADER_LENGTH);
            new_header.SetType (DHCPV6REPLY);
            new_header.SetChaddr(source);
            new_header.SetYiaddr(address);
            packet->AddHeader (new_header);
            m_peer= Inet6SocketAddress::ConvertFrom (from).GetIpv6 ();
 NS_LOG_INFO ("[node " << GetNode()->GetId() << "]  "<< "Trace TX: DHCPV6 REPLY SENT"<<source);
       if(m_peer!=address)
       m_socket->SendTo(packet, 0, Inet6SocketAddress(Inet6SocketAddress::ConvertFrom (from).GetIpv6(), Inet6SocketAddress::ConvertFrom (from).GetPort ()));
       else
       m_socket->SendTo(packet, 0, Inet6SocketAddress(m_peer, Inet6SocketAddress::ConvertFrom (from).GetPort ()));
                        break;
                    }
                }
                if(i == m_leasedAddresses.end ()){
                    NS_LOG_INFO ("[node " << GetNode()->GetId() << "]  "<< "This IP addr does not exists or released!");
                   packet = Create<Packet> (DHCPV6_HEADER_LENGTH);
            new_header.SetType (DHCPV6NREPLY);
            new_header.SetChaddr(source);
            new_header.SetYiaddr(address);
          
            packet->AddHeader (new_header);
 NS_LOG_INFO ("[node " << GetNode()->GetId() << "]  "<< "Trace TX: DHCPV6 NREPLY SENT");
       m_socket->SendTo(packet, 0, Inet6SocketAddress(Inet6SocketAddress::ConvertFrom (from).GetIpv6(), Inet6SocketAddress::ConvertFrom (from).GetPort ()));
                }
            }
        }
    }
}


} // Namespace ns3
