#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("RandomPacketApplication");

int
main (int argc, char *argv[])
{
  Time::SetResolution (Time::NS);
  LogComponentEnable ("RandomApplication", LOG_LEVEL_INFO);

  NodeContainer nodes;
  nodes.Create (2);
  
  ApplicationContainer sourceApplications;
  ApplicationContainer sinkApplications;

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes);

  InternetStackHelper stack;
  stack.Install (nodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");

  Ipv4InterfaceContainer interfaces = address.Assign (devices);
  
  InetSocketAddress sinkSocket (interfaces.GetAddress (0), 9);
  sinkSocket.SetTos (0xc0);
      
  PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", sinkSocket);
  sinkApplications.Add (packetSinkHelper.Install (nodes.Get (0)));
  
  RandomHelper randomPacketGenerator  ("ns3::UdpSocketFactory", sinkSocket);
  //randomPacketGenerator.SetAttribute ("Interval", StringValue   ("ns3::UniformRandomVariable[Min=0.001|Max=0.01]"));
  randomPacketGenerator.SetAttribute ("PacketSize", StringValue  ("ns3::UniformRandomVariable[Min=500|Max=1000]"));
  //randomPacketGenerator.SetAttribute ("PacketSize", StringValue  ("ns3::NormalRandomVariable[Mean=500|Variance=300]"));
  //randomPacketGenerator.SetAttribute ("PacketSize", StringValue  ("ns3::ConstantRandomVariable[Constant=500]"));
  randomPacketGenerator.SetAttribute ("Interval", StringValue   ("ns3::ConstantRandomVariable[Constant=0.1]"));
  randomPacketGenerator.SetAttribute ("MaxBytes", UintegerValue (0));
  
  sourceApplications.Add (randomPacketGenerator.Install (nodes.Get (1))); 

  Simulator::Stop (Seconds (60));
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
