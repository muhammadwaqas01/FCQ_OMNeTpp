#include <omnetpp.h>

using namespace omnetpp;

namespace processor {

class Sink : public cSimpleModule {
  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};

Define_Module(Sink);

void Sink::initialize() {

}

void Sink::handleMessage(cMessage *msg) {

    delete msg;
}

}; //namespace
