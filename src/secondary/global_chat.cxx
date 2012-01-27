/**
 * @file
 * @author Jason Lingle
 * @brief Implements src/secondary/global_chat.hxx
 */

/*
 * global_chat.cxx
 *
 *  Created on: 28.02.2011
 *      Author: jason
 */

#include <string>
#include <deque>

#include "global_chat.hxx"
#include "confreg.hxx"
#include "src/globals.hxx"

using namespace std;

namespace global_chat {
  deque<message> messages;
  //Start at a number near wrap-around to make sure
  //all code that uses the seqs does so correctly
  static unsigned nextSeq=(unsigned)-100;

  void post(const char* str) noth {
    postLocal(str);
    //postRemote(str);
  }

  void postLocal(const char* str) noth {
    message msg;
    msg.text = str;
    msg.timeLeft = conf["conf"]["chat_message_time"];
    msg.seq = nextSeq++;
    messages.push_back(msg);
    if (messages.size() > (unsigned)conf["conf"]["max_chat_messages"])
      messages.pop_front();
  }

  //postRemote in net/peer.cxx

  void update(float et) noth {
    if (messages.empty()) return;
    messages[0].timeLeft -= et;
    if (messages[0].timeLeft < 0)
      messages.pop_front();
  }

  void clear() noth {
    messages.clear();
  }
}
