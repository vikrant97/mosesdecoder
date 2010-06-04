#pragma once

#include <map>
#include <utility>

#include "Derivation.h"
#include "FeatureVector.h"
#include "Gibbler.h"
#include "MpiDebug.h"
#include "StaticData.h"
#include "SampleCollector.h"

using namespace Moses;

namespace Josiah {

class GainFunction;
//class Derivation;

class ExpectedLossCollector : public SampleCollector {
  public:
  ExpectedLossCollector( const GainFunction*  f)  { addGainFunction(f);}
  ExpectedLossCollector()  {}
  
    virtual ~ExpectedLossCollector() {}
    virtual void collect(Sample& sample);
    // returns the expected gain and expected sentence length
    virtual float UpdateGradient(FVector* gradient, FValue* exp_len, FValue* unreg_gain);
    void addGainFunction (const GainFunction* f) {g.push_back(f);}
    virtual FVector getFeatureExpectations() const;
    double getExpectedGain() const;
    
  protected:
    /** Hooks for adding, eg, entropy regularisation. The first is added in to the gradient, the second to the objective.*/
    virtual FValue getRegularisationGradientFactor(size_t i) {return 0;}
    virtual FValue getRegularisation() {return 0;}
    virtual bool ComputeScaleGradient() {return false;}
    vector<const GainFunction*> g;
    std::vector<FVector> m_featureVectors;
    std::vector<FVector> m_rbFeatureVectors; // Rao-Blackwellised feature vectors
    std::vector<FValue> m_gains;
    std::vector<size_t> m_lengths;
//    std::vector<Derivation> m_samples;
    
    
  
};

}
