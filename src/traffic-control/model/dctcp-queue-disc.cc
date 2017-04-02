/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007, 2014 University of Washington
 *               2015 Universita' degli Studi di Napoli Federico II
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
 * Authors:  Stefano Avallone <stavallo@unina.it>
 *           Tom Henderson <tomhend@u.washington.edu>
 */

#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/object-factory.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/socket.h"
#include "dctcp-queue-disc.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DctcpFastQueueDisc");

NS_OBJECT_ENSURE_REGISTERED (DctcpFastQueueDisc);

TypeId DctcpFastQueueDisc::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DctcpFastQueueDisc")
    .SetParent<QueueDisc> ()
    .SetGroupName ("TrafficControl")
    .AddConstructor<DctcpFastQueueDisc> ()
    .AddAttribute ("Limit",
                   "The maximum number of packets accepted by this queue disc.",
                   UintegerValue (1000),
                   MakeUintegerAccessor (&DctcpFastQueueDisc::m_limit),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Threshold",
                   "The threshold value above which packets will be marked by this queue disc.",
                   UintegerValue (40),
                   MakeUintegerAccessor (&DctcpFastQueueDisc::m_k),
                   MakeUintegerChecker<uint32_t> ())
  ;
  return tid;
}

DctcpFastQueueDisc::DctcpFastQueueDisc ()
{
  NS_LOG_FUNCTION (this);
}

DctcpFastQueueDisc::~DctcpFastQueueDisc ()
{
  NS_LOG_FUNCTION (this);
}

bool
DctcpFastQueueDisc::DoEnqueue (Ptr<QueueDiscItem> item)
{
  NS_LOG_FUNCTION (this << item);

  if (GetNPackets () > m_limit)
    {
      NS_LOG_LOGIC ("Queue disc limit exceeded -- dropping packet");
      Drop (item);
      return false;
    }

  if (GetNPackets () > m_k)
    {
      item->Mark ();
      NS_LOG_LOGIC ("Marking due to exceding threshold " << item);
    }

  bool retval = GetInternalQueue (0)->Enqueue (item);

  // If Queue::Enqueue fails, QueueDisc::Drop is called by the internal queue
  // because QueueDisc::AddInternalQueue sets the drop callback

  NS_LOG_LOGIC ("Number packets in queue: " << GetInternalQueue (0)->GetNPackets ());

  return retval;
}


uint32_t
DctcpFastQueueDisc::GetQueueSize (void)
{
  NS_LOG_FUNCTION (this);
  if (GetInternalQueue (0)->GetMode () == Queue::QUEUE_MODE_BYTES)
    {
      return GetInternalQueue (0)->GetNBytes ();
    }
  else if (GetInternalQueue (0)->GetMode () == Queue::QUEUE_MODE_PACKETS)
    {
      return GetInternalQueue (0)->GetNPackets ();
    }
  else
    {
      NS_ABORT_MSG ("Unknown RED mode.");
    }
}

Ptr<QueueDiscItem>
DctcpFastQueueDisc::DoDequeue (void)
{
  NS_LOG_FUNCTION (this);

  Ptr<QueueDiscItem> item;

  if ((item = StaticCast<QueueDiscItem> (GetInternalQueue (0)->Dequeue ())) != 0)
    {
      NS_LOG_LOGIC ("Popped from queue: " << item);
      NS_LOG_LOGIC ("Number packets in queue: " << GetInternalQueue (0)->GetNPackets ());
      return item;
    }
  
  NS_LOG_LOGIC ("Queue empty");
  return item;
}

Ptr<const QueueDiscItem>
DctcpFastQueueDisc::DoPeek (void) const
{
  NS_LOG_FUNCTION (this);

  Ptr<const QueueDiscItem> item;

    {
      item = StaticCast<const QueueDiscItem> (GetInternalQueue (0)->Peek ());
      NS_LOG_LOGIC ("Peeked from queue: " << item);
      NS_LOG_LOGIC ("Number packets in queue: " << GetInternalQueue (0)->GetNPackets ());
      return item;
    }

  NS_LOG_LOGIC ("Queue empty");
  return item;
}

bool
DctcpFastQueueDisc::CheckConfig (void)
{
  NS_LOG_FUNCTION (this);
  if (GetNQueueDiscClasses () > 0)
    {
      NS_LOG_ERROR ("DctcpFastQueueDisc cannot have classes");
      return false;
    }

  if (GetNPacketFilters () != 0)
    {
      NS_LOG_ERROR ("DctcpFastQueueDisc needs no packet filter");
      return false;
    }

  if (GetNInternalQueues () == 0)
    {
      // create 1 DropTail queues with m_limit packets each
      ObjectFactory factory;
      factory.SetTypeId ("ns3::DropTailQueue");
      factory.Set ("Mode", EnumValue (Queue::QUEUE_MODE_PACKETS));
      factory.Set ("MaxPackets", UintegerValue (m_limit));
      AddInternalQueue (factory.Create<Queue> ());
    }

  if (GetNInternalQueues () != 1)
    {
      NS_LOG_ERROR ("DctcpFastQueueDisc needs 1 internal queues");
      return false;
    }

  if (GetInternalQueue (0)-> GetMode () != Queue::QUEUE_MODE_PACKETS)
    {
      NS_LOG_ERROR ("DctcpFastQueueDisc needs 1 internal queues operating in packet mode");
      return false;
    }

  if (GetInternalQueue (0)->GetMaxPackets () < m_limit)
    {
      NS_LOG_ERROR ("The capacity of the internal queue is less than the queue disc capacity");
      return false;
    }

  return true;
}

void
DctcpFastQueueDisc::InitializeParams (void)
{
  NS_LOG_FUNCTION (this);
}

} // namespace ns3
