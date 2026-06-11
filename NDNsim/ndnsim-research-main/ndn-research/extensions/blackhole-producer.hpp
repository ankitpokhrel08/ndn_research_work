#ifndef BLACKHOLE_PRODUCER_HPP
#define BLACKHOLE_PRODUCER_HPP

#include "ns3/ndnSIM/apps/ndn-app.hpp"
#include "ns3/ndnSIM/model/ndn-common.hpp"

namespace ns3 {
namespace ndn {

// Registers a prefix so interests for it are delivered here and enter
// the PIT, then does nothing -> they expire at InterestLifetime.
// This is what makes IFA interests "unsatisfiable" without triggering a NACK.
class BlackholeProducer : public App {
public:
  static TypeId GetTypeId();
  BlackholeProducer();
  virtual void OnInterest(shared_ptr<const Interest> interest) override;

protected:
  virtual void StartApplication() override;
  virtual void StopApplication() override;

private:
  Name m_prefix;
};

} // namespace ndn
} // namespace ns3
#endif