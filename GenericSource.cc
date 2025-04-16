// Copyright (C) [2025] [Muhammad Waqas]
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.





#include <omnetpp.h>

using namespace omnetpp;

namespace processor {

class GenericSource : public cSimpleModule
{
  private:
    cMessage *sendMessageEvent = nullptr;
    simsignal_t msgGeneratedSignal;
    std::string sourceId;

  public:
    virtual ~GenericSource();

  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};

Define_Module(GenericSource);

GenericSource::~GenericSource()
{
    cancelAndDelete(sendMessageEvent);
}

void GenericSource::initialize()
{
    sourceId = par("sourceId").stringValue();
    sendMessageEvent = new cMessage(("sendMessageEvent-" + sourceId).c_str());
    scheduleAt(simTime(), sendMessageEvent);
    msgGeneratedSignal = registerSignal("msgGenerated");
}


void GenericSource::handleMessage(cMessage *msg)
{
    ASSERT(msg == sendMessageEvent);

    cMessage *job = new cMessage(("job-" + sourceId).c_str());
    int requiredResourceValue = par("requiredResource").intValue();

    job->addPar("origin").setStringValue(sourceId.c_str());
    job->addPar("requiredResource").setLongValue(requiredResourceValue);

    job->addPar("serviceTime").setDoubleValue(par("serviceTime").doubleValue());
    job->setTimestamp();
    // Logging message ID and required resources
    EV << "Generated message from " << sourceId << " with ID: " << job->getId()
       << ", Required Resource: " << requiredResourceValue<< endl;


    send(job, "out");
    scheduleAt(simTime()+par("interarrivalTime").doubleValue(), sendMessageEvent);
    emit(msgGeneratedSignal, 1);
}

}; //namespace
