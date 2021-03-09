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

#ifndef RANDOM_HELPER_H
#define RANDOM_HELPER_H

#include <stdint.h>
#include <string>
#include "ns3/object-factory.h"
#include "ns3/address.h"
#include "ns3/attribute.h"
#include "ns3/net-device.h"
#include "ns3/node-container.h"
#include "ns3/application-container.h"
#include "ns3/random-application.h"

namespace ns3 {

class DataRate;

class RandomHelper
{
public:

  RandomHelper (std::string protocol, Address address);
  void SetAttribute (std::string name, const AttributeValue &value);
  ApplicationContainer Install (NodeContainer c) const;
  ApplicationContainer Install (Ptr<Node> node) const;
  ApplicationContainer Install (std::string nodeName) const;
  int64_t AssignStreams (NodeContainer c, int64_t stream);

private:
  Ptr<Application> InstallPriv (Ptr<Node> node) const;
  ObjectFactory m_factory; //!< Object factory.
};

} // namespace ns3

#endif /* RANDOM_HELPER_H */
