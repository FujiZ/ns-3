/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 NITK Surathkal
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
 * Authors: Shravya Ks <shravya.ks0@gmail.com>
 *
 */

#include "tcp-general-test.h"
#include "ns3/node.h"
#include "ns3/log.h"
#include "tcp-error-model.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TcpECNTestSuite");

class TcpECNTest : public TcpGeneralTest
{
public:
  TcpECNTest (uint32_t testcase, const std::string &desc);

protected:
  virtual void Rx (const Ptr<const Packet> p, const TcpHeader&h, SocketWho who);
  virtual void Tx (const Ptr<const Packet> p, const TcpHeader&h, SocketWho who);
  void ConfigureProperties ();

private:
  uint32_t m_senderSent;
  uint32_t m_receiverSent;
  uint32_t m_senderReceived;
  uint32_t m_receiverReceived;
  uint32_t m_testcase;
};

TcpECNTest::TcpECNTest (uint32_t testcase, const std::string &desc)
  : TcpGeneralTest (desc),
    m_senderSent (0),
    m_receiverSent (0),
    m_senderReceived (0),
    m_receiverReceived (0),
    m_testcase (testcase)
{
}

void
TcpECNTest::ConfigureProperties ()
{
  TcpGeneralTest::ConfigureProperties ();
  if (m_testcase == 2 || m_testcase == 4)
    {
      SetECN (SENDER);
    }
  if (m_testcase == 3 || m_testcase == 4)
    {
      SetECN (RECEIVER);
    }
}

void
TcpECNTest::Rx (const Ptr<const Packet> p, const TcpHeader &h, SocketWho who)
{
  if (who == RECEIVER)
    {
      if (m_receiverReceived == 0)
        {
          NS_TEST_ASSERT_MSG_NE (((h.GetFlags ()) & TcpHeader::SYN), 0, "SYN should be received as first message at the receiver");
          if (m_testcase == 2 || m_testcase == 4)
            {
              NS_TEST_ASSERT_MSG_NE (((h.GetFlags ()) & TcpHeader::ECE) && ((h.GetFlags ()) & TcpHeader::CWR), 0, "The flags ECE + CWR should be set in the TCP header of first message receieved at receiver when sender is ECN Capable");
            }
          else
            {
              NS_TEST_ASSERT_MSG_EQ (((h.GetFlags ()) & TcpHeader::ECE) && ((h.GetFlags ()) & TcpHeader::CWR), 0, "The flags ECE + CWR should not be set in the TCP header of first message receieved at receiver when sender is not ECN Capable");
            }
          m_receiverReceived++;
        }
      else if (m_receiverReceived == 1)
        {
          NS_TEST_ASSERT_MSG_NE (((h.GetFlags ()) & TcpHeader::ACK), 0, "ACK should be received as second message at receiver");
          m_receiverReceived++;
        }
    }
  else if (who == SENDER)
    {
      if (m_receiverSent == 0)
        {
          NS_TEST_ASSERT_MSG_NE (((h.GetFlags ()) & TcpHeader::SYN) && ((h.GetFlags ()) & TcpHeader::ACK), 0, "SYN+ACK received as first message at sender");
          if (m_testcase == 4)
            {
              NS_TEST_ASSERT_MSG_NE (((h.GetFlags ()) & TcpHeader::ECE), 0, "The flag ECE should be set in the TCP header of first message receieved at sender when both receiver and sender are ECN Capable");
            }
          else
            {
              NS_TEST_ASSERT_MSG_EQ (((h.GetFlags ()) & TcpHeader::ECE), 0, "The flag ECE should not be set in the TCP header of first message receieved at sender when  either receiver or sender are not ECN Capable");
            }
          m_receiverSent++;
        }
    }
}

void
TcpECNTest::Tx (const Ptr<const Packet> p, const TcpHeader &h, SocketWho who)
{
  if (who == SENDER)
    {
      m_senderSent++;
      if (m_senderSent == 3)
        {
          SocketIpTosTag ipTosTag;
          p->PeekPacketTag (ipTosTag);
          if (m_testcase == 4)
            {
              NS_TEST_ASSERT_MSG_EQ ((ipTosTag.GetTos ()), 0x02, "IP TOS should have ECT set if ECN negotiation between endpoints is successful");
            }
          else
            {
              NS_TEST_ASSERT_MSG_NE ((ipTosTag.GetTos ()), 0x02, "IP TOS should not have ECT set if ECN negotiation between endpoints is unsuccessful");
            }
        }
    }
}

//-----------------------------------------------------------------------------

static class TcpECNTestSuite : public TestSuite
{
public:
  TcpECNTestSuite () : TestSuite ("tcp-ecn-test", UNIT)
  {
    AddTestCase (new TcpECNTest (1, "ECN incapable sender and ECN incapable receiver"),
                 TestCase::QUICK);
    AddTestCase (new TcpECNTest (2, "ECN capable sender and ECN incapable receiver"),
                 TestCase::QUICK);
    AddTestCase (new TcpECNTest (3, "ECN incapable sender and ECN capable receiver"),
                 TestCase::QUICK);
    AddTestCase (new TcpECNTest (4, "ECN capable sender and ECN capable receiver"),
                 TestCase::QUICK);
  }
} g_tcpECNTestSuite;

} // namespace ns3
