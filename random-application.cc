/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
//
// Copyright (c) 2021 The University of Melbourne
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation;
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Author: Sampath Edirisinghe<essedirisinghe@gmail.com>
//

#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/packet-socket-address.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"
#include "ns3/random-variable-stream.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include "ns3/boolean.h"
#include "ns3/trace-source-accessor.h"
#include "random-application.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RandomApplication");

NS_OBJECT_ENSURE_REGISTERED (RandomApplication);

TypeId
RandomApplication::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RandomApplication")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<RandomApplication> ()
    .AddAttribute ("Remote", "The address of the destination",
                   AddressValue (),
                   MakeAddressAccessor (&RandomApplication::m_peer),
                   MakeAddressChecker ())
    .AddAttribute ("Local",
                   "The Address on which to bind the socket. If not set, it is generated automatically.",
                   AddressValue (),
                   MakeAddressAccessor (&RandomApplication::m_local),
                   MakeAddressChecker ())
    .AddAttribute ("Interval", "A RandomVariableStream used to pick the interval between packets",
                   StringValue ("ns3::ConstantRandomVariable[Constant=1]"),
                   MakePointerAccessor (&RandomApplication::m_interval),
                   MakePointerChecker <RandomVariableStream>())
    .AddAttribute ("PacketSize", "A RandomVariableStream used to pick the packet size",
                   StringValue ("ns3::ConstantRandomVariable[Constant=1000]"),
                   MakePointerAccessor (&RandomApplication::m_size),
                   MakePointerChecker <RandomVariableStream>())
    .AddAttribute ("MaxBytes",
                   "The total number of bytes to send. Once these bytes are sent, "
                   "no packet is sent again, even in on state. The value zero means "
                   "that there is no limit.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RandomApplication::m_maxBytes),
                   MakeUintegerChecker<uint64_t> ())
    .AddAttribute ("Protocol", "The type of protocol to use. This should be "
                   "a subclass of ns3::SocketFactory",
                   TypeIdValue (UdpSocketFactory::GetTypeId ()),
                   MakeTypeIdAccessor (&RandomApplication::m_tid),
                   // This should check for SocketFactory as a parent
                   MakeTypeIdChecker ())
    .AddAttribute ("EnableSeqTsSizeHeader",
                   "Enable use of SeqTsSizeHeader for sequence number and timestamp",
                   BooleanValue (false),
                   MakeBooleanAccessor (&RandomApplication::m_enableSeqTsSizeHeader),
                   MakeBooleanChecker ())
    .AddTraceSource ("Tx", "A new packet is created and is sent",
                     MakeTraceSourceAccessor (&RandomApplication::m_txTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("TxWithAddresses", "A new packet is created and is sent",
                     MakeTraceSourceAccessor (&RandomApplication::m_txTraceWithAddresses),
                     "ns3::Packet::TwoAddressTracedCallback")
    .AddTraceSource ("TxWithSeqTsSize", "A new packet is created with SeqTsSizeHeader",
                     MakeTraceSourceAccessor (&RandomApplication::m_txTraceWithSeqTsSize),
                     "ns3::PacketSink::SeqTsSizeCallback")
  ;
  return tid;
}


RandomApplication::RandomApplication ()
  : m_socket (0),
    m_connected (false),
    m_lastStartTime (Seconds (0)),
    m_totBytes (0),
    m_unsentPacket (0)
{
  NS_LOG_FUNCTION (this);
}

RandomApplication::~RandomApplication()
{
  NS_LOG_FUNCTION (this);
}

void
RandomApplication::SetMaxBytes (uint64_t maxBytes)
{
  NS_LOG_FUNCTION (this << maxBytes);
  m_maxBytes = maxBytes;
}

Ptr<Socket>
RandomApplication::GetSocket (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socket;
}

int64_t
RandomApplication::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_interval->SetStream (stream);
  m_size->SetStream (stream + 1);
  return 2;
}

void
RandomApplication::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  CancelEvents ();
  m_socket = 0;
  m_unsentPacket = 0;
  // chain up
  Application::DoDispose ();
}

// Application Methods
void RandomApplication::StartApplication () // Called at time specified by Start
{
  NS_LOG_FUNCTION (this);

  // Create the socket if not already
  if (!m_socket)
    {
      m_socket = Socket::CreateSocket (GetNode (), m_tid);
      int ret = -1;

      if (! m_local.IsInvalid())
        {
          NS_ABORT_MSG_IF ((Inet6SocketAddress::IsMatchingType (m_peer) && InetSocketAddress::IsMatchingType (m_local)) ||
                           (InetSocketAddress::IsMatchingType (m_peer) && Inet6SocketAddress::IsMatchingType (m_local)),
                           "Incompatible peer and local address IP version");
          ret = m_socket->Bind (m_local);
        }
      else
        {
          if (Inet6SocketAddress::IsMatchingType (m_peer))
            {
              ret = m_socket->Bind6 ();
            }
          else if (InetSocketAddress::IsMatchingType (m_peer) ||
                   PacketSocketAddress::IsMatchingType (m_peer))
            {
              ret = m_socket->Bind ();
            }
        }

      if (ret == -1)
        {
          NS_FATAL_ERROR ("Failed to bind socket");
        }

      m_socket->Connect (m_peer);
      m_socket->SetAllowBroadcast (true);
      m_socket->ShutdownRecv ();

      m_socket->SetConnectCallback (
        MakeCallback (&RandomApplication::ConnectionSucceeded, this),
        MakeCallback (&RandomApplication::ConnectionFailed, this));
    }

  // Insure no pending event
  CancelEvents ();
  // If we are not yet connected, there is nothing to do here
  // The ConnectionComplete upcall will start timers at that time
  //if (!m_connected) return;
  ScheduleStartEvent ();
}

