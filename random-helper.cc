/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021 The University of Melbourne
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
 * Author: Sampath Edirisinghe <essedirisinghe@gmail.com>
 */

#include "random-helper.h"
#include "ns3/inet-socket-address.h"
#include "ns3/packet-socket-address.h"
#include "ns3/string.h"
#include "ns3/data-rate.h"
#include "ns3/uinteger.h"
#include "ns3/names.h"
#include "ns3/random-variable-stream.h"
#include "ns3/random-application.h"

namespace ns3 {

RandomHelper::RandomHelper (std::string protocol, Address address)
{
  m_factory.SetTypeId ("ns3::RandomApplication");
  m_factory.Set ("Protocol", StringValue (protocol));
  m_factory.Set ("Remote", AddressValue (address));
}

void
RandomHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
RandomHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
RandomHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
RandomHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
RandomHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<Application> ();
  node->AddApplication (app);

  return app;
}

int64_t
RandomHelper::AssignStreams (NodeContainer c, int64_t stream)
{
  int64_t currentStream = stream;
  Ptr<Node> node;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      node = (*i);
      for (uint32_t j = 0; j < node->GetNApplications (); j++)
        {
          Ptr<RandomApplication> onoff = DynamicCast<RandomApplication> (node->GetApplication (j));
          if (onoff)
            {
              currentStream += onoff->AssignStreams (currentStream);
            }
        }
    }
  return (currentStream - stream);
}

} // namespace ns3
