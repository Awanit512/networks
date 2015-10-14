/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 UPB
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

#ifndef DHCPV6_HEADER_H
#define DHCPV6_HEADER_H

#include "ns3/header.h"
#include <ns3/mac48-address.h>

#define DHCPV6_HEADER_LENGTH      48
//1+6+4+4 bytes !!!

namespace ns3 {
/**
 * \ingroup udpclientserver
 * \class SeqTsHeader
 * \brief Packet header for Udp client/server application
 * The header is made of a 32bits sequence number followed by
 * a 64bits time stamp.
 */
class Dhcpv6Header : public Header
{
public:
  Dhcpv6Header ();

  /**
   * \param seq the sequence number
   */
    void SetType (uint8_t type);
  /**
   * \return the sequence number
   */
  uint8_t GetType (void) const;
  /**
   * \return the time stamp
   */
//  Time GetTs (void) const;
    static TypeId GetTypeId (void);
    void SetChaddr (Mac48Address addr);
    Mac48Address GetChaddr (void);
    void SetYiaddr (Ipv6Address addr);
    Ipv6Address GetYiaddr (void) const;
    void SetOption_1 (Ipv6Address addr);
    Ipv6Address GetOption_1 (void) const;
    
    
private:
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

  uint8_t m_op;
  Mac48Address m_chaddr;
  Ipv6Address m_yiaddr;
  Ipv6Address m_option_1;
};

} // namespace ns3

#endif /* DHCPV6_HEADER_H */