void RandomApplication::StopApplication () // Called at time specified by Stop
{
  NS_LOG_FUNCTION (this);

  CancelEvents ();
  if(m_socket != 0)
    {
      m_socket->Close ();
    }
  else
    {
      NS_LOG_WARN ("OnOffApplication found null socket to close in StopApplication");
    }
}

void RandomApplication::CancelEvents ()
{
  NS_LOG_FUNCTION (this);

  Simulator::Cancel (m_sendEvent);
  Simulator::Cancel (m_startStopEvent);
  // Canceling events may cause discontinuity in sequence number if the
  // SeqTsSizeHeader is header, and m_unsentPacket is true
  if (m_unsentPacket)
    {
      NS_LOG_DEBUG ("Discarding cached packet upon CancelEvents ()");
    }
  m_unsentPacket = 0;
}

// Event handlers
void RandomApplication::StartSending ()
{
  NS_LOG_FUNCTION (this);
  m_lastStartTime = Simulator::Now ();
  ScheduleNextTx ();  // Schedule the send packet event
}

// Private helpers
void RandomApplication::ScheduleNextTx ()
{
  NS_LOG_FUNCTION (this);

  if (m_maxBytes == 0 || m_totBytes < m_maxBytes)
    {
      Time nextTime = Seconds (m_interval->GetValue ());
      m_sendEvent = Simulator::Schedule (nextTime,
                                         &RandomApplication::SendPacket, this);
    }
  else
    { // All done, cancel any pending events
      StopApplication ();
    }
}

void RandomApplication::ScheduleStartEvent ()
{  // Schedules the event to start sending data (switch to the "On" state)
  NS_LOG_FUNCTION (this);

  Time offInterval = Seconds (m_interval->GetValue ());
  NS_LOG_LOGIC ("start at " << offInterval.As (Time::S));
  m_startStopEvent = Simulator::Schedule (offInterval, &RandomApplication::StartSending, this);
}

void RandomApplication::SendPacket ()
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT (m_sendEvent.IsExpired ());

  Ptr<Packet> packet;

  if (m_unsentPacket)
    {
      packet = m_unsentPacket;
    }
  else if (m_enableSeqTsSizeHeader)
    {
      m_pktSize = m_size->GetValue ();
      Address from, to;
      m_socket->GetSockName (from);
      m_socket->GetPeerName (to);
      SeqTsSizeHeader header;
      header.SetSeq (m_seq++);
      header.SetSize (m_pktSize);
      NS_ABORT_IF (m_pktSize < header.GetSerializedSize ());
      packet = Create<Packet> (m_pktSize - header.GetSerializedSize ());
      // Trace before adding header, for consistency with PacketSink
      m_txTraceWithSeqTsSize (packet, from, to, header);
      packet->AddHeader (header);
    }
  else
    {
      m_pktSize = m_size->GetValue ();
      packet = Create<Packet> (m_pktSize);
    }

  int actual = m_socket->Send (packet);
  if ((unsigned) actual == m_pktSize)
    {
      m_txTrace (packet);
      m_totBytes += m_pktSize;

      m_unsentPacket = 0;
      Address localAddress;
      m_socket->GetSockName (localAddress);
      if (InetSocketAddress::IsMatchingType (m_peer))
        {

           NS_LOG_INFO ("Time " << Simulator::Now ().As (Time::S)
                       << " size "
                       <<  packet->GetSize ());

          m_txTraceWithAddresses (packet, localAddress, InetSocketAddress::ConvertFrom (m_peer));
        }
      else if (Inet6SocketAddress::IsMatchingType (m_peer))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().As (Time::S)
                       << " on-off application sent "
                       <<  packet->GetSize () << " bytes to "
                       << Inet6SocketAddress::ConvertFrom(m_peer).GetIpv6 ()
                       << " port " << Inet6SocketAddress::ConvertFrom (m_peer).GetPort ()
                       << " total Tx " << m_totBytes << " bytes");
          m_txTraceWithAddresses (packet, localAddress, Inet6SocketAddress::ConvertFrom(m_peer));
        }
    }
  else
    {
      NS_LOG_DEBUG ("Unable to send packet; actual " << actual << " size " << m_pktSize << "; caching for later attempt");
      m_unsentPacket = packet;
    }

  m_lastStartTime = Simulator::Now ();
  ScheduleNextTx ();
}

void RandomApplication::ConnectionSucceeded (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  m_connected = true;
}

void RandomApplication::ConnectionFailed (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_FATAL_ERROR ("Can't connect");
}


} // Namespace ns3
