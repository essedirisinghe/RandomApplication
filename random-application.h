/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef RANDOM_APPLICATION_H
#define RANDOM_APPLICATION_H

#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/data-rate.h"
#include "ns3/traced-callback.h"
#include "ns3/seq-ts-size-header.h"

namespace ns3 {

class Address;
class RandomVariableStream;
class Socket;

class RandomApplication : public Application
{
public:

  static TypeId GetTypeId (void);
  RandomApplication ();
  virtual ~RandomApplication();
  void SetMaxBytes (uint64_t maxBytes);
  Ptr<Socket> GetSocket (void) const;
  int64_t AssignStreams (int64_t stream);

protected:
  virtual void DoDispose (void);
private:
  // inherited from Application base class.
  virtual void StartApplication (void);    // Called at time specified by Start
  virtual void StopApplication (void);     // Called at time specified by Stop

  void CancelEvents ();
  void StartSending ();
  void SendPacket ();

  Ptr<Socket>     m_socket;       //!< Associated socket
  Address         m_peer;         //!< Peer address
  Address         m_local;        //!< Local address to bind to
  bool            m_connected;    //!< True if connected
  Ptr<RandomVariableStream>  m_interval;       //!< rng for On Time
  Ptr<RandomVariableStream>  m_size;      //!< rng for Off Time
  uint32_t        m_pktSize;      //!< Size of packets
  Time            m_lastStartTime; //!< Time last packet sent
  uint64_t        m_maxBytes;     //!< Limit total number of bytes sent
  uint64_t        m_totBytes;     //!< Total bytes sent so far
  EventId         m_startStopEvent;     //!< Event id for next start or stop event
  EventId         m_sendEvent;    //!< Event id of pending "send packet" event
  TypeId          m_tid;          //!< Type of the socket used
  uint32_t        m_seq {0};      //!< Sequence
  Ptr<Packet>     m_unsentPacket; //!< Unsent packet cached for future attempt
  bool            m_enableSeqTsSizeHeader {false}; //!< Enable or disable the use of SeqTsSizeHeader


  /// Traced Callback: transmitted packets.
  TracedCallback<Ptr<const Packet> > m_txTrace;

  /// Callbacks for tracing the packet Tx events, includes source and destination addresses
  TracedCallback<Ptr<const Packet>, const Address &, const Address &> m_txTraceWithAddresses;

  /// Callback for tracing the packet Tx events, includes source, destination, the packet sent, and header
  TracedCallback<Ptr<const Packet>, const Address &, const Address &, const SeqTsSizeHeader &> m_txTraceWithSeqTsSize;

private:

  void ScheduleNextTx ();
  void ScheduleStartEvent ();
  void ConnectionSucceeded (Ptr<Socket> socket);
  void ConnectionFailed (Ptr<Socket> socket);
};

} // namespace ns3

#endif /* RANDOM_APPLICATION_H */